/*
 * audit.c - Message-queue based audit logger + daemon process.
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "audit.h"
#include "db.h"
#include "util.h"

struct audit_msg {
    long mtype;                       /* AUDIT_MSG_LOG or AUDIT_MSG_STOP */
    char text[AUDIT_TEXT_LEN];
};

static int g_qid = -1;

int audit_init(void)
{
    key_t key = ftok(db_data_dir(), 'Q');
    if (key == -1) { perror("ftok"); return -1; }
    g_qid = msgget(key, IPC_CREAT | 0600);
    if (g_qid == -1) { perror("msgget"); return -1; }
    return 0;
}

void audit_log(const char *fmt, ...)
{
    struct audit_msg m;
    m.mtype = AUDIT_MSG_LOG;
    char when[32];
    format_time((int64_t)time(NULL), when, sizeof when);
    int n = snprintf(m.text, sizeof m.text, "%s [pid %d] ", when, (int)getpid());
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m.text + n, sizeof m.text - (size_t)n, fmt, ap);
    va_end(ap);

    /* IPC_NOWAIT: never block a banking operation because the log is slow */
    if (g_qid == -1 || msgsnd(g_qid, &m, strlen(m.text) + 1, IPC_NOWAIT) == -1)
        fprintf(stderr, "(audit fallback) %s\n", m.text);
}

static volatile sig_atomic_t daemon_stop = 0;
static void daemon_sigterm(int sig) { (void)sig; daemon_stop = 1; }

static void kill_old_daemon(const char *pid_path)
{
    /* if a previous server was killed with -9 its daemon may still be running */
    int fd = open(pid_path, O_RDONLY);
    if (fd < 0) return;
    char buf[32] = {0};
    ssize_t r = read(fd, buf, sizeof buf - 1);
    close(fd);
    int pid;
    if (r > 0 && parse_int(buf, &pid) == 0 && pid > 1 && kill(pid, 0) == 0) {
        kill(pid, SIGTERM);
        usleep(200000);
    }
}

pid_t audit_start_daemon(const char *log_path)
{
    char abs_log[PATH_MAX], pid_path[PATH_MAX + 16];
    str_copy(abs_log, log_path, sizeof abs_log);
    snprintf(pid_path, sizeof pid_path, "%s.pid", abs_log);
    kill_old_daemon(pid_path);

    pid_t pid = fork();
    if (pid < 0) { perror("fork audit daemon"); return -1; }
    if (pid > 0) return pid;                         /* server continues */

    /* ------------------------- daemon process (slide 76) ------------------------- */
    setsid();                 /* new session: no controlling terminal, immune to Ctrl+C */
    if (chdir("/") == -1) { /* ignore */ }
    umask(027);

    int devnull = open("/dev/null", O_RDWR);         /* detach stdin/stdout/stderr */
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > 2) close(devnull);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = daemon_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    int pfd = open(pid_path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (pfd >= 0) {
        char buf[32];
        int n = snprintf(buf, sizeof buf, "%d\n", (int)getpid());
        if (write(pfd, buf, (size_t)n) < 0) { /* ignore */ }
        close(pfd);
    }

    int logfd = open(abs_log, O_WRONLY | O_CREAT | O_APPEND, 0640);
    if (logfd < 0) _exit(1);

    struct audit_msg m;
    while (!daemon_stop) {
        ssize_t n = msgrcv(g_qid, &m, sizeof m.text, 0, 0);   /* blocks */
        if (n < 0) {
            if (errno == EINTR) continue;
            break;                                           /* queue removed */
        }
        if (m.mtype == AUDIT_MSG_STOP) break;
        m.text[sizeof m.text - 1] = '\0';
        size_t len = strlen(m.text);
        if (len > sizeof m.text - 2) len = sizeof m.text - 2;
        m.text[len] = '\n';
        if (write(logfd, m.text, len + 1) < 0) { /* ignore */ }
    }
    const char bye[] = "---- audit daemon stopped ----\n";
    if (write(logfd, bye, sizeof bye - 1) < 0) { /* ignore */ }
    fsync(logfd);
    close(logfd);
    unlink(pid_path);
    _exit(0);
}

void audit_stop(pid_t daemon_pid)
{
    if (g_qid == -1) return;
    struct audit_msg m;
    m.mtype = AUDIT_MSG_STOP;
    strcpy(m.text, "stop");
    /* STOP is queued AFTER every pending log message, so nothing is lost */
    msgsnd(g_qid, &m, strlen(m.text) + 1, 0);
    if (daemon_pid > 0) {
        for (int i = 0; i < 50; i++) {                 /* wait up to 5 s */
            pid_t r = waitpid(daemon_pid, NULL, WNOHANG);
            if (r == daemon_pid || (r == -1 && kill(daemon_pid, 0) == -1)) break;
            usleep(100000);
        }
    }
    msgctl(g_qid, IPC_RMID, NULL);
    g_qid = -1;
}
