#include <stdio.h>
#include <unistd.h>

int main(void)
{
    char *args[] = {
        "ls",
        "-Rl",
        NULL
    };

    /* Execute ls using an explicit path and argument vector */
    if (execv("/usr/bin/ls", args) == -1)
    {
        perror("execv");
        return 1;
    }

    return 0;
}

/*
 * Test:
 * gcc question_26_d.c -o question_26_d
 * ./question_26_d
 *
 * Expected:
 * Recursive long-format listing equivalent to:
 * ls -Rl
 */