#include <stdio.h>

/* Declare the environment variable array provided by the system */
extern char **environ;

int main(void)
{
    char **env = environ;

    /* Traverse and display all environment variables */
    while (*env != NULL)
    {
        printf("%s\n", *env);
        env++;
    }

    return 0;
}

/*
 * Test:
 * gcc question_15.c -o question_15
 * ./question_15
 *
 * The program should display all environment variables
 * inherited from the shell.
 *
 * Compare the output with:
 * env
 *
 * To test a custom environment variable:
 * export MY_VAR="Hello Linux"
 * ./question_15 | grep MY_VAR
 *
 * Expected:
 * MY_VAR=Hello Linux
 */