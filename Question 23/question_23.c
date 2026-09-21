#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
    pid_t pid;

    /* Create a child process */
    pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return 1;
    }

    /* Keep the child alive after its parent exits */
    if (pid == 0)
    {
        printf("Child: PID = %d, Initial PPID = %d\n",
               getpid(), getppid());
        fflush(stdout);

        /* Give the parent time to exit and become an orphan */
        sleep(5);

        /* Display the new parent after reparenting */
        printf("Child: PID = %d, New PPID = %d\n",
               getpid(), getppid());
        fflush(stdout);

        sleep(20);
    }
    else
    {
        /* Parent exits while the child is still running */
        printf("Parent: PID = %d\n", getpid());
        printf("Parent exiting...\n");
        fflush(stdout);

        exit(0);
    }

    return 0;
}

/*
 * Test:
 * gcc question_23.c -o question_23
 * ./question_23
 *
 * Expected:
 * Parent: PID = <parent_pid>
 * Parent exiting...
 * Child: PID = <child_pid>, Initial PPID = <parent_pid>
 *
 * After the parent exits, the child should display a new PPID,
 * commonly 1 when PID 1 is the child-reaper in the current
 * PID namespace.
 *
 * The child can also be checked using:
 * ps -o pid,ppid,stat,comm -p <child_pid>
 *
 * Or:
 * cat /proc/<child_pid>/status | grep PPid
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_23.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_23.c -o question_23
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_23
Parent: PID = 6977
Parent exiting...
Child: PID = 6978, Initial PPID = 6977
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ Child: PID = 6978, New PPID = 2754
*/