#include <stdio.h>
#include <unistd.h>
#include <sched.h>

void print_policy(int policy)
{
    /* Convert the policy constant into a readable name */
    if (policy == SCHED_OTHER)
        printf("SCHED_OTHER\n");
    else if (policy == SCHED_FIFO)
        printf("SCHED_FIFO\n");
    else if (policy == SCHED_RR)
        printf("SCHED_RR\n");
    else
        printf("Unknown policy\n");
}

int main(void)
{
    int policy;
    struct sched_param param;

    /* Obtain the current scheduling policy of this process */
    policy = sched_getscheduler(0);

    if (policy == -1)
    {
        perror("sched_getscheduler");
        return 1;
    }

    printf("Current scheduling policy: ");
    print_policy(policy);

    /* Set the real-time priority required by SCHED_FIFO */
    param.sched_priority = 10;

    /* Change the current process to the SCHED_FIFO policy */
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1)
    {
        perror("sched_setscheduler");
        return 1;
    }

    /* Obtain the scheduling policy after modification */
    policy = sched_getscheduler(0);

    if (policy == -1)
    {
        perror("sched_getscheduler");
        return 1;
    }

    printf("New scheduling policy: ");
    print_policy(policy);

    return 0;
}

/*
 * Test:
 * gcc question_29.c -o question_29
 *
 * Run normally:
 * ./question_29
 *
 * Expected on an unprivileged Linux account:
 * Current scheduling policy: SCHED_OTHER
 * sched_setscheduler: Operation not permitted
 *
 * To attempt the real-time policy change with elevated privileges:
 * sudo ./question_29
 *
 * If permitted, expected:
 * Current scheduling policy: SCHED_OTHER
 * New scheduling policy: SCHED_FIFO
 *
 * To test SCHED_RR, replace SCHED_FIFO in sched_setscheduler()
 * with SCHED_RR and run the program again.
 *
 * The policy can also be checked independently using:
 * chrt -p <PID>
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_29.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_29.c -o question_29
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_29
Current scheduling policy: SCHED_OTHER
sched_setscheduler: Operation not permitted
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ sudo ./question_29
[sudo: authenticate] Password:                
Current scheduling policy: SCHED_OTHER
New scheduling policy: SCHED_FIFO
*/