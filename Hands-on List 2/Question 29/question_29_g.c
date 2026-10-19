#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

volatile unsigned long long counter = 0;

void handler(int signo)
{
    const char msg[] = "Caught SIGPROF: process CPU-time timer expired.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void)
{
    struct itimerval timer = {0};

    signal(SIGPROF, handler);

    timer.it_value.tv_sec = 2;
    timer.it_value.tv_usec = 0;

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_PROF, &timer, NULL) == -1) {
        const char msg[] = "setitimer failed.\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    const char msg[] = "ITIMER_PROF set for 2 process CPU seconds.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    while (1)
        counter++;

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_29_g
ITIMER_PROF set for 2 process CPU seconds.
Caught SIGPROF: process CPU-time timer expired.
*/