/*
 * session.c - Session table in shared memory guarded by a binary semaphore.
 *
 *   shmget/shmat  : create + attach the segment (children inherit the mapping
 *                   across fork(), so every server process sees the same table)
 *   semget/semctl : one binary semaphore initialised to 1
 *   semop P / V   : lock / unlock. SEM_UNDO makes the kernel undo the P if the
 *                   process dies inside the critical section, so a crash can
 *                   never leave the semaphore stuck at 0.
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#include "db.h"
#include "session.h"
#include "util.h"

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static int g_shmid = -1;
static int g_semid = -1;
static SessionTable *g_table = NULL;

static void sem_P(void)
{
    struct sembuf op = {0, -1, SEM_UNDO};
    while (semop(g_semid, &op, 1) == -1 && errno == EINTR) {}
}

static void sem_V(void)
{
    struct sembuf op = {0, +1, SEM_UNDO};
    while (semop(g_semid, &op, 1) == -1 && errno == EINTR) {}
}

static int alive(pid_t pid)
{
    return pid > 0 && (kill(pid, 0) == 0 || errno == EPERM);
}

int session_init(void)
{
    /* ftok() turns an existing path + project id into a System V IPC key */
    key_t shm_key = ftok(db_data_dir(), 'S');
    key_t sem_key = ftok(db_data_dir(), 'M');
    if (shm_key == -1 || sem_key == -1) { perror("ftok"); return -1; }

    g_shmid = shmget(shm_key, sizeof(SessionTable), IPC_CREAT | 0600);
    if (g_shmid == -1) {
        /* an old segment of a different size (older build) - remove and retry */
        int old = shmget(shm_key, 0, 0);
        if (old != -1) shmctl(old, IPC_RMID, NULL);
        g_shmid = shmget(shm_key, sizeof(SessionTable), IPC_CREAT | 0600);
        if (g_shmid == -1) { perror("shmget"); return -1; }
    }
    g_table = shmat(g_shmid, NULL, 0);
    if (g_table == (void *)-1) { perror("shmat"); g_table = NULL; return -1; }

    g_semid = semget(sem_key, 1, IPC_CREAT | 0600);
    if (g_semid == -1) { perror("semget"); return -1; }
    union semun arg;
    arg.val = 1;                           /* binary semaphore: 1 = free */
    if (semctl(g_semid, 0, SETVAL, arg) == -1) { perror("semctl SETVAL"); return -1; }

    sem_P();
    memset(g_table, 0, sizeof *g_table);   /* fresh server = nobody logged in */
    g_table->server_start = (int64_t)time(NULL);
    g_table->server_pid = (int32_t)getpid();
    sem_V();
    return 0;
}

void session_destroy(void)
{
    if (g_table) shmdt(g_table);
    if (g_shmid != -1) shmctl(g_shmid, IPC_RMID, NULL);
    if (g_semid != -1) semctl(g_semid, 0, IPC_RMID);
    g_table = NULL;
    g_shmid = g_semid = -1;
}

int session_acquire(int role, int uid, const char *name, const char *addr, pid_t *holder)
{
    int result = -2, free_slot = -1;
    sem_P();                                            /* ---- critical section ---- */
    for (int i = 0; i < MAX_SESSIONS; i++) {
        SessionSlot *s = &g_table->slot[i];
        if (s->in_use && !alive(s->pid)) {              /* stale: owner crashed */
            s->in_use = 0;
            g_table->reclaimed_stale++;
        }
        if (s->in_use && s->uid == uid) {
            if (holder) *holder = s->pid;
            result = -1;                                /* already logged in */
            goto out;
        }
        if (!s->in_use && free_slot == -1) free_slot = i;
    }
    if (free_slot != -1) {
        SessionSlot *s = &g_table->slot[free_slot];
        memset(s, 0, sizeof *s);
        s->in_use = 1;
        s->role = role;
        s->uid = uid;
        s->pid = (int32_t)getpid();
        s->login_time = s->last_active = (int64_t)time(NULL);
        str_copy(s->name, name, sizeof s->name);
        str_copy(s->client_addr, addr, sizeof s->client_addr);
        g_table->total_logins++;
        result = free_slot;
    }
out:
    sem_V();                                            /* -------------------------- */
    return result;
}

void session_release(int slot)
{
    if (slot < 0 || slot >= MAX_SESSIONS || !g_table) return;
    sem_P();
    if (g_table->slot[slot].pid == (int32_t)getpid()) g_table->slot[slot].in_use = 0;
    sem_V();
}

void session_touch(int slot)
{
    if (slot < 0 || slot >= MAX_SESSIONS || !g_table) return;
    sem_P();
    if (g_table->slot[slot].pid == (int32_t)getpid())
        g_table->slot[slot].last_active = (int64_t)time(NULL);
    sem_V();
}

pid_t session_find(int uid, int *role_out)
{
    pid_t pid = 0;
    sem_P();
    for (int i = 0; i < MAX_SESSIONS; i++) {
        SessionSlot *s = &g_table->slot[i];
        if (s->in_use && s->uid == uid && alive(s->pid)) {
            pid = s->pid;
            if (role_out) *role_out = s->role;
            break;
        }
    }
    sem_V();
    return pid;
}

int session_snapshot(SessionTable *out)
{
    int n = 0;
    sem_P();
    memcpy(out, g_table, sizeof *out);
    sem_V();
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (out->slot[i].in_use && !alive(out->slot[i].pid)) out->slot[i].in_use = 0;
        if (out->slot[i].in_use) n++;
    }
    return n;
}

int session_reap_dead(void)
{
    int n = 0;
    if (!g_table) return 0;
    sem_P();
    for (int i = 0; i < MAX_SESSIONS; i++) {
        SessionSlot *s = &g_table->slot[i];
        if (s->in_use && !alive(s->pid)) {
            s->in_use = 0;
            g_table->reclaimed_stale++;
            n++;
        }
    }
    sem_V();
    return n;
}

void session_count_connection(void)
{
    if (!g_table) return;
    sem_P();
    g_table->total_connections++;
    sem_V();
}
