#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Write all requested bytes, handling partial writes and interruptions. */
static int write_all(int fd, const void *buf, size_t count)
{
    const char *p = buf;

    while (count > 0) {
        ssize_t n = write(fd, p, count);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0) {
            errno = EIO;
            return -1;
        }

        p += n;
        count -= (size_t)n;
    }

    return 0;
}

int main(void)
{
    int fd[2];
    const char message[] = "Hello from the pipe!";
    const char prefix[] = "Message read from pipe: ";
    const char newline[] = "\n";
    char buffer[128];
    ssize_t n;

    /* Create an unnamed pipe: fd[0] reads, fd[1] writes. */
    if (pipe(fd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    /* Send the message bytes through the pipe's write end. */
    if (write_all(fd[1], message, sizeof(message) - 1) == -1) {
        perror("write");
        close(fd[0]);
        close(fd[1]);
        return EXIT_FAILURE;
    }

    /* Close the writer so the reader can eventually observe EOF. */
    if (close(fd[1]) == -1) {
        perror("close write end");
        close(fd[0]);
        return EXIT_FAILURE;
    }

    /* Display a label, then display the bytes received from the pipe. */
    if (write_all(STDOUT_FILENO, prefix, sizeof(prefix) - 1) == -1) {
        perror("write stdout");
        close(fd[0]);
        return EXIT_FAILURE;
    }

    /* Read until EOF; a read may return fewer bytes than requested. */
    for (;;) {
        n = read(fd[0], buffer, sizeof(buffer));

        if (n > 0) {
            if (write_all(STDOUT_FILENO, buffer, (size_t)n) == -1) {
                perror("write stdout");
                close(fd[0]);
                return EXIT_FAILURE;
            }
        } else if (n == 0) {
            break;
        } else if (errno != EINTR) {
            perror("read");
            close(fd[0]);
            return EXIT_FAILURE;
        }
    }

    if (write_all(STDOUT_FILENO, newline, sizeof(newline) - 1) == -1) {
        perror("write stdout");
        close(fd[0]);
        return EXIT_FAILURE;
    }

    /* Release the remaining pipe descriptor. */
    if (close(fd[0]) == -1) {
        perror("close read end");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_1.c -o question_1
 * ./question_1
 *
 * Expected output:
 * Message read from pipe: Hello from the pipe!
 *
 * The message is written to the pipe, read back, and displayed
 * on standard output. The read loop terminates when it receives EOF.
 */


/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_1.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_1.c -o question_1
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_1
Message read from pipe: Hello from the pipe!
*/