/*
 * server.c - Banking Management System server (UML class "Bank").
 *
 * Architecture (docs/architecture.png):
 *
 *   main process ----- accept() loop ------------------------------------------
 *     |  fork() per client  (concurrent server, slide 114)
 *     |    `- server child: handle_client_connection() for ONE client, for the
 *     |       whole session; knows who is logged in -> no impersonation
 *     |  pthread "housekeeper": reclaims dead sessions + runs crash recovery
 *     |  SIGCHLD handler: reaps children (no zombies, slide 71)
 *     |  SIGINT/SIGTERM : graceful shutdown (tell clients, remove IPC objects)
 *     `- fork() audit daemon (setsid): msgrcv() -> logs/audit.log
 *
 *   Shared between all processes:
 *     data/<table>.dat       record files + fcntl() record locks
 *     shared memory + sem   live session table (one session per user)
 *     message queue         audit log messages
 */
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "audit.h"
#include "db.h"
#include "journal.h"
#include "locking.h"
#include "net.h"
#include "seed.h"
#include "server.h"
#include "session.h"
#include "util.h"

#define BACKLOG 64
#define MAX_CHILDREN 1024

volatile sig_atomic_t g_idle_timed_out = 0;
volatile sig_atomic_t g_forced_logout = 0;
volatile sig_atomic_t g_shutting_down = 0;
int g_idle_timeout_sec = 300;

static pid_t g_children[MAX_CHILDREN];         /* live server children (main process) */
static volatile sig_atomic_t g_abnormal_exits = 0;
static pid_t g_audit_pid = -1;

/* ============================================================ responses */

static void resp_vset(Response *rs, int status, const char *fmt, va_list ap)
{
    rs->status = status;
    vsnprintf(rs->text, sizeof rs->text, fmt, ap);
}

void resp_ok(Response *rs, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    resp_vset(rs, RESP_OK, fmt, ap);
    va_end(ap);
}

void resp_err(Response *rs, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    resp_vset(rs, RESP_ERROR, fmt, ap);
    va_end(ap);
}

void resp_append(Response *rs, const char *fmt, ...)
{
    size_t used = strlen(rs->text);
    if (used + 1 >= sizeof rs->text) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(rs->text + used, sizeof rs->text - used, fmt, ap);
    va_end(ap);
}

/* ====================================================== child: signals */

static void child_sig_handler(int sig)
{
    if (sig == SIGALRM) g_idle_timed_out = 1;          /* alarm() expired       */
    else if (sig == SIGUSR1) g_forced_logout = 1;      /* admin / manager action */
    else g_shutting_down = 1;                          /* SIGTERM or SIGINT     */
}

static void install_child_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = child_sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;              /* no SA_RESTART: a signal interrupts select()/read() */
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGPIPE, SIG_IGN);
    sigset_t none;
    sigemptyset(&none);
    sigprocmask(SIG_SETMASK, &none, NULL);
}

/* ===================================================== child: dispatch */

static void end_session(ClientCtx *c, const char *how)
{
    if (!c->logged_in) return;
    session_release(c->slot);
    audit_log("LOGOUT (%s): %s %d", how, role_name(c->role), c->uid);
    c->logged_in = 0;
    c->role = ROLE_NONE;
    c->uid = 0;
    c->slot = -1;
}

