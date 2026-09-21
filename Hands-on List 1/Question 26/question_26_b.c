#include <stdio.h>
#include <unistd.h>

int main(void)
{
    /* Execute ls by searching for it in PATH */
    if (execlp("ls", "ls", "-Rl", NULL) == -1)
    {
        perror("execlp");
        return 1;
    }

    return 0;
}

/*
 * Test:
 * gcc question_26_b.c -o question_26_b
 * ./question_26_b
 *
 * Expected:
 * Recursive long-format listing equivalent to:
 * ls -Rl
 */