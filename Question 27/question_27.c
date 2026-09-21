#include <stdio.h>
#include <sched.h>

int main(void)
{
    int min_priority;
    int max_priority;

    /* Get the minimum real-time priority for SCHED_FIFO */
    min_priority = sched_get_priority_min(SCHED_FIFO);

    if (min_priority == -1)
    {
        perror("sched_get_priority_min");
        return 1;
    }

    /* Get the maximum real-time priority for SCHED_FIFO */
    max_priority = sched_get_priority_max(SCHED_FIFO);

    if (max_priority == -1)
    {
        perror("sched_get_priority_max");
        return 1;
    }

    printf("SCHED_FIFO minimum priority: %d\n", min_priority);
    printf("SCHED_FIFO maximum priority: %d\n", max_priority);

    return 0;
}

/*
 * Test:
 * gcc question_27.c -o question_27
 * ./question_27
 *
 * Typical output on Linux:
 * SCHED_FIFO minimum priority: 1
 * SCHED_FIFO maximum priority: 99
 *
 * The program retrieves the priority range supported by the
 * SCHED_FIFO real-time scheduling policy.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_27.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_27.c -o question_27
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ 
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_27
SCHED_FIFO minimum priority: 1
SCHED_FIFO maximum priority: 99
*/