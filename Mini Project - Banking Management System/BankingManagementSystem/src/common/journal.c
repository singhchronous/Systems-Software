/*
 * journal.c - Write-ahead undo journal + crash recovery (Atomicity, Durability).
 * See journal.h for the protocol.
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "db.h"
#include "journal.h"
#include "util.h"

int g_crash_test = 0;
void (*journal_recovery_hook)(const char *msg) = NULL;

static uint32_t checksum(const Journal *j)
{
    /* FNV-1a over everything before the checksum field */
    const uint8_t *p = (const uint8_t *)j;
    size_t n = offsetof(Journal, checksum);
    uint32_t h = 2166136261U;
    for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 16777619U; }
    return h;
}

static void journal_file(pid_t pid, char *buf, size_t cap)
{
    char name[64];
    snprintf(name, sizeof name, "journal/jnl_%d.dat", (int)pid);
    db_path(name, buf, cap);
}

static void sync_journal_dir(void)
{
    /* fsync() on the directory makes the create/unlink of the journal durable */
    char dir[PATH_MAX];
    db_path("journal", dir, sizeof dir);
    int fd = open(dir, O_RDONLY | O_DIRECTORY);
    if (fd >= 0) { fsync(fd); close(fd); }
}

void journal_begin(Journal *j, const char *description)
{
    memset(j, 0, sizeof *j);
    j->magic = JOURNAL_MAGIC;
    j->pid = (int32_t)getpid();
    j->created_at = (int64_t)time(NULL);
    str_copy(j->description, description, sizeof j->description);
}

int journal_add(Journal *j, Table t, int32_t id, const void *before, const void *after)
{
    size_t sz = db_recsize(t);
    if (j->n >= JOURNAL_MAX_ENTRIES || sz > JOURNAL_IMAGE_MAX) return -1;
    JournalEntry *e = &j->e[j->n++];
    e->table = t;
    e->id = id;
    e->size = (int32_t)sz;
    memcpy(e->before, before, sz);
    memcpy(e->after, after, sz);
    return 0;
}

int journal_commit(Journal *j)
{
    char path[PATH_MAX];
    journal_file(getpid(), path, sizeof path);
    j->end_magic = JOURNAL_MAGIC;
    j->checksum = checksum(j);

    /* Step 3: write-ahead - the journal reaches the disk BEFORE any data file changes */
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) { perror("journal open"); return -1; }
    const char *p = (const char *)j;
    size_t left = sizeof *j;
    while (left > 0) {
        ssize_t w = write(fd, p, left);
        if (w < 0) { if (errno == EINTR) continue; close(fd); unlink(path); return -1; }
        p += w;
        left -= (size_t)w;
    }
    if (fsync(fd) == -1) { close(fd); unlink(path); return -1; }
    close(fd);
    sync_journal_dir();

    /* Step 4: apply the after images to the real files */
    int touched[T_COUNT] = {0};
    for (int i = 0; i < j->n; i++) {
        JournalEntry *e = &j->e[i];
        if (db_write_raw((Table)e->table, e->id, e->after) == -1) {
            /* undo what we already applied, then drop the journal */
            fprintf(stderr, "journal: write failed, rolling the operation back\n");
            for (int k = i; k >= 0; k--)
                db_write_raw((Table)j->e[k].table, j->e[k].id, j->e[k].before);
            for (int t = 0; t < T_COUNT; t++) db_sync((Table)t);
            unlink(path);
            sync_journal_dir();
            return -1;
        }
        touched[e->table] = 1;
        if (g_crash_test && i == 0 && strncmp(j->description, "TRANSFER", 8) == 0) {
            /* --crash-test: die half-way through a transfer (debit written,
             * credit not written) to demonstrate crash recovery */
            const char msg[] = "\n*** --crash-test: killing this server process in the MIDDLE of a transfer ***\n";
            if (write(STDERR_FILENO, msg, sizeof msg - 1) < 0) { /* ignore */ }
            raise(SIGKILL);
        }
    }
    for (int t = 0; t < T_COUNT; t++)
        if (touched[t]) db_sync((Table)t);

    /* Step 5: COMMIT = journal removed */
    unlink(path);
    sync_journal_dir();
    return 0;
}

static int parse_journal_pid(const char *name, pid_t *pid)
{
    int v;
    if (strncmp(name, "jnl_", 4) != 0) return -1;
    if (sscanf(name + 4, "%d.dat", &v) != 1) return -1;
    *pid = (pid_t)v;
    return 0;
}

static int process_alive(pid_t pid)
{
    /* kill(pid, 0) sends no signal; it only checks that the process exists */
    return kill(pid, 0) == 0 || errno == EPERM;
}

int journal_orphans_exist(void)
{
    char dir[PATH_MAX];
    db_path("journal", dir, sizeof dir);
    DIR *d = opendir(dir);
    if (!d) return 0;
    struct dirent *de;
    int found = 0;
    while ((de = readdir(d)) != NULL) {
        pid_t pid;
        if (parse_journal_pid(de->d_name, &pid) == -1) continue;
        if (pid == getpid()) continue;
        if (!process_alive(pid)) { found = 1; break; }
    }
    closedir(d);
    return found;
}

int journal_recover(int at_startup)
{
    char dir[PATH_MAX];
    db_path("journal", dir, sizeof dir);

    gate_exclusive();         /* wait until no operation is in progress */

    DIR *d = opendir(dir);
    if (!d) { gate_release(); return 0; }
    int recovered = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        pid_t pid;
        if (parse_journal_pid(de->d_name, &pid) == -1) continue;
        if (!at_startup && (pid == getpid() || process_alive(pid))) continue;

        char path[PATH_MAX + 512];
        snprintf(path, sizeof path, "%s/%s", dir, de->d_name);
        Journal j;
        int fd = open(path, O_RDONLY);
        if (fd < 0) continue;
        ssize_t r = read(fd, &j, sizeof j);
        close(fd);

        char msg[256];
        if (r != (ssize_t)sizeof j || j.magic != JOURNAL_MAGIC ||
            j.end_magic != JOURNAL_MAGIC || j.checksum != checksum(&j) ||
            j.n < 0 || j.n > JOURNAL_MAX_ENTRIES) {
            /* torn journal: the crash happened while writing the journal itself,
             * so no data file was touched yet - just discard it */
            snprintf(msg, sizeof msg,
                     "RECOVERY: discarded incomplete journal of pid %d (no data was changed)", (int)pid);
        } else {
            int touched[T_COUNT] = {0};
            for (int i = j.n - 1; i >= 0; i--) {          /* undo in reverse order */
                JournalEntry *e = &j.e[i];
                if (e->table < 0 || e->table >= T_COUNT) continue;
                db_write_raw((Table)e->table, e->id, e->before);
                touched[e->table] = 1;
            }
            for (int t = 0; t < T_COUNT; t++)
                if (touched[t]) db_sync((Table)t);
            snprintf(msg, sizeof msg,
                     "RECOVERY: rolled back interrupted operation '%s' of crashed pid %d",
                     j.description, (int)pid);
            recovered++;
        }
        unlink(path);
        fprintf(stderr, "[recovery] %s\n", msg);
        if (journal_recovery_hook) journal_recovery_hook(msg);
    }
    closedir(d);
    sync_journal_dir();
    gate_release();
    return recovered;
}
