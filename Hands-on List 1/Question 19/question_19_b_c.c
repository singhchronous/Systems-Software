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
 * To stop the process:
 * kill -STOP <PID>
 *
 * Expected state:
 * State:  T (stopped)
 *
 * To continue:
 * kill -CONT <PID>
 *
 * To terminate:
 * kill <PID>
 *
 * For the sleeping state, replace the infinite loop with:
 * while (1)
 * {
 *     sleep(10);
 * }
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_19_b.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_19_b.c -o question_19_b
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_19_b &
PID = 6562
Running...
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ps -o pid,stat,comm -p 6562
    PID STAT COMMAND
   6562 S    question_19_b
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ Running...
Running...
cat /proc/6562Running...grep State

State:	S (sleeping)
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ Running... kill -STOP 6562

[3]+  Stopped                    ./question_19_b
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ps -o pid,stat,comm -p 6562
    PID STAT COMMAND
   6562 T    question_19_b
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat /proc/6562/status | grep State
State:	T (stopped)
*/