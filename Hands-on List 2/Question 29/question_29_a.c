/*
 * Question 29(a): Catch SIGSEGV using signal().
 *
 * Test:
 *   gcc question_29_a.c -o question_29_a
 *   ./question_29_a
 *
 * Expected:
 *   Caught SIGSEGV: invalid memory access.
 */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int signo)
{
    const char msg[] = "Caught SIGSEGV: invalid memory access.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(128 + signo);
}

int main(void)
{
    signal(SIGSEGV, handler);

    volatile int *ptr = (volatile int *)0;
    *ptr = 42;

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_29_a
Caught SIGSEGV: invalid memory access.
*/