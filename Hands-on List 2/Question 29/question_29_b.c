#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int signo)
{
    const char msg[] = "\nCaught SIGINT: Ctrl+C received.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(128 + signo);
}

int main(void)
{
    signal(SIGINT, handler);

    const char msg[] = "Program running. Press Ctrl+C to generate SIGINT.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    while (1)
        pause();

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_29_b
Program running. Press Ctrl+C to generate SIGINT.
^C
Caught SIGINT: Ctrl+C received.
*/