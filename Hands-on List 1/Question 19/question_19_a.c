#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("PID = %d\n", getpid());
    fflush(stdout);

    while (1)
    {
        printf("Running...\n");
        fflush(stdout);

        sleep(10);
    }

    return 0;
}


/*
 * Test:
 * gcc question_19.c -o question_19
 * ./question_19 &
 *
 * Check the running state:
 * ps -o pid,stat,comm -p <PID>
 *
 * Expected:
 * <PID> R question_19
 *
 * Check through /proc:
 * cat /proc/<PID>/status | grep State
 *
 * Expected:
 * State:  R (running)
 *
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_19_a.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_19_a.c -o question_19_a
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_19_a
PID = 6503
Process is now running...
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ps -o pid,stat,comm -p 6503
    PID STAT COMMAND
   6503 R    question_19_a
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat /proc/6503/status | grep State
State:	R (running)

*/