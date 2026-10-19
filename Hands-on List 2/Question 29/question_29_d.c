#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int signo)
{
    const char msg[] = "Caught SIGALRM: alarm timer expired.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void)
{
    signal(SIGALRM, handler);

    const char msg[] = "Alarm set for 5 seconds.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    alarm(5);

    while (1)
        pause();

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_29_d
Alarm set for 5 seconds.
Caught SIGALRM: alarm timer expired.
*/