#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
    const char *fifo = "myfifo_q7";
    const char *message = "Hello from the FIFO writer!\n";
    struct stat info;

    /* Create the FIFO if it does not already exist. */
    if (mkfifo(fifo, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return EXIT_FAILURE;
    }

    /* Reject an existing path that is not a FIFO. */
    if (lstat(fifo, &info) == -1) {
        perror("lstat");
        return EXIT_FAILURE;
    }

    if (!S_ISFIFO(info.st_mode)) {
        fprintf(stderr, "%s exists but is not a FIFO\n", fifo);
        return EXIT_FAILURE;
    }

    /* Opening for writing normally waits for a reader. */
    int fd = open(fifo, O_WRONLY);
    if (fd == -1) {
        perror("open FIFO for writing");
        return EXIT_FAILURE;
    }

    /* Handle partial writes so the complete message is sent. */
    size_t length = strlen(message);
    size_t sent = 0;

    while (sent < length) {
        ssize_t n = write(fd, message + sent, length - sent);

        if (n == -1) {
            if (errno == EINTR)
                continue;

            perror("write");
            close(fd);
            return EXIT_FAILURE;
        }

        sent += (size_t)n;
    }

    printf("Message sent successfully.\n");

    /* Closing the write end lets the reader eventually observe EOF. */
    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_7_writer.c -o question_7_writer
 * ./question_7_writer
 *
 * Run the reader in another terminal before starting the writer.
 * Expected writer output:
 * Message sent successfully.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_7_writer.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_7_reader.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_7_writer.c -o question_7_writer
gcc question_7_reader.c -o question_7_reader
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cd ~/"Hands-on List 2"
./question_7_writer
Message sent successfully.
*/