static void dispatch(ClientCtx *c, const Request *rq, Response *rs)
{
    if (rq->op == OP_LOGIN) {
        if (c->logged_in) {
            resp_err(rs, "Already logged in on this connection.");
            return;
        }
        validate_login(c, rq, rs);
        return;
    }
    if (rq->op == OP_EXIT) {
        end_session(c, "exit");
        resp_ok(rs, "Goodbye!");
        return;
    }
    if (!c->logged_in) {
        resp_err(rs, "Please log in first.");
        return;
    }
    session_touch(c->slot);

    if (rq->op == OP_LOGOUT) {
        end_session(c, "logout");
        resp_ok(rs, "Logged out successfully.");
        return;
    }
    if (rq->op == OP_CHANGE_PASSWORD) {
        handle_change_password(c, rq, rs);
        return;
    }

    int handled = 0;
    switch (c->role) {          /* server-side authorisation by the SESSION's role */
    case ROLE_CUSTOMER: handled = handle_customer_requests(c, rq, rs); break;
    case ROLE_EMPLOYEE: handled = handle_employee_requests(c, rq, rs); break;
    case ROLE_MANAGER:  handled = handle_manager_requests(c, rq, rs);  break;
    case ROLE_ADMIN:    handled = handle_admin_requests(c, rq, rs);    break;
    }
    if (!handled) {
        resp_err(rs, "Permission denied: this operation is not available for a %s.", role_name(c->role));
        audit_log("DENIED op %d for %s %d", rq->op, role_name(c->role), c->uid);
    }
}

static void send_notice(ClientCtx *c, int code, const char *text)
{
    Response rs;
    memset(&rs, 0, sizeof rs);
    rs.status = RESP_NOTICE;
    rs.code = code;
    str_copy(rs.text, text, sizeof rs.text);
    send_response(c->sock, &rs);
}

/* Serves one client for its whole connection (UML: handle_client_connection). */
static void handle_client_connection(int sock, const char *addr)
{
    ClientCtx c;
    memset(&c, 0, sizeof c);
    c.sock = sock;
    c.slot = -1;
    str_copy(c.addr, addr, sizeof c.addr);
    session_count_connection();

    Request rq;
    Response rs;
    const char *end_reason = "disconnect";

    for (;;) {
        /* ---- wait for the next request, but wake up for signals ---- */
        g_idle_timed_out = 0;
        alarm((unsigned)g_idle_timeout_sec);          /* idle timer (slide 115) */
        int ready = 0;
        while (!ready) {
            if (g_shutting_down) {
                send_notice(&c, NOTICE_SERVER_SHUTDOWN, "The bank server is shutting down. You have been logged out.");
                end_reason = "server shutdown";
                goto finish;
            }
            if (g_forced_logout) {
                send_notice(&c, NOTICE_FORCED_LOGOUT, "Your session was ended by the bank (forced logout).");
                end_reason = "forced";
                goto finish;
            }
            if (g_idle_timed_out) {
                char msg[128];
                snprintf(msg, sizeof msg, "Session closed after %d seconds of inactivity.", g_idle_timeout_sec);
                send_notice(&c, NOTICE_IDLE_TIMEOUT, msg);
                end_reason = "idle timeout";
                goto finish;
            }
            /* select() with a 1 s timeout so flags set by signals are always noticed */
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(sock, &rfds);
            struct timeval tv = {1, 0};
            int n = select(sock + 1, &rfds, NULL, NULL, &tv);
            if (n > 0 && FD_ISSET(sock, &rfds)) ready = 1;
            else if (n < 0 && errno != EINTR) goto finish;
        }
        alarm(0);

        int r = recv_request(sock, &rq);
        if (r == 0) { end_reason = "client disconnected"; break; }
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }

        memset(&rs, 0, sizeof rs);
        dispatch(&c, &rq, &rs);
        if (send_response(sock, &rs) == -1) break;
        if (rq.op == OP_EXIT) { end_reason = "exit"; break; }
    }
finish:
    alarm(0);
    end_session(&c, end_reason);
    shutdown(sock, SHUT_RDWR);
    close(sock);
}

/* ===================================================== main: signals */

static void main_sigchld(int sig)
{
    (void)sig;
    int saved = errno;
    int status;
    pid_t pid;
    /* reap every finished child so none stays a zombie */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_CHILDREN; i++)
            if (g_children[i] == pid) { g_children[i] = 0; break; }
        if (WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0))
            g_abnormal_exits++;
    }
    errno = saved;
}

static void main_sigterm(int sig)
{
    (void)sig;
    g_shutting_down = 1;
}

