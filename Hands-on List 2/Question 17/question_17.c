#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>

#define SHM_SIZE 1024

int main(void) {
    /* Create a fresh shared memory segment. */
    int shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    printf("Shared memory created. ID: %d\n", shmid);

    char *writable = (char *)shmat(shmid, NULL, 0);
    if (writable == (void *)-1) {
        perror("shmat writable");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    /* Write data through a read/write attachment. */
    strcpy(writable, "Hello from shared memory!");
    printf("Data written: %s\n", writable);

    if (shmdt(writable) == -1) {
        perror("shmdt writable");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }
    printf("Writable attachment detached.\n");

    /* Attach the same segment with read-only access. */
    char *readonly = (char *)shmat(shmid, NULL, SHM_RDONLY);
    if (readonly == (void *)-1) {
        perror("shmat read-only");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    printf("Read-only attachment successful.\n");
    printf("Data read: %s\n", readonly);

    /* Isolate the invalid write in a child process. */
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        shmdt(readonly);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    if (pid == 0) {
        volatile char *p = (volatile char *)readonly;
        p[0] = 'X';
        _exit(0);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        shmdt(readonly);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    if (WIFSIGNALED(status) &&
        (WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS)) {
        printf("Overwrite failed: read-only memory cannot be written.\n");
    } else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("Unexpected: overwrite succeeded.\n");
    } else {
        printf("Child terminated unexpectedly; signal/status indicates failure.\n");
    }

    /* Detach this process and remove the shared memory segment. */
    if (shmdt(readonly) == -1) {
        perror("shmdt read-only");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }
    printf("Read-only attachment detached.\n");

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
        return 1;
    }
    printf("Shared memory marked for removal.\n");

    return 0;
}

/*
Test:
    gcc question_17.c -o question_17
    ./question_17
    ipcs -m
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_17.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_17.c -o question_17
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_17
Shared memory created. ID: 2
Data written: Hello from shared memory!
Writable attachment detached.
Read-only attachment successful.
Data read: Hello from shared memory!
Overwrite failed: read-only memory cannot be written.
Read-only attachment detached.
Shared memory marked for removal.
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -m

------ Shared Memory Segments --------
key        shmid      owner      perms      bytes      nattch     status      

*/