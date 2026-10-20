/*
 * db.c - Fixed-size record files accessed with lseek()/read()/write() and
 *        protected by fcntl() record locks.
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "db.h"
#include "journal.h"
#include "locking.h"
#include "records.h"
#include "util.h"

static char g_dir[PATH_MAX] = "data";
static int g_fds[T_COUNT] = {-1, -1, -1, -1, -1, -1};
static int g_gate_fd = -1;

static const struct {
    const char *file;
    size_t      size;
    int32_t     base;
} TABLES[T_COUNT] = {
    [T_CUSTOMERS] = {"customers.dat",    sizeof(CustomerRecord),    CUSTOMER_ID_BASE},
    [T_EMPLOYEES] = {"employees.dat",    sizeof(EmployeeRecord),    EMPLOYEE_ID_BASE},
    [T_ADMINS]    = {"admins.dat",       sizeof(AdminRecord),       ADMIN_ID_BASE},
    [T_LOANS]     = {"loans.dat",        sizeof(LoanRecord),        LOAN_ID_BASE},
    [T_TXNS]      = {"transactions.dat", sizeof(TransactionRecord), TXN_ID_BASE},
    [T_FEEDBACK]  = {"feedback.dat",     sizeof(FeedbackRecord),    FEEDBACK_ID_BASE},
};

/* ------------------------------------------------------------------ setup */

int db_init_dir(const char *dir)
{
    /* store an absolute path: the audit daemon does chdir("/") */
    if (dir[0] == '/') {
        str_copy(g_dir, dir, sizeof g_dir);
    } else {
        char cwd[PATH_MAX - 256];
        if (!getcwd(cwd, sizeof cwd)) return -1;
        snprintf(g_dir, sizeof g_dir, "%s/%.200s", cwd, dir);
    }
    if (mkdir(g_dir, 0700) == -1 && errno != EEXIST) { perror("mkdir data"); return -1; }
    char jdir[PATH_MAX];
    db_path("journal", jdir, sizeof jdir);
    if (mkdir(jdir, 0700) == -1 && errno != EEXIST) { perror("mkdir journal"); return -1; }
    return 0;
}

const char *db_data_dir(void) { return g_dir; }

void db_path(const char *name, char *buf, size_t cap)
{
    snprintf(buf, cap, "%s/%s", g_dir, name);
}

int db_files_exist(void)
{
    char p[PATH_MAX];
    struct stat st;
    db_path(TABLES[T_ADMINS].file, p, sizeof p);
    return stat(p, &st) == 0 && st.st_size > 0;
}

int db_open_all(void)
{
    char p[PATH_MAX];
    for (int t = 0; t < T_COUNT; t++) {
        if (g_fds[t] >= 0) continue;
        db_path(TABLES[t].file, p, sizeof p);
        g_fds[t] = open(p, O_RDWR | O_CREAT, 0600);
        if (g_fds[t] < 0) { perror(p); return -1; }
    }
    if (g_gate_fd < 0) {
        db_path(".gate", p, sizeof p);
        g_gate_fd = open(p, O_RDWR | O_CREAT, 0600);
        if (g_gate_fd < 0) { perror(p); return -1; }
    }
    return 0;
}

void db_close_all(void)
{
    for (int t = 0; t < T_COUNT; t++) {
        if (g_fds[t] >= 0) close(g_fds[t]);
        g_fds[t] = -1;
    }
    if (g_gate_fd >= 0) close(g_gate_fd);
    g_gate_fd = -1;
}

/* ------------------------------------------------------------ metadata */

size_t db_recsize(Table t)          { return TABLES[t].size; }
int32_t db_idbase(Table t)          { return TABLES[t].base; }
const char *db_table_file(Table t)  { return TABLES[t].file; }
int db_fd(Table t)                  { return g_fds[t]; }

off_t db_offset(Table t, int32_t id)
{
    return (off_t)(id - TABLES[t].base) * (off_t)TABLES[t].size;
}

int db_count(Table t)
{
    struct stat st;
    if (fstat(g_fds[t], &st) == -1) return 0;
    return (int)(st.st_size / (off_t)TABLES[t].size);
}

int db_valid_id(Table t, int32_t id)
{
    if (id < TABLES[t].base) return 0;
    return (id - TABLES[t].base) < db_count(t);
}

/* -------------------------------------------------------------- raw I/O */

static int read_full(int fd, void *buf, size_t n)
{
    size_t done = 0;
    while (done < n) {
        ssize_t r = read(fd, (char *)buf + done, n - done);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) return -1;              /* unexpected end of file */
        done += (size_t)r;
    }
    return 0;
}

static int write_full(int fd, const void *buf, size_t n)
{
    size_t done = 0;
    while (done < n) {
        ssize_t w = write(fd, (const char *)buf + done, n - done);
        if (w < 0) { if (errno == EINTR) continue; return -1; }
        done += (size_t)w;
    }
    return 0;
}

int db_read_raw(Table t, int32_t id, void *rec)
{
    if (!db_valid_id(t, id)) return -1;
    if (lseek(g_fds[t], db_offset(t, id), SEEK_SET) == (off_t)-1) return -1;
    return read_full(g_fds[t], rec, TABLES[t].size);
}

int db_write_raw(Table t, int32_t id, const void *rec)
{
    if (id < TABLES[t].base) return -1;
    if (lseek(g_fds[t], db_offset(t, id), SEEK_SET) == (off_t)-1) return -1;
    return write_full(g_fds[t], rec, TABLES[t].size);
}

int db_sync(Table t)
{
    return fsync(g_fds[t]);            /* Durability: force data to disk */
}

