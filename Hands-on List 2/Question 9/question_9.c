#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/select.h>

int main(void)
{
    const char *fifo = "myfifo_q9";
    struct stat info;
    char buffer[512];

    /* Create the FIFO if absent. */
    if (mkfifo(fifo, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return EXIT_FAILURE;
    }

    /* Validate an existing path instead of assuming it is a FIFO. */
    if (lstat(fifo, &info) == -1) {
        perror("lstat");
        return EXIT_FAILURE;
    }

    if (!S_ISFIFO(info.st_mode)) {
        fprintf(stderr, "%s exists but is not a FIFO\n", fifo);
        return EXIT_FAILURE;
    }

    /*
     * Linux permits O_RDWR on a FIFO. Keeping a local write end open
     * avoids immediate EOF when no external writer is connected.
     * O_NONBLOCK also prevents the subsequent read from blocking.
     */
    int fd = open(fifo, O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    /* Monitor the FIFO descriptor for read readiness. */
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    printf("Waiting up to 10 seconds for FIFO data...\n");
    fflush(stdout);

    int result = select(fd + 1, &readfds, NULL, NULL, &timeout);

    if (result == -1) {
        perror("select");
        close(fd);
        return EXIT_FAILURE;
    }

    if (result == 0) {
        printf("Timeout: no data received within 10 seconds.\n");
        close(fd);
        return EXIT_SUCCESS;
    }

    /* Read only after select() reports that the FIFO is readable. */
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);

    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("No data available to read.\n");
            close(fd);
            return EXIT_SUCCESS;
        }

        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }

    if (n == 0) {
        printf("FIFO reached EOF.\n");
    } else {
        buffer[n] = '\0';
        printf("Data received: %s", buffer);

        if (buffer[n - 1] != '\n')
            putchar('\n');
    }

    close(fd);
    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_9.c -o question_9
 * ./question_9
 *
 * In another terminal, within 10 seconds, run:
 * printf "Hello through FIFO!\n" > myfifo_q9
 *
 * Expected:
 * Data received: Hello through FIFO!
 *
 * If no data arrives within 10 seconds:
 * Timeout: no data received within 10 seconds.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_9.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_9.c -o question_9
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_9
Waiting up to 10 seconds for FIFO data...
Timeout: no data received within 10 seconds.

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_9

Terminal 2:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ printf 'Hello through FIFO!\n' > myfifo_q9

Waiting up to 10 seconds for FIFO data...
Data received: Hello through FIFO!
*/