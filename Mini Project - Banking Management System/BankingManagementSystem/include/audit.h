/*
 * audit.h - Audit logging through a System V MESSAGE QUEUE to a separate
 *           DAEMON process (slides 75-76, 91-95).
 *
 * Server processes never write the log file themselves: they msgsnd() a
 * message and continue immediately. The audit daemon (a session leader with no
 * controlling terminal) msgrcv()s the messages one by one and appends them to
 * logs/audit.log, so log lines from many concurrent processes never interleave.
 */
#ifndef AUDIT_H
#define AUDIT_H

#include <sys/types.h>

#define AUDIT_MSG_LOG  1L
#define AUDIT_MSG_STOP 2L
#define AUDIT_TEXT_LEN 400

int   audit_init(void);                       /* create/get the message queue */
pid_t audit_start_daemon(const char *log_path);
void  audit_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void  audit_stop(pid_t daemon_pid);           /* send STOP, wait, remove queue */

#endif
