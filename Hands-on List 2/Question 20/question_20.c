#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static int semaphore_op(int semid, unsigned short sem_num, short operation) {
    struct sembuf op = {sem_num, operation, 0};

    while (semop(semid, &op, 1) == -1) {
        if (errno == EINTR)
            continue;
        return -1;
    }
    return 0;
}

static int write_byte(int fd, char byte) {
    for (;;) {
        ssize_t n = write(fd, &byte, 1);
        if (n == 1)
            return 0;
        if (n == -1 && errno == EINTR)
            continue;
        return -1;
    }
}

static int read_byte(int fd, char *byte) {
    for (;;) {
        ssize_t n = read(fd, byte, 1);
        if (n == 1)
            return 0;
        if (n == -1 && errno == EINTR)
            continue;
        if (n == 0)
            errno = EPIPE;
        return -1;
    }
}

int main(void) {
    setbuf(stdout, NULL);

    int ready[2], go[2];

    if (pipe(ready) == -1 || pipe(go) == -1) {
        perror("pipe");
        return 1;
    }

    /* Create two semaphores, both initialized to 1. */
    int semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    unsigned short values[2] = {1, 1};
    union semun arg;
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1) {
        perror("semctl SETALL");
        semctl(semid, 0, IPC_RMID);
        return 1;
    }

    printf("Created S1 and S2, both initialized to 1.\n");
    printf("Starting two processes with opposite acquisition orders.\n");

    pid_t children[2];

    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            semctl(semid, 0, IPC_RMID);
            return 1;
        }

        if (pid == 0) {
            close(ready[0]);
            close(go[1]);

            unsigned short first = (unsigned short)i;
            unsigned short second = (unsigned short)(1 - i);

            printf("P%d: attempting to acquire S%d.\n",
                   i + 1, first + 1);

            if (semaphore_op(semid, first, -1) == -1) {
                perror("first semop");
                _exit(1);
            }

            printf("P%d: acquired S%d.\n", i + 1, first + 1);

            if (write_byte(ready[1], 'R') == -1) {
                perror("ready write");
                _exit(1);
            }

            char signal;
            if (read_byte(go[0], &signal) == -1) {
                perror("go read");
                _exit(1);
            }

            printf("P%d: attempting to acquire S%d.\n",
                   i + 1, second + 1);

            if (semaphore_op(semid, second, -1) == -1) {
                int error = errno;
                if (error == EIDRM) {
                    printf("P%d: semaphore set removed; deadlock released.\n",
                           i + 1);
                    _exit(0);
                }
                errno = error;
                perror("second semop");
                _exit(1);
            }

            printf("P%d: acquired both semaphores.\n", i + 1);
            _exit(0);
        }

        children[i] = pid;
    }

    close(ready[1]);
    close(go[0]);

    char byte;
    for (int i = 0; i < 2; i++) {
        if (read_byte(ready[0], &byte) == -1) {
            perror("ready read");
            semctl(semid, 0, IPC_RMID);
            return 1;
        }
    }

    printf("Both processes hold their first semaphore.\n");

    if (write_byte(go[1], 'G') == -1 ||
        write_byte(go[1], 'G') == -1) {
        perror("go write");
        semctl(semid, 0, IPC_RMID);
        return 1;
    }

    close(ready[0]);
    close(go[1]);

    printf("Both processes will now wait for the other's semaphore.\n");
    printf("Deadlock should persist for 5 seconds.\n");
    sleep(5);

    /* Remove the set to wake the blocked semop() calls. */
    printf("Removing semaphore set to release blocked processes.\n");
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        return 1;
    }

    int failed = 0;
    for (int i = 0; i < 2; i++) {
        int status;
        if (waitpid(children[i], &status, 0) == -1) {
            perror("waitpid");
            failed = 1;
        } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            failed = 1;
        }
    }

    printf("Deadlock demonstration completed.\n");
    return failed ? 1 : 0;
}

/*
Test:
    gcc question_20.c -o question_20
    ./question_20
    ipcs -s
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_20.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_20.c -o question_20
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_20
Created S1 and S2, both initialized to 1.
Starting two processes with opposite acquisition orders.
P1: attempting to acquire S1.
P1: acquired S1.
P2: attempting to acquire S2.
P2: acquired S2.
Both processes hold their first semaphore.
Both processes will now wait for the other's semaphore.
Deadlock should persist for 5 seconds.
P2: attempting to acquire S1.
P1: attempting to acquire S2.
Removing semaphore set to release blocked processes.
P2: semaphore set removed; deadlock released.
P1: semaphore set removed; deadlock released.
Deadlock demonstration completed.
*/