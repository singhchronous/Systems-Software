#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(void)
{
    pid_t pid;

    /* Create a new child process */
    pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return 1;
    }

    /* fork() returns 0 in the child process */
    if (pid == 0)
    {
        printf("Child process: PID = %d, Parent PID = %d\n",
               getpid(), getppid());
    }
    else
    {
        /* fork() returns the child PID in the parent process */
        printf("Parent process: PID = %d, Child PID = %d\n",
               getpid(), pid);
    }

    return 0;
}

/*
 * Test:
 * gcc question_20.c -o question_20
 * ./question_20
 *
 * Expected output:
 * Parent process: PID = <parent_pid>, Child PID = <child_pid>
 * Child process: PID = <child_pid>, Parent PID = <parent_pid>
 *
 * The order of the two lines may vary because the scheduler
 * determines which process executes first after fork().
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_20.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_20.c -o question_20
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_20
Parent process: PID = 6702, Child PID = 6703
Child process: PID = 6703, Parent PID = 6702
*/