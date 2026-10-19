#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>

#define RESOURCE_COUNT 2
#define CHILDREN 4

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static int semaphore_op(int semid, short operation) {
    struct sembuf op = {0, operation, 0};

    while (semop(semid, &op, 1) == -1) {
        if (errno == EINTR)
            continue;
        return -1;
    }
    return 0;
}

int main(void) {
    setbuf(stdout, NULL);

    /* Create and initialize a counting semaphore. */
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    union semun arg;
    arg.val = RESOURCE_COUNT;

    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        semctl(semid, 0, IPC_RMID);
        return 1;
    }

    printf("Counting semaphore ID: %d\n", semid);
    printf("Available pseudo-resources: %d\n", RESOURCE_COUNT);

    for (int i = 0; i < CHILDREN; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            printf("PID %ld waiting for a resource.\n", (long)getpid());

            if (semaphore_op(semid, -1) == -1) {
                perror("semop acquire");
                _exit(1);
            }

            printf("PID %ld acquired a resource.\n", (long)getpid());
            sleep(2);
            printf("PID %ld released a resource.\n", (long)getpid());

            if (semaphore_op(semid, 1) == -1) {
                perror("semop release");
                _exit(1);
            }

            _exit(0);
        }
    }

    int failed = 0;
    for (int i = 0; i < CHILDREN; i++) {
        int status;
        if (wait(&status) == -1) {
            perror("wait");
            failed = 1;
        } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            failed = 1;
        }
    }

    int value = semctl(semid, 0, GETVAL);
    if (value == -1) {
        perror("semctl GETVAL");
        failed = 1;
    } else {
        printf("Available resources after all processes finish: %d\n",
               value);
    }

    printf("Semaphore retained for Q19(d).\n");
    return failed ? 1 : 0;
}

/*
Test:
    gcc question_19_c.c -o question_19_c
    ./question_19_c
    ipcs -s
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_19_c.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_19_c.c -o question_19_c
./question_19_c
Counting semaphore ID: 4
Available pseudo-resources: 2
PID 9783 waiting for a resource.
PID 9783 acquired a resource.
PID 9784 waiting for a resource.
PID 9784 acquired a resource.
PID 9782 waiting for a resource.
PID 9785 waiting for a resource.
PID 9783 released a resource.
PID 9784 released a resource.
PID 9782 acquired a resource.
PID 9785 acquired a resource.
PID 9785 released a resource.
PID 9782 released a resource.
Available resources after all processes finish: 2
Semaphore retained for Q19(d).
*/