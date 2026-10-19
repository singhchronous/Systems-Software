#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int signo)
{
    const char msg[] = "Caught SIGSEGV using sigaction: invalid memory access.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(128 + signo);
}

int main(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("Generating SIGSEGV...\n");
    fflush(stdout);

    volatile int *ptr = (volatile int *)0;
    *ptr = 42;

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_31_a
Generating SIGSEGV...
Caught SIGSEGV using sigaction: invalid memory access.
*/