/*
 * session.h - Live session table in System V SHARED MEMORY, protected by a
 *             System V SEMAPHORE (slides 96-103).
 *
 * Enforces "one session per user": login succeeds only if no LIVE process
 * already holds a session for that user id. Because each slot stores the pid
 * of the server child that owns it, a session left behind by a crashed child
 * is detected with kill(pid, 0) and reclaimed - a user can never get stuck
 * as "already logged in" (a bug in last year's reference project).
 */
#ifndef SESSION_H
#define SESSION_H

#include <stdint.h>
#include <sys/types.h>

#define MAX_SESSIONS 128

typedef struct {
    int32_t in_use;
    int32_t role;
    int32_t uid;
    int32_t pid;            /* server child process serving this client */
    int64_t login_time;
    int64_t last_active;
    char    name[64];
    char    client_addr[32];
} SessionSlot;

typedef struct {
    int64_t     server_start;
    int32_t     server_pid;
    int32_t     total_logins;
    int32_t     total_connections;
    int32_t     reclaimed_stale;
    SessionSlot slot[MAX_SESSIONS];
} SessionTable;

/* Server start-up: create (or reuse) and reset the segment + semaphore. */
int  session_init(void);
/* Remove the shared memory segment and semaphore (graceful shutdown). */
void session_destroy(void);

/* Returns slot index >= 0, -1 if the user already has a live session (its
 * pid in *holder), -2 if the table is full. */
int  session_acquire(int role, int uid, const char *name, const char *addr, pid_t *holder);
void session_release(int slot);
void session_touch(int slot);

/* pid of the live process holding uid's session, or 0 if none. */
pid_t session_find(int uid, int *role_out);

/* Copy of the live sessions (dead ones skipped). Returns count. */
int  session_snapshot(SessionTable *out);

/* Free slots whose owner process is dead. Returns number reclaimed. */
int  session_reap_dead(void);

void session_count_connection(void);

#endif
