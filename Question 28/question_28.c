#include <stdio.h>
#include <unistd.h>

int main(void)
{
    /* Print the process ID so that its scheduling priority can be inspected */
    printf("PID = %d\n", getpid());
    fflush(stdout);

    /* Keep the process running so that its priority can be checked */
    while (1)
    {
    }

    return 0;
}

/*
 * Test:
 * gcc question_28.c -o question_28
 *
 * Run with the default nice value:
 * ./question_28 &
 *
 * Check the process priority:
 * ps -o pid,ni,pri,comm -p <PID>
 *
 * Expected:
 * NI should normally be 0.
 *
 * Run with a modified nice value:
 * nice -n 10 ./question_28 &
 *
 * Check again:
 * ps -o pid,ni,pri,comm -p <PID>
 *
 * Expected:
 * NI should be 10.
 *
 * To change the priority of an already-running process:
 * renice 10 -p <PID>
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_28.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_28.c -o question_28
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_28 &
PID = 7324
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ps -o pid,ni,pri,comm -p 7324
    PID  NI PRI COMMAND
   7324   0  19 question_28
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ kill 7324
[4]   Terminated                 ./question_28
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ nice -n 10 ./question_28 &
[4] 7361
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ PID = 7361
ps -o pid,ni,pri,comm -p 7361
    PID  NI PRI COMMAND
   7361  10   9 question_28
*/