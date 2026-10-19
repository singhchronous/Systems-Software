#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int signo)
{
    const char msg[] = "\nCaught SIGINT using sigaction: Ctrl+C received.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(128 + signo);
}

int main(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("Program running. Press Ctrl+C to generate SIGINT.\n");
    fflush(stdout);

    while (1)
        pause();

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_31_b
Program running. Press Ctrl+C to generate SIGINT.
^C
Caught SIGINT using sigaction: Ctrl+C received.
*/