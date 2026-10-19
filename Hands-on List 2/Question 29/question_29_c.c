#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int signo)
{
    const char msg[] = "Caught SIGFPE: arithmetic exception signal.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(128 + signo);
}

int main(void)
{
    signal(SIGFPE, handler);

    const char msg[] = "Generating SIGFPE...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    raise(SIGFPE);

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_29_c
Generating SIGFPE...
Caught SIGFPE: arithmetic exception signal.
*/