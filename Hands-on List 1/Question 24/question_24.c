#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(void)
{
    pid_t children[3];
    int status;

    /* Create three child processes */
    for (int i = 0; i < 3; i++)
    {
        children[i] = fork();

        if (children[i] == -1)
        {
            perror("fork");
            return 1;
        }

        /* Each child performs its work and then terminates */
        if (children[i] == 0)
        {
            printf("Child %d: PID = %d, PPID = %d\n",
                   i + 1, getpid(), getppid());

            sleep((i + 1) * 2);

            printf("Child %d exiting...\n", i + 1);

            exit(10 + i);
        }
    }

    printf("Parent: PID = %d\n", getpid());
    printf("Waiting specifically for Child 2: PID = %d\n",
           children[1]);

    /* Wait specifically for the second child */
    if (waitpid(children[1], &status, 0) == -1)
    {
        perror("waitpid");
        return 1;
    }

    /* Check how the selected child terminated */
    if (WIFEXITED(status))
    {
        printf("Child 2 terminated with exit status %d\n",
               WEXITSTATUS(status));
    }

    printf("Parent: waitpid() completed.\n");

    return 0;
}

/*
 * Test:
 * gcc question_24.c -o question_24
 * ./question_24
 *
 * The program creates three child processes.
 *
 * The parent specifically waits for Child 2 using:
 * waitpid(children[1], &status, 0)
 *
 * Child 1 exits after 2 seconds.
 * Child 2 exits after 4 seconds.
 * Child 3 exits after 6 seconds.
 *
 * Expected:
 * Child 2 terminated with exit status 11
 *
 * The exact order of output may vary because the processes
 * are scheduled independently.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_24.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_24.c -o question_24
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_24
Child 1: PID = 7026, PPID = 7025
Parent: PID = 7025
Waiting specifically for Child 2: PID = 7027
Child 2: PID = 7027, PPID = 7025
Child 3: PID = 7028, PPID = 7025
Child 1 exiting...
Child 2 exiting...
Child 2 terminated with exit status 11
Parent: waitpid() completed.
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ Child 3 exiting...
*/