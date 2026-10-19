#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>

#define CHILDREN 3
#define TICKETS_PER_CHILD 5

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

    int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    int *counter = (int *)shmat(shmid, NULL, 0);
    if (counter == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }
    *counter = 0;

    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("semget");
        shmdt(counter);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    union semun arg;
    arg.val = 1;

    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        shmdt(counter);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        return 1;
    }

    printf("Semaphore ID: %d\n", semid);
    printf("Generating tickets using three processes...\n");

    for (int i = 0; i < CHILDREN; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            shmdt(counter);
            shmctl(shmid, IPC_RMID, NULL);
            return 1;
        }

        if (pid == 0) {
            for (int j = 0; j < TICKETS_PER_CHILD; j++) {
                if (semaphore_op(semid, -1) == -1) {
                    perror("semop acquire");
                    _exit(1);
                }

                int ticket = ++(*counter);
                printf("PID %ld issued ticket %d\n",
                       (long)getpid(), ticket);

                if (semaphore_op(semid, 1) == -1) {
                    perror("semop release");
                    _exit(1);
                }
            }

            shmdt(counter);
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

    printf("Final ticket counter: %d\n", *counter);

    shmdt(counter);
    shmctl(shmid, IPC_RMID, NULL);

    if (failed)
        return 1;

    printf("Shared memory removed. Semaphore retained for Q19(d).\n");
    return 0;
}

/*
Test:
    gcc question_19_a.c -o question_19_a
    ./question_19_a
    ipcs -s
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_19_a.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_19_a.c -o question_19_a
./question_19_a
Semaphore ID: 2
Generating tickets using three processes...
PID 9679 issued ticket 1
PID 9679 issued ticket 2
PID 9679 issued ticket 3
PID 9679 issued ticket 4
PID 9678 issued ticket 5
PID 9679 issued ticket 6
PID 9680 issued ticket 7
PID 9678 issued ticket 8
PID 9680 issued ticket 9
PID 9678 issued ticket 10
PID 9680 issued ticket 11
PID 9678 issued ticket 12
PID 9680 issued ticket 13
PID 9678 issued ticket 14
PID 9680 issued ticket 15
Final ticket counter: 15
Shared memory removed. Semaphore retained for Q19(d).
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -s

------ Semaphore Arrays --------
key        semid      owner      perms      nsems     
0x00000000 0          shubham-si 600        1         
0x00000000 1          shubham-si 600        1         
0x00000000 2          shubham-si 600        1         
*/