/*
 * Question 23(a): ITIMER_REAL
 * Measures elapsed wall-clock time and delivers SIGALRM.
 *
 * Test:
 *   gcc question_23_a.c -o question_23_a
 *   ./question_23_a
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

static void handle_alarm(int signo)
{
    (void)signo;

    const char message[] = "ITIMER_REAL expired: SIGALRM received.\n";
    write(STDOUT_FILENO, message, sizeof(message) - 1);
}

int main(void)
{
    struct sigaction sa;
    struct itimerval timer;

    sa.sa_handler = handle_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    timer.it_value.tv_sec = 10;
    timer.it_value.tv_usec = 10;
    timer.it_interval.tv_sec = 10;
    timer.it_interval.tv_usec = 10;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        return EXIT_FAILURE;
    }

    printf("ITIMER_REAL started: initial expiration in 10 seconds and 10 microseconds.\n");
    printf("Periodic interval: 10 seconds and 10 microseconds.\n");
    fflush(stdout);

    while (1)
        pause();

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

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_23_a
ITIMER_REAL started: initial expiration in 10 seconds and 10 microseconds.
Periodic interval: 10 seconds and 10 microseconds.
ITIMER_REAL expired: SIGALRM received.
*/