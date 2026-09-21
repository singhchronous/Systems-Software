#include <stdio.h>
#include <unistd.h>

int main(void)
{
    /* Execute ls using an explicit path and argument list */
    if (execl("/usr/bin/ls", "ls", "-Rl", NULL) == -1)
    {
        perror("execl");
        return 1;
    }

    return 0;
}

/*
 * Test:
 * gcc question_26_a.c -o question_26_a
 * ./question_26_a
 *
 * Expected:
 * Recursive long-format listing equivalent to:
 * ls -Rl
 */