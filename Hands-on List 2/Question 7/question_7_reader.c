#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
    const char *fifo = "myfifo_q7";
    struct stat info;
    char buffer[128];

    /* Create the FIFO if necessary; reuse it if it already exists. */
    if (mkfifo(fifo, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return EXIT_FAILURE;
    }

    /* Ensure the path refers to a FIFO, not an ordinary file. */
    if (lstat(fifo, &info) == -1) {
        perror("lstat");
        return EXIT_FAILURE;
    }

    if (!S_ISFIFO(info.st_mode)) {
        fprintf(stderr, "%s exists but is not a FIFO\n", fifo);
        return EXIT_FAILURE;
    }

    /* Opening for reading normally waits for a writer. */
    int fd = open(fifo, O_RDONLY);
    if (fd == -1) {
        perror("open FIFO for reading");
        return EXIT_FAILURE;
    }

    printf("Waiting for message...\n");
    printf("Received message: ");

    /* Read until EOF, handling interruptions and arbitrary read sizes. */
    for (;;) {
        ssize_t n = read(fd, buffer, sizeof(buffer));

        if (n == -1) {
            if (errno == EINTR)
                continue;

            perror("read");
            close(fd);
            return EXIT_FAILURE;
        }

        if (n == 0)
            break;

        if (fwrite(buffer, 1, (size_t)n, stdout) != (size_t)n) {
            perror("fwrite");
            close(fd);
            return EXIT_FAILURE;
        }
    }

    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_7_reader.c -o question_7_reader
 * ./question_7_reader
 *
 * Start this program in one terminal, then run the writer in another.
 * Expected received message:
 * Hello from the FIFO writer!
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_7_writer.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_7_reader.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_7_writer.c -o question_7_writer
gcc question_7_reader.c -o question_7_reader
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_7_reader
Waiting for message...
Received message: Hello from the FIFO writer!
*/
