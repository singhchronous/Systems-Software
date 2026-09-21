#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>

int main(void)
{
    fd_set readfds;
    struct timeval timeout;
    char buffer[100];
    int result;

    /* Add STDIN to the set of file descriptors to monitor */
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    /* Set the timeout to 10 seconds */
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    printf("Waiting for input from STDIN for 10 seconds...\n");
    fflush(stdout);

    /* Wait for STDIN to become ready or for the timeout to expire */
    result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

    if (result == -1)
    {
        perror("select");
        return 1;
    }

    /* No file descriptor became ready within 10 seconds */
    if (result == 0)
    {
        printf("Timeout: No input received within 10 seconds.\n");
    }
    else if (FD_ISSET(STDIN_FILENO, &readfds))
    {
        printf("Data is available on STDIN.\n");

        /* Read and display the available input */
        int bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0';
            printf("Input received: %s", buffer);
        }
    }

    return 0;
}

/*
 * Test:
 * gcc question_13.c -o question_13
 * ./question_13
 *
 * Test 1:
 * Do not enter anything and wait for 10 seconds.
 *
 * Expected output:
 * Timeout: No input received within 10 seconds.
 *
 * Test 2:
 * Run the program and enter text within 10 seconds.
 *
 * Expected output:
 * Data is available on STDIN.
 * Input received: <entered text>
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_13.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_13.c -o question_13
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_13
Waiting for input from STDIN for 10 seconds...
Timeout: No input received within 10 seconds.
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_13
Waiting for input from STDIN for 10 seconds...
Hello Linux
Data is available on STDIN.
Input received: Hello Linux

*/