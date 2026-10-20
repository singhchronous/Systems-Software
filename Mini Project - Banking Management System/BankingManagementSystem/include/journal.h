/*
 * journal.h - Write-Ahead (undo) Journal: gives Atomicity + Durability.
 *
 * Every operation that changes more than one record (deposit, withdraw,
 * transfer, loan approval, account opening) works like this:
 *
 *   1. lock all records involved (fixed order -> no deadlock)
 *   2. compute the new versions ("after images") of every record,
 *      keeping the old versions ("before images")
 *   3. write before+after images to  data/journal/jnl_<pid>.dat  and fsync()
 *      -> from now on the change can always be undone
 *   4. write the after images into the real files and fsync() them
 *   5. delete the journal file (this is the COMMIT point) and fsync the dir
 *   6. unlock
 *
 * If the process dies anywhere in steps 3-5 the journal file is still there.
 * Recovery (at server start-up, by the server's housekeeper thread, and by any
 * process that finds an orphan journal after taking its locks) writes the
 * BEFORE images back, so the operation either happened completely or not at
 * all - never half (no money is lost or created).
 */
#ifndef JOURNAL_H
#define JOURNAL_H

#include <stdint.h>
#include "db.h"

#define JOURNAL_MAX_ENTRIES 4
#define JOURNAL_IMAGE_MAX   512
#define JOURNAL_MAGIC       0x4A524E4CU   /* "JRNL" */

typedef struct {
    int32_t table;                        /* Table */
    int32_t id;                           /* record id */
    int32_t size;                         /* bytes used in the images */
    int32_t pad;
    uint8_t before[JOURNAL_IMAGE_MAX];
    uint8_t after[JOURNAL_IMAGE_MAX];
} JournalEntry;

typedef struct {
    uint32_t magic;
    int32_t  pid;                         /* owner process */
    int32_t  n;                           /* number of entries */
    int32_t  pad;
    int64_t  created_at;
    char     description[96];
    JournalEntry e[JOURNAL_MAX_ENTRIES];
    uint32_t checksum;                    /* detects a journal torn by a crash */
    uint32_t end_magic;
} Journal;

/* Test hook (server flag --crash-test): the process kills itself with SIGKILL
 * after writing the FIRST after-image, i.e. in the middle of the operation. */
extern int g_crash_test;

void journal_begin(Journal *j, const char *description);
int  journal_add(Journal *j, Table t, int32_t id, const void *before, const void *after);

/* Steps 3-5 above. Returns 0 on success. */
int  journal_commit(Journal *j);

/* 1 if a journal exists whose owner process is dead (needs recovery). */
int  journal_orphans_exist(void);

/* Roll back orphan journals. at_startup=1 rolls back EVERY journal (no other
 * process can be running yet). Takes the gate exclusively.
 * Returns the number of operations rolled back. */
int  journal_recover(int at_startup);

/* Optional callback so the server can write recoveries to the audit log. */
extern void (*journal_recovery_hook)(const char *msg);

#endif
