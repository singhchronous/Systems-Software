#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

#define INITIAL_VALUE 5

int main(void) {
    /* Create a set containing one semaphore. */
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    /* Initialize the counting semaphore to 5. */
    union semun arg;
    arg.val = INITIAL_VALUE;

    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        semctl(semid, 0, IPC_RMID);
        return 1;
    }

    int value = semctl(semid, 0, GETVAL);
    if (value == -1) {
        perror("semctl GETVAL");
        semctl(semid, 0, IPC_RMID);
        return 1;
    }

    printf("Counting semaphore created successfully.\n");
    printf("Semaphore ID: %d\n", semid);
    printf("Initial value: %d\n", value);

    return 0;
}

/*
Test:
    gcc question_18_b.c -o question_18_b
    ./question_18_b
    ipcs -s
    ipcs -s -i <semid>
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_18_b.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_18_b.c -o question_18_b
./question_18_b
Counting semaphore created successfully.
Semaphore ID: 1
Initial value: 5
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -s

------ Semaphore Arrays --------
key        semid      owner      perms      nsems     
0x00000000 0          shubham-si 600        1         
0x00000000 1          shubham-si 600        1         

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -s -i 1

Semaphore Array semid=1
uid=1000	 gid=1000	 cuid=1000	 cgid=1000
mode=0600, access_perms=0600
nsems = 1
otime = Not set                   
ctime = Tue Oct 20 00:11:35 2026  
semnum     value      ncount     zcount     pid       
0          5          0          0          9597      
*/