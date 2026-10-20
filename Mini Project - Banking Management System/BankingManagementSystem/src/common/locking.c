/*
 * locking.c - fcntl() advisory record locking.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "locking.h"

int g_locking_enabled = 1;
int g_race_delay_us = 0;

static int do_lock(int fd, short type, off_t start, off_t len)
{
    struct flock fl;
    memset(&fl, 0, sizeof fl);
    fl.l_type = type;          /* F_RDLCK, F_WRLCK or F_UNLCK */
    fl.l_whence = SEEK_SET;
    fl.l_start = start;
    fl.l_len = len;
    fl.l_pid = 0;

    for (;;) {
        if (fcntl(fd, type == F_UNLCK ? F_SETLK : F_SETLKW, &fl) == 0)
            return 0;
        if (errno == EINTR)
            continue;          /* a signal (SIGALRM, SIGCHLD...) interrupted the wait: retry */
        if (errno == EDEADLK) {
            /* The kernel detected that waiting would deadlock. Our global lock
             * ordering (see DESIGN.md) makes this impossible in practice; if it
             * ever happens the caller releases everything and reports an error. */
            fprintf(stderr, "[pid %d] fcntl: deadlock detected by kernel\n", (int)getpid());
            return -1;
        }
        perror("fcntl lock");
        return -1;
    }
}

int lock_range(int fd, short type, off_t start, off_t len)
{
    if (!g_locking_enabled) return 0;
    return do_lock(fd, type, start, len);
}

int unlock_range(int fd, off_t start, off_t len)
{
    if (!g_locking_enabled) return 0;
    return do_lock(fd, F_UNLCK, start, len);
}

int range_is_locked(int fd, short type, off_t start, off_t len, pid_t *holder)
{
    struct flock fl;
    memset(&fl, 0, sizeof fl);
    fl.l_type = type;
    fl.l_whence = SEEK_SET;
    fl.l_start = start;
    fl.l_len = len;
    if (fcntl(fd, F_GETLK, &fl) == -1) return 0;
    if (fl.l_type == F_UNLCK) return 0;
    if (holder) *holder = fl.l_pid;
    return 1;
}
