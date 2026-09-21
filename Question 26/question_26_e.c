#include <stdio.h>
#include <unistd.h>

int main(void)
{
    char *args[] = {
        "ls",
        "-Rl",
        NULL
    };

    /* Execute ls by searching for it in PATH */
    if (execvp("ls", args) == -1)
    {
        perror("execvp");
        return 1;
    }

    return 0;
}

/*
 * Test:
 * gcc question_26_e.c -o question_26_e
 * ./question_26_e
 *
 * Expected:
 * Recursive long-format listing equivalent to:
 * ls -Rl
 */