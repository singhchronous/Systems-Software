/*
 * Question 23(c): ITIMER_PROF
 * Measures user-mode plus system-mode CPU time and delivers SIGPROF.
 *
 * Test:
 *   gcc question_23_c.c -o question_23_c
 *   ./question_23_c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

static volatile unsigned long long counter = 0;

static void handle_prof(int signo)
{
    (void)signo;

    const char message[] = "ITIMER_PROF expired: SIGPROF received.\n";
    write(STDOUT_FILENO, message, sizeof(message) - 1);
}

int main(void)
{
    struct sigaction sa;
    struct itimerval timer;

    sa.sa_handler = handle_prof;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGPROF, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    timer.it_value.tv_sec = 10;
    timer.it_value.tv_usec = 10;
    timer.it_interval.tv_sec = 10;
    timer.it_interval.tv_usec = 10;

    if (setitimer(ITIMER_PROF, &timer, NULL) == -1) {
        perror("setitimer");
        return EXIT_FAILURE;
    }

    printf("ITIMER_PROF started: measuring user plus system CPU time.\n");
    printf("Initial expiration and repeat interval: 10 seconds and 10 microseconds.\n");
    fflush(stdout);

    while (1)
        counter++;

    return EXIT_SUCCESS;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_23_a.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_23_b.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_23_c.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_23_a.c -o question_23_a
gcc question_23_b.c -o question_23_b
gcc question_23_c.c -o question_23_c

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_23_c
ITIMER_PROF started: measuring user plus system CPU time.
Initial expiration and repeat interval: 10 seconds and 10 microseconds.
ITIMER_PROF expired: SIGPROF received.
*/