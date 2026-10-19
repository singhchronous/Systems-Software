#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <semid>\n", argv[0]);
        return 1;
    }

    char *end;
    errno = 0;
    long value = strtol(argv[1], &end, 10);

    if (errno != 0 || *argv[1] == '\0' || *end != '\0' ||
        value < 0 || value > INT_MAX) {
        fprintf(stderr, "Invalid semaphore ID.\n");
        return 1;
    }

    int semid = (int)value;

    /* Remove the specified semaphore set. */
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        return 1;
    }

    printf("Semaphore set %d removed successfully.\n", semid);
    return 0;
}

/*
Test:
    gcc question_19_d.c -o question_19_d
    ipcs -s
    ./question_19_d <semid>
    ipcs -s
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_19_d.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_19_d.c -o question_19_d
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -s

------ Semaphore Arrays --------
key        semid      owner      perms      nsems     
0x00000000 0          shubham-si 600        1         
0x00000000 1          shubham-si 600        1         
0x00000000 2          shubham-si 600        1         
0x00000000 3          shubham-si 600        1         
0x00000000 4          shubham-si 600        1         

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_19_d 4
Semaphore set 4 removed successfully.
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -s

------ Semaphore Arrays --------
key        semid      owner      perms      nsems     
0x00000000 0          shubham-si 600        1         
0x00000000 1          shubham-si 600        1         
0x00000000 2          shubham-si 600        1         
0x00000000 3          shubham-si 600        1         
*/
