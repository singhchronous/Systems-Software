/*
 * bank_stress.c - Concurrency / race-condition demonstrator.
 *
 * Phase 1  "one session per user": 5 processes try to log in as the SAME
 *          customer at the same instant. Exactly one must succeed.
 * Phase 2  "money conservation": N concurrent clients (processes created with
 *          fork(), or threads with --threads) each log in as their own test
 *          customer and fire M random transfers at the other test accounts,
 *          all starting at the same moment (a pipe is used as a starting gun).
 *          Transfers only MOVE money, so the total across the test accounts
 *          must be exactly the same before and after.
 *
 *   With the normal server            -> total unchanged      (PASS)
 *   With ./bank_server --no-lock ...  -> lost updates, total changes (FAIL)
 *
 * Results travel from the worker processes back to the parent through a pipe.
 *
 * Usage: ./bank_stress [-n clients] [-m transfers] [--threads]
 *                      [-H host] [-p port] [-e employee_id] [-w employee_password]
 */
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "net.h"
#include "protocol.h"
#include "records.h"
#include "util.h"

#define TEST_PASSWORD "stress123"
#define OPENING_RUPEES 10000

static const char *g_host = "127.0.0.1";
static int g_port = DEFAULT_PORT;
static int g_n = 20, g_m = 50, g_threads = 0;
static int g_emp = 2002;
static const char *g_emp_pw = "emp123";
static int32_t *g_ids;

typedef struct {
    int worker;
    int ok, insufficient, failed;
    double seconds;
} Result;

static double now_sec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

static int call(int sd, Request *rq, Response *rs)
{
    if (send_request(sd, rq) == -1) return -1;
    if (recv_response(sd, rs) <= 0) return -1;
    return rs->status == RESP_OK ? 0 : 1;
}

static int login(int sd, int role, int id, const char *pw, Response *rs)
{
    Request rq;
    memset(&rq, 0, sizeof rq);
    rq.op = OP_LOGIN;
    rq.id1 = role;
    rq.id2 = id;
    str_copy(rq.s1, pw, sizeof rq.s1);
    return call(sd, &rq, rs);
}

static void bye(int sd)
{
    Request rq;
    Response rs;
    memset(&rq, 0, sizeof rq);
    rq.op = OP_EXIT;
    call(sd, &rq, &rs);
    close(sd);
}

/* ------------------------------------------------------------- setup */

static int create_test_accounts(void)
{
    int sd = connect_to_server(g_host, g_port);
    if (sd < 0) { fprintf(stderr, "Cannot connect to %s:%d - start ./bank_server first.\n", g_host, g_port); return -1; }
    Response rs;
    if (login(sd, ROLE_EMPLOYEE, g_emp, g_emp_pw, &rs) != 0) {
        fprintf(stderr, "Employee login %d failed: %s\n", g_emp, rs.text);
        close(sd);
        return -1;
    }
    char tag[3] = {'A', 'A', 0};
    srand((unsigned)time(NULL));
    tag[0] = (char)('A' + rand() % 26);
    for (int i = 0; i < g_n; i++) {
        Request rq;
        memset(&rq, 0, sizeof rq);
        rq.op = OP_ADD_CUSTOMER;
        tag[1] = (char)('a' + i % 26);
        snprintf(rq.s1, sizeof rq.s1, "Stress");
        snprintf(rq.s2, sizeof rq.s2, "User%s%c", tag, (char)('a' + (i / 26) % 26));
        snprintf(rq.s3, sizeof rq.s3, "90000%05d", i);
        snprintf(rq.s4, sizeof rq.s4, "stress%d@test.in", i);
        str_copy(rq.text, TEST_PASSWORD, sizeof rq.text);
        rq.amount = (int64_t)OPENING_RUPEES * 100;
        if (call(sd, &rq, &rs) != 0) {
            fprintf(stderr, "Could not create test account: %s\n", rs.text);
            bye(sd);
            return -1;
        }
        g_ids[i] = (int32_t)rs.value;
    }
    bye(sd);
    return 0;
}

