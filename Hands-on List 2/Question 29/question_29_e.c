#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int signo)
{
    const char msg[] = "Caught SIGALRM: ITIMER_REAL expired.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void)
{
    struct itimerval timer = {0};

    signal(SIGALRM, handler);

    timer.it_value.tv_sec = 5;
    timer.it_value.tv_usec = 0;

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        const char msg[] = "setitimer failed.\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    const char msg[] = "ITIMER_REAL set for 5 seconds.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    while (1)
        pause();

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_29_e
ITIMER_REAL set for 5 seconds.
Caught SIGALRM: ITIMER_REAL expired.
*/