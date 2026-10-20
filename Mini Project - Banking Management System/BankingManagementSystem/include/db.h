/*
 * db.h - File-based "database" built only on system calls:
 *        open / read / write / lseek / fstat / fsync / close + fcntl locks.
 *
 * Each table is one binary file of fixed-size records (records.h).
 *
 * IMPORTANT RULES (explained in docs/DESIGN.md):
 *  1. Every process opens its OWN descriptors (db_open_all() after fork()).
 *     Descriptors inherited across fork() share one file offset, so lseek()+read()
 *     from two processes would race on the offset.
 *  2. fcntl locks are per process and are all dropped when the process closes
 *     ANY descriptor of that file - so each file is opened exactly once.
 *  3. Lock order (prevents deadlock):  gate -> employees -> loans -> customers
 *     (ascending id) -> transactions -> feedback.
 */
#ifndef DB_H
#define DB_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef enum {
    T_CUSTOMERS = 0,
    T_EMPLOYEES,
    T_ADMINS,
    T_LOANS,
    T_TXNS,
    T_FEEDBACK,
    T_COUNT
} Table;

/* A 1-byte region far beyond any real record, used as the "append lock" so
 * that two processes appending at the same moment get different record slots. */
#define APPEND_LOCK_OFFSET ((off_t)1 << 40)

/* ---- setup ---- */
int  db_init_dir(const char *dir);          /* remember + create data dir and journal dir */
const char *db_data_dir(void);
void db_path(const char *name, char *buf, size_t cap);
int  db_open_all(void);                     /* open every table in THIS process */
void db_close_all(void);
int  db_files_exist(void);

/* ---- table metadata ---- */
size_t  db_recsize(Table t);
int32_t db_idbase(Table t);
const char *db_table_file(Table t);
int     db_fd(Table t);
off_t   db_offset(Table t, int32_t id);
int     db_count(Table t);                  /* number of records in the file */
int     db_valid_id(Table t, int32_t id);   /* 1 if a record with this id exists */

/* ---- raw access: NO locking - caller must already hold the right lock ---- */
int db_read_raw(Table t, int32_t id, void *rec);
int db_write_raw(Table t, int32_t id, const void *rec);
int db_sync(Table t);

/* ---- record / table locks ---- */
int db_lock(Table t, int32_t id, short type);   /* F_RDLCK / F_WRLCK on one record */
int db_unlock(Table t, int32_t id);
int db_lock_table(Table t, short type);         /* whole table (all records) */
int db_unlock_table(Table t);

/* Append a new record: assigns the next id (UML: get_next_id) into the first
 * int32 field of rec, writes it and fsyncs. Returns 0 on success. */
int db_append(Table t, void *rec, int32_t *new_id);

/* ---- the "gate": a shared/exclusive lock that every operation enters in
 *      SHARED mode; crash recovery enters it in EXCLUSIVE mode so it never
 *      runs in the middle of somebody else's operation. ---- */
int gate_shared(void);
int gate_exclusive(void);
int gate_release(void);

/* ---- guarded helpers (gate + record lock + crash-recovery check) ----
 * Never call these while already inside another guarded operation. */
int db_get(Table t, int32_t id, void *rec);      /* 0 ok, -1 no such record */

/* Callback edits *rec in place. Return 0 = write it, 1 = nothing to write,
 * -1 = refuse (put the reason in err). */
typedef int (*db_update_fn)(void *rec, void *ctx, char *err, size_t errcap);
/* Returns 0 ok, -1 no such record, -2 callback refused, -3 I/O error. */
int db_update(Table t, int32_t id, db_update_fn fn, void *ctx, char *err, size_t errcap);

/* Read-only scan of every record under a shared table lock.
 * Callback returns 0 to continue, 1 to stop. */
typedef int (*db_scan_fn)(const void *rec, void *ctx);
int db_scan(Table t, db_scan_fn fn, void *ctx);

/* Scan with an exclusive table lock. Callback returns 1 if it modified the
 * record (it is then written back). Returns number of records modified. */
typedef int (*db_scan_update_fn)(void *rec, void *ctx);
int db_scan_update(Table t, db_scan_update_fn fn, void *ctx);

/* Sleep g_race_delay_us microseconds (race demo only). */
void race_window(void);

#endif
