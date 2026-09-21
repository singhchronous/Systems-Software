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

    /* Child terminates immediately */
    if (pid == 0)
    {
        printf("Child process: PID = %d\n", getpid());
        printf("Child exiting...\n");
        exit(0);
    }
    else
    {
        /* Keep the parent alive without calling wait() */
        printf("Parent process: PID = %d\n", getpid());
        printf("Child PID = %d\n", pid);
        printf("Parent sleeping. Check the child process state now.\n");

        sleep(30);
    }

    return 0;
}

/*
 * Test:
 * gcc question_22.c -o question_22
 * ./question_22
 *
 * While the parent is sleeping, open another terminal and run:
 *
 * ps -o pid,ppid,stat,comm -p <child_pid>
 *
 * Expected:
 * <child_pid> <parent_pid> Z question_22
 *
 * Or check /proc:
 * cat /proc/<child_pid>/status | grep State
 *
 * Expected:
 * State:  Z (zombie)
 *
 * The child has already terminated, but the parent has not called
 * wait() or waitpid(), so the child remains as a zombie temporarily.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_22.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_22.c -o question_22
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_22
Parent process: PID = 6916
Child PID = 6917
Parent sleeping. Check the child process state now.
Child process: PID = 6917
Child exiting...
[Terminal 2:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ps -o pid,ppid,stat,comm -p 6917
  PID  PPID STAT COMMAND
 6917  6916 Z    question_22
]
*/