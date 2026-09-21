#include <stdio.h>
#include <unistd.h>

int main(void)
{
    char *envp[] = {
        "MY_VAR=HelloLinux",
        "PATH=/usr/bin:/bin",
        NULL
    };

    /* Execute ls with an explicitly supplied environment */
    if (execle("/usr/bin/ls", "ls", "-Rl", NULL, envp) == -1)
    {
        perror("execle");
        return 1;
    }

    return 0;
}

/*
 * Test:
 * gcc question_26_c.c -o question_26_c
 * ./question_26_c
 *
 * Expected:
 * Recursive long-format listing equivalent to:
 * ls -Rl
 *
 * The environment is explicitly supplied through envp.
 */