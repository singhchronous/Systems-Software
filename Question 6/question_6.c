#include <unistd.h>

int main(void)
{
    char buffer[100];
    ssize_t bytes_read;

    /* Read input from STDIN */
    bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));

    if (bytes_read > 0)
    {
        /* Display the input on STDOUT */
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    return 0;
}

/*
 * Test:
 * gcc question_6.c -o question_6
 * ./question_6
 *
 * Enter text and press Enter.
 * The same text is displayed on STDOUT.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_6
Hello Linux
Hello Linux
*/