static void install_main_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = main_sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    sa.sa_handler = main_sigterm;
    sa.sa_flags = 0;                 /* interrupt accept() */
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);
}

/* ============================================ main: housekeeper thread */

static void *housekeeper(void *arg)
{
    (void)arg;
    int last_abnormal = 0;
    while (!g_shutting_down) {
        for (int i = 0; i < 20 && !g_shutting_down; i++) usleep(100000);   /* 2 s */

        int reaped = session_reap_dead();
        if (reaped > 0) {
            fprintf(stderr, "[housekeeper] reclaimed %d session(s) of crashed server processes\n", reaped);
            audit_log("HOUSEKEEPER: reclaimed %d stale session(s)", reaped);
        }
        if (g_abnormal_exits != last_abnormal) {
            last_abnormal = g_abnormal_exits;
            fprintf(stderr, "[housekeeper] a server child terminated abnormally - checking journal\n");
        }
        if (journal_orphans_exist()) {
            int n = journal_recover(0);
            if (n > 0) fprintf(stderr, "[housekeeper] crash recovery rolled back %d operation(s)\n", n);
        }
    }
    return NULL;
}

/* ================================================================ main */

static void recovery_to_audit(const char *msg) { audit_log("%s", msg); }

static void usage(const char *prog)
{
    printf("Usage: %s [options]\n"
           "  -p <port>          TCP port (default %d)\n"
           "  -t <seconds>       idle session timeout (default 300)\n"
           "  -d <dir>           data directory (default ./data)\n"
           "  --no-lock          DISABLE record locking (race-condition demo only!)\n"
           "  --race-delay <us>  sleep between read and write of a balance (demo)\n"
           "  --crash-test       kill the server process in the middle of every transfer\n"
           "                     (crash-recovery demo)\n",
           prog, DEFAULT_PORT);
}

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    const char *data_dir = "data";

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-p") && i + 1 < argc) port = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-t") && i + 1 < argc) g_idle_timeout_sec = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) data_dir = argv[++i];
        else if (!strcmp(argv[i], "--no-lock")) g_locking_enabled = 0;
        else if (!strcmp(argv[i], "--race-delay") && i + 1 < argc) g_race_delay_us = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--crash-test")) g_crash_test = 1;
        else { usage(argv[0]); return strcmp(argv[i], "-h") && strcmp(argv[i], "--help") ? 1 : 0; }
    }
    if (g_idle_timeout_sec < 5) g_idle_timeout_sec = 5;

    setvbuf(stdout, NULL, _IOLBF, 0);
    if (db_init_dir(data_dir) == -1) return 1;
    if (mkdir("logs", 0700) == -1 && errno != EEXIST) { perror("mkdir logs"); return 1; }

    if (db_open_all() == -1) return 1;
    if (!db_files_exist()) {
        printf("No database found - creating demo data.\n");
        if (seed_database(1) == -1) return 1;
    }

    /* Crash recovery BEFORE accepting anyone: undo operations interrupted by a
     * previous crash (journal files left behind). */
    int rec = journal_recover(1);
    if (rec > 0) printf("Crash recovery: rolled back %d interrupted operation(s).\n", rec);

    if (session_init() == -1 || audit_init() == -1) {
        fprintf(stderr, "Failed to create IPC objects (try: make clean-ipc)\n");
        return 1;
    }
    char cwd[PATH_MAX - 64], log_path[PATH_MAX];
    if (!getcwd(cwd, sizeof cwd)) return 1;
    snprintf(log_path, sizeof log_path, "%s/logs/audit.log", cwd);
    g_audit_pid = audit_start_daemon(log_path);
    journal_recovery_hook = recovery_to_audit;

    install_main_signals();

    int sd = create_listen_socket(port, BACKLOG);
    if (sd < 0) {
        audit_stop(g_audit_pid);
        session_destroy();
        return 1;
    }

    /* housekeeper thread: created with the "main" signals blocked so that
     * SIGINT/SIGCHLD are always delivered to the main thread */
    sigset_t block, old;
    sigemptyset(&block);
    sigaddset(&block, SIGINT);
    sigaddset(&block, SIGTERM);
    sigaddset(&block, SIGCHLD);
    pthread_sigmask(SIG_BLOCK, &block, &old);
    pthread_t hk;
    pthread_create(&hk, NULL, housekeeper, NULL);
    pthread_sigmask(SIG_SETMASK, &old, NULL);

    audit_log("SERVER START: port %d, pid %d, idle timeout %ds%s%s", port, (int)getpid(),
              g_idle_timeout_sec, g_locking_enabled ? "" : ", LOCKING DISABLED",
              g_crash_test ? ", CRASH TEST" : "");

    printf("==============================================================\n");
    printf(" Banking Management System server   pid %d   port %d\n", (int)getpid(), port);
    printf(" Data: %s   Audit log: %s\n", db_data_dir(), log_path);
    printf(" Idle timeout: %d s   Audit daemon pid: %d\n", g_idle_timeout_sec, (int)g_audit_pid);
    if (!g_locking_enabled)
        printf(" !!! RECORD LOCKING DISABLED (--no-lock) - race conditions WILL happen !!!\n");
    if (g_race_delay_us) printf(" Race window delay: %d us\n", g_race_delay_us);
    if (g_crash_test) printf(" !!! CRASH TEST: every transfer kills its server process half-way !!!\n");
    printf(" Press Ctrl+C to shut down gracefully.\n");
    printf("==============================================================\n");

    while (!g_shutting_down) {
        struct sockaddr_in cli;
        socklen_t len = sizeof cli;
        int csd = accept(sd, (struct sockaddr *)&cli, &len);
        if (csd < 0) {
            if (errno == EINTR) continue;           /* signal: re-check the shutdown flag */
            perror("accept");
            continue;
        }
        char addr[32];
        snprintf(addr, sizeof addr, "%s:%d", inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(csd);
            continue;
        }
        if (pid == 0) {
            /* ------------------------- server child ------------------------- */
            close(sd);
            install_child_signals();
            db_close_all();             /* inherited fds share offsets with the parent */
            if (db_open_all() == -1) _exit(1);
            printf("[server] pid %d serving %s\n", (int)getpid(), addr);
            handle_client_connection(csd, addr);
            printf("[server] pid %d finished (%s)\n", (int)getpid(), addr);
            db_close_all();
            _exit(0);
        }
        /* parent */
        for (int i = 0; i < MAX_CHILDREN; i++)
            if (g_children[i] == 0) { g_children[i] = pid; break; }
        close(csd);
    }

    /* ======================= graceful shutdown ======================= */
    printf("\nShutting down: notifying connected clients...\n");
    close(sd);
    for (int i = 0; i < MAX_CHILDREN; i++)
        if (g_children[i] > 0) kill(g_children[i], SIGTERM);

    for (int waited = 0; waited < 50; waited++) {      /* up to 5 s */
        int alive = 0;
        for (int i = 0; i < MAX_CHILDREN; i++)
            if (g_children[i] > 0 && kill(g_children[i], 0) == 0) alive++;
        if (!alive) break;
        usleep(100000);
    }
    for (int i = 0; i < MAX_CHILDREN; i++)
        if (g_children[i] > 0 && kill(g_children[i], 0) == 0) kill(g_children[i], SIGKILL);
    while (waitpid(-1, NULL, WNOHANG) > 0) {}

    pthread_join(hk, NULL);
    audit_log("SERVER STOP: graceful shutdown");
    audit_stop(g_audit_pid);         /* daemon drains the queue, then the queue is removed */
    session_destroy();               /* remove shared memory + semaphore */
    db_close_all();
    printf("Server stopped cleanly (shared memory, semaphore and message queue removed).\n");
    return 0;
}
