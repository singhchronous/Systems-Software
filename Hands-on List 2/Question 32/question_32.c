/*
 * Question 32: Ignoring and Resetting SIGINT using sigaction().
 *
 * Test:
 *   gcc question_32.c -o question_32
 *   ./question_32
 *
 * Expected:
 *   SIGINT is ignored during the initial 10-second window.
 *   After resetting to SIG_DFL, Ctrl+C terminates the process.
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction: ignore SIGINT");
        return 1;
    }

    printf("SIGINT is now ignored.\n");
    printf("Press Ctrl+C during the next 10 seconds.\n");
    fflush(stdout);

    sleep(10);

    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction: reset SIGINT");
        return 1;
    }

    printf("\nSIGINT reset to its default action.\n");
    printf("Press Ctrl+C now to terminate the program.\n");
    fflush(stdout);

    while (1)
        pause();

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_32.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_32.c -o question_32
./question_32
SIGINT is now ignored.
Press Ctrl+C during the next 10 seconds.
^C^C^C^C
SIGINT reset to its default action.
Press Ctrl+C now to terminate the program.
^C
*/