
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>

#define CHILDREN 3
#define UPDATES_PER_CHILD 4

struct shared_data {
    int counter;
    char last_update[100];
};

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

    int shmid = shmget(IPC_PRIVATE, sizeof(struct shared_data),
                       IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    struct shared_data *data =
        (struct shared_data *)shmat(shmid, NULL, 0);
    if (data == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    data->counter = 0;
    strcpy(data->last_update, "No updates yet");

    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("semget");
        shmdt(data);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    union semun arg;
    arg.val = 1;

    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        shmdt(data);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        return 1;
    }

    printf("Semaphore ID: %d\n", semid);

    for (int i = 0; i < CHILDREN; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            shmdt(data);
            shmctl(shmid, IPC_RMID, NULL);
            return 1;
        }

        if (pid == 0) {
            for (int j = 0; j < UPDATES_PER_CHILD; j++) {
                if (semaphore_op(semid, -1) == -1) {
                    perror("semop acquire");
                    _exit(1);
                }

                data->counter++;
                snprintf(data->last_update, sizeof(data->last_update),
                         "PID %ld performed update %d",
                         (long)getpid(), j + 1);

                printf("Counter: %d, %s\n",
                       data->counter, data->last_update);

                if (semaphore_op(semid, 1) == -1) {
                    perror("semop release");
                    _exit(1);
                }
            }

            shmdt(data);
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

    printf("Final counter: %d\n", data->counter);
    printf("Last update: %s\n", data->last_update);

    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);

    if (failed)
        return 1;

    printf("Shared memory removed. Semaphore retained for Q19(d).\n");
    return 0;
}

/*
Test:
    gcc question_19_b.c -o question_19_b
    ./question_19_b
    ipcs -s
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_19_b.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_19_b.c -o question_19_b
./question_19_b
Semaphore ID: 3
Counter: 1, PID 9736 performed update 1
Counter: 2, PID 9736 performed update 2
Counter: 3, PID 9736 performed update 3
Counter: 4, PID 9736 performed update 4
Counter: 5, PID 9738 performed update 1
Counter: 6, PID 9738 performed update 2
Counter: 7, PID 9738 performed update 3
Counter: 8, PID 9738 performed update 4
Counter: 9, PID 9739 performed update 1
Counter: 10, PID 9739 performed update 2
Counter: 11, PID 9739 performed update 3
Counter: 12, PID 9739 performed update 4
Final counter: 12
Last update: PID 9739 performed update 4
Shared memory removed. Semaphore retained for Q19(d).
*/