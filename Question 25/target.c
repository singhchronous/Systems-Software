#include <stdio.h>

int main(int argc, char *argv[])
{
    /* Check whether a name was provided */
    if (argc != 2)
    {
        printf("Usage: %s <name>\n", argv[0]);
        return 1;
    }

    /* Display the command-line argument */
    printf("Hello, %s!\n", argv[1]);

    return 0;
}

/*
 * Test:
 * gcc target.c -o target
 * ./target Shubham
 *
 * Expected output:
 * Hello, Shubham!
 */