static int64_t balance_of(int32_t id, int *err)
{
    int sd = connect_to_server(g_host, g_port);
    Response rs;
    if (sd < 0 || login(sd, ROLE_CUSTOMER, id, TEST_PASSWORD, &rs) != 0) {
        if (sd >= 0) close(sd);
        (*err)++;
        return 0;
    }
    Request rq;
    memset(&rq, 0, sizeof rq);
    rq.op = OP_VIEW_BALANCE;
    int64_t v = 0;
    if (call(sd, &rq, &rs) == 0) v = rs.value; else (*err)++;
    bye(sd);
    return v;
}

static int64_t total_balance(int *err)
{
    int64_t t = 0;
    for (int i = 0; i < g_n; i++) t += balance_of(g_ids[i], err);
    return t;
}

/* --------------------------------------------------- phase 1: sessions */

static int phase_one_session(void)
{
    const int K = 5;
    int gun[2];
    if (pipe(gun) == -1) { perror("pipe"); return -1; }
    pid_t pids[5];
    for (int k = 0; k < K; k++) {
        pids[k] = fork();
        if (pids[k] == 0) {
            close(gun[1]);
            int sd = connect_to_server(g_host, g_port);
            char c;
            if (read(gun[0], &c, 1) < 0) { /* released when the parent closes the pipe */ }
            Response rs;
            int ok = sd >= 0 && login(sd, ROLE_CUSTOMER, g_ids[0], TEST_PASSWORD, &rs) == 0;
            if (ok) sleep(1);            /* hold the session while the others try */
            if (sd >= 0) bye(sd);
            _exit(ok ? 0 : 1);
        }
    }
    close(gun[0]);
    usleep(300000);                       /* let everyone connect and block on the gun */
    close(gun[1]);                        /* BANG: read() returns 0 in every child at once */
    int winners = 0;
    for (int k = 0; k < K; k++) {
        int st;
        waitpid(pids[k], &st, 0);
        if (WIFEXITED(st) && WEXITSTATUS(st) == 0) winners++;
    }
    printf("  %d simultaneous logins as customer %d -> %d succeeded, %d refused\n", K, g_ids[0], winners, K - winners);
    printf("  [%s] exactly one session per user\n", winners == 1 ? "PASS" : "FAIL");
    return winners == 1 ? 0 : 1;
}

/* ------------------------------------------------- phase 2: transfers */

static Result run_worker(int w, int gun_fd)
{
    Result res = {w, 0, 0, 0, 0};
    unsigned seed = (unsigned)(time(NULL) ^ (w * 7919) ^ getpid());
    int sd = connect_to_server(g_host, g_port);
    Response rs;
    if (sd < 0 || login(sd, ROLE_CUSTOMER, g_ids[w], TEST_PASSWORD, &rs) != 0) {
        res.failed = g_m;
        if (sd >= 0) close(sd);
        return res;
    }
    char c;
    if (gun_fd >= 0 && read(gun_fd, &c, 1) < 0) { /* wait for the start signal */ }
    double t0 = now_sec();
    for (int i = 0; i < g_m; i++) {
        int to = rand_r(&seed) % g_n;
        if (to == w) to = (to + 1) % g_n;
        Request rq;
        memset(&rq, 0, sizeof rq);
        rq.op = OP_TRANSFER;
        rq.id1 = g_ids[to];
        rq.amount = (int64_t)(1 + rand_r(&seed) % 500) * 100;
        int r = call(sd, &rq, &rs);
        if (r == 0) res.ok++;
        else if (r == 1 && strstr(rs.text, "Insufficient")) res.insufficient++;
        else res.failed++;
    }
    res.seconds = now_sec() - t0;
    bye(sd);
    return res;
}

typedef struct {
    int w;
    int gun_fd;
    Result res;
} ThreadArg;

static void *thread_main(void *p)
{
    ThreadArg *a = p;
    a->res = run_worker(a->w, a->gun_fd);
    return NULL;
}

static int phase_conservation(void)
{
    int err = 0;
    int64_t before = total_balance(&err);
    char b[40];
    format_money(before, b, sizeof b);
    printf("  Total of the %d test accounts BEFORE: %s\n", g_n, b);
    printf("  Starting %d concurrent %s x %d random transfers each...\n", g_n, g_threads ? "threads" : "processes", g_m);

    int gun[2], results[2];
    if (pipe(gun) == -1 || pipe(results) == -1) { perror("pipe"); return -1; }
    Result tot = {0, 0, 0, 0, 0};
    double t0 = now_sec();

    if (g_threads) {
        ThreadArg *args = calloc((size_t)g_n, sizeof *args);
        pthread_t *th = calloc((size_t)g_n, sizeof *th);
        for (int w = 0; w < g_n; w++) {
            args[w].w = w;
            args[w].gun_fd = gun[0];
            pthread_create(&th[w], NULL, thread_main, &args[w]);
        }
        usleep(500000);
        close(gun[1]);
        for (int w = 0; w < g_n; w++) {
            pthread_join(th[w], NULL);
            tot.ok += args[w].res.ok;
            tot.insufficient += args[w].res.insufficient;
            tot.failed += args[w].res.failed;
        }
        free(args);
        free(th);
    } else {
        for (int w = 0; w < g_n; w++) {
            pid_t pid = fork();
            if (pid == 0) {
                close(gun[1]);
                close(results[0]);
                Result r = run_worker(w, gun[0]);
                if (write(results[1], &r, sizeof r) < 0) { /* parent gone */ }
                _exit(0);
            }
        }
        close(results[1]);
        close(gun[0]);
        usleep(500000);                   /* all workers logged in and waiting */
        close(gun[1]);                    /* start! */
        Result r;
        while (read(results[0], &r, sizeof r) == (ssize_t)sizeof r) {   /* IPC: pipe */
            tot.ok += r.ok;
            tot.insufficient += r.insufficient;
            tot.failed += r.failed;
        }
        while (wait(NULL) > 0) {}
    }
    double elapsed = now_sec() - t0;

    int64_t after = total_balance(&err);
    char a[40], diff[40];
    format_money(after, a, sizeof a);
    format_money(after - before, diff, sizeof diff);
    int total_ops = tot.ok + tot.insufficient + tot.failed;
    printf("  Done in %.2f s: %d transfers succeeded, %d refused (insufficient balance), %d errors  (%.0f ops/s)\n",
           elapsed, tot.ok, tot.insufficient, tot.failed, total_ops / (elapsed > 0 ? elapsed : 1));
    printf("  Total of the %d test accounts AFTER : %s\n", g_n, a);
    if (before == after && err == 0) {
        printf("  [PASS] money conserved - no lost updates, no race condition\n");
        return 0;
    }
    printf("  [FAIL] total changed by %s -> RACE CONDITION (lost updates)%s\n", diff,
           err ? " / some balance queries failed" : "");
    return 1;
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n") && i + 1 < argc) g_n = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-m") && i + 1 < argc) g_m = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-H") && i + 1 < argc) g_host = argv[++i];
        else if (!strcmp(argv[i], "-p") && i + 1 < argc) g_port = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-e") && i + 1 < argc) g_emp = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-w") && i + 1 < argc) g_emp_pw = argv[++i];
        else if (!strcmp(argv[i], "--threads")) g_threads = 1;
        else {
            printf("Usage: %s [-n clients] [-m transfers] [--threads] [-H host] [-p port] [-e emp_id] [-w emp_password]\n", argv[0]);
            return 2;
        }
    }
    if (g_n < 2) g_n = 2;
    if (g_n > 100) g_n = 100;
    signal(SIGPIPE, SIG_IGN);
    setvbuf(stdout, NULL, _IOLBF, 0);
    g_ids = calloc((size_t)g_n, sizeof *g_ids);

    printf("=== Banking Management System - concurrency stress test ===\n");
    printf("Setup: employee %d opens %d test accounts with Rs. %d each...\n", g_emp, g_n, OPENING_RUPEES);
    if (create_test_accounts() == -1) return 2;
    printf("  test accounts %d .. %d created\n\n", g_ids[0], g_ids[g_n - 1]);

    printf("Phase 1: one session per user\n");
    int f1 = phase_one_session();
    printf("\nPhase 2: concurrent transfers (race-condition test)\n");
    int f2 = phase_conservation();

    printf("\nOverall: %s\n", (f1 || f2) ? "FAIL" : "PASS");
    printf("Tip: run ./bank_verify to check every account against its transaction history.\n");
    return (f1 || f2) ? 1 : 0;
}
