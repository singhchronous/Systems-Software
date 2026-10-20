/*
 * locking.h - fcntl() record locking wrappers (slides 44-51).
 *
 *   F_RDLCK : shared lock   - many readers, no writer
 *   F_WRLCK : exclusive lock - one writer, no readers
 *   F_SETLKW: block (wait) until the lock can be granted
 *
 * Locks cover a byte range [start, start+len) of a file, so two processes can
 * update two DIFFERENT accounts in customers.dat at the same time while two
 * processes touching the SAME account are serialised.
 *
 * fcntl locks belong to the process: the kernel releases them automatically if
 * the process dies, which is why crashed children can never leave a record
 * locked forever.
 */
#ifndef LOCKING_H
#define LOCKING_H

#include <sys/types.h>

/* 1 = normal operation. 0 = locking disabled (ONLY for the race-condition demo,
 * server flag --no-lock). */
extern int g_locking_enabled;

/* Microseconds to sleep between "read balance" and "write balance" - widens
 * the race window so the demo shows lost updates clearly when locks are off. */
extern int g_race_delay_us;

/* Blocking lock of [start, start+len). type = F_RDLCK or F_WRLCK.
 * Retries if interrupted by a signal. Returns 0 on success, -1 on error. */
int lock_range(int fd, short type, off_t start, off_t len);

/* Release [start, start+len). */
int unlock_range(int fd, off_t start, off_t len);

/* Non-blocking probe: returns 1 if a conflicting lock is held by someone else
 * (F_GETLK), 0 if free. Used by the demo/diagnostics. */
int range_is_locked(int fd, short type, off_t start, off_t len, pid_t *holder);

#endif