/* --------------------------------------------------------------- locks */

int db_lock(Table t, int32_t id, short type)
{
    return lock_range(g_fds[t], type, db_offset(t, id), (off_t)TABLES[t].size);
}

int db_unlock(Table t, int32_t id)
{
    return unlock_range(g_fds[t], db_offset(t, id), (off_t)TABLES[t].size);
}

/* Whole-table lock = every possible record slot, but NOT the append-lock byte. */
int db_lock_table(Table t, short type)
{
    return lock_range(g_fds[t], type, 0, APPEND_LOCK_OFFSET);
}

int db_unlock_table(Table t)
{
    return unlock_range(g_fds[t], 0, APPEND_LOCK_OFFSET);
}

int db_append(Table t, void *rec, int32_t *new_id)
{
    int fd = g_fds[t];
    /* 1. exclusive "append lock": only one process at a time picks a slot */
    if (lock_range(fd, F_WRLCK, APPEND_LOCK_OFFSET, 1) == -1) return -1;

    /* 2. next id = base + number of records already in the file (get_next_id) */
    int32_t id = TABLES[t].base + db_count(t);
    memcpy(rec, &id, sizeof id);      /* every record starts with int32 id */

    /* 3. lock the new slot so scanners never see a half-written record */
    int rc = -1;
    if (lock_range(fd, F_WRLCK, db_offset(t, id), (off_t)TABLES[t].size) == 0) {
        if (db_write_raw(t, id, rec) == 0 && fsync(fd) == 0) rc = 0;
        unlock_range(fd, db_offset(t, id), (off_t)TABLES[t].size);
    }
    unlock_range(fd, APPEND_LOCK_OFFSET, 1);
    if (rc == 0 && new_id) *new_id = id;
    return rc;
}

/* ---------------------------------------------------------------- gate */

int gate_shared(void)    { return lock_range(g_gate_fd, F_RDLCK, 0, 1); }
int gate_exclusive(void) { return lock_range(g_gate_fd, F_WRLCK, 0, 1); }
int gate_release(void)   { return unlock_range(g_gate_fd, 0, 1); }

void race_window(void)
{
    if (g_race_delay_us > 0) usleep((useconds_t)g_race_delay_us);
}

/* ----------------------------------------------------- guarded helpers */

/*
 * Pattern used by every guarded helper:
 *   enter gate (shared) -> take record/table lock -> if a crashed process left
 *   a journal behind, release everything, run recovery, and start again.
 * The check happens AFTER taking the lock: a crashed process holds its locks
 * until it dies, and its journal file is created before it touches any data
 * file, so if we now own the lock and an orphan journal exists, the data we
 * are about to read may be half-updated and must be rolled back first.
 */
#define MAX_RETRIES 5

int db_get(Table t, int32_t id, void *rec)
{
    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        if (!db_valid_id(t, id)) return -1;
        gate_shared();
        db_lock(t, id, F_RDLCK);
        if (journal_orphans_exist()) {
            db_unlock(t, id);
            gate_release();
            journal_recover(0);
            continue;
        }
        int rc = db_read_raw(t, id, rec);
        db_unlock(t, id);
        gate_release();
        return rc;
    }
    return -1;
}

int db_update(Table t, int32_t id, db_update_fn fn, void *ctx, char *err, size_t errcap)
{
    unsigned char rec[JOURNAL_IMAGE_MAX];
    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        if (!db_valid_id(t, id)) return -1;
        gate_shared();
        if (db_lock(t, id, F_WRLCK) == -1) { gate_release(); return -3; }
        if (journal_orphans_exist()) {
            db_unlock(t, id);
            gate_release();
            journal_recover(0);
            continue;
        }
        int rc = 0;
        if (db_read_raw(t, id, rec) == -1) {
            rc = -3;
        } else {
            race_window();
            int r = fn(rec, ctx, err, errcap);
            if (r == -1) rc = -2;
            else if (r == 0) {
                if (db_write_raw(t, id, rec) == -1 || db_sync(t) == -1) rc = -3;
            }
        }
        db_unlock(t, id);
        gate_release();
        return rc;
    }
    return -3;
}

int db_scan(Table t, db_scan_fn fn, void *ctx)
{
    unsigned char rec[JOURNAL_IMAGE_MAX];
    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        gate_shared();
        db_lock_table(t, F_RDLCK);
        if (journal_orphans_exist()) {
            db_unlock_table(t);
            gate_release();
            journal_recover(0);
            continue;
        }
        int n = db_count(t);
        for (int i = 0; i < n; i++) {
            if (db_read_raw(t, TABLES[t].base + i, rec) == -1) break;
            if (fn(rec, ctx)) break;
        }
        db_unlock_table(t);
        gate_release();
        return 0;
    }
    return -1;
}

int db_scan_update(Table t, db_scan_update_fn fn, void *ctx)
{
    unsigned char rec[JOURNAL_IMAGE_MAX];
    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        gate_shared();
        db_lock_table(t, F_WRLCK);
        if (journal_orphans_exist()) {
            db_unlock_table(t);
            gate_release();
            journal_recover(0);
            continue;
        }
        int n = db_count(t), modified = 0;
        for (int i = 0; i < n; i++) {
            int32_t id = TABLES[t].base + i;
            if (db_read_raw(t, id, rec) == -1) break;
            if (fn(rec, ctx) == 1) {
                db_write_raw(t, id, rec);
                modified++;
            }
        }
        if (modified) db_sync(t);
        db_unlock_table(t);
        gate_release();
        return modified;
    }
    return -1;
}
