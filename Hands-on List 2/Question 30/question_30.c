/*
 * Question 30: Ignoring and Resetting Signals
 *
 * Demonstrates ignoring SIGINT using SIG_IGN and restoring
 * its default action using SIG_DFL.
 *
 * Test:
 *   gcc question_30.c -o question_30
 *   ./question_30
 *
 * Expected:
 *   Ctrl+C is ignored during the first 10 seconds.
 *   After SIGINT is reset to SIG_DFL, Ctrl+C terminates
 *   the program.
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main(void)
{
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    printf("SIGINT is now ignored.\n");
    printf("Press Ctrl+C during the next 10 seconds.\n");
    sleep(10);

    if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    printf("\nSIGINT reset to default action.\n");
    printf("Press Ctrl+C now to terminate the program.\n");

    while (1)
        pause();

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_30.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_30.c -o question_30
./question_30
SIGINT is now ignored.
Press Ctrl+C during the next 10 seconds.
^C^C
SIGINT reset to default action.
Press Ctrl+C now to terminate the program.
^C
*/