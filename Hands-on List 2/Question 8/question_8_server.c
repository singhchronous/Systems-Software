#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static int ensure_fifo(const char *path)
{
    struct stat info;

    /* Create the FIFO if absent, then validate the filesystem entry. */
    if (mkfifo(path, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return -1;
    }

    if (lstat(path, &info) == -1) {
        perror("lstat");
        return -1;
    }

    if (!S_ISFIFO(info.st_mode)) {
        fprintf(stderr, "%s exists but is not a FIFO\n", path);
        return -1;
    }

    return 0;
}

static int read_line(int fd, char *buffer, size_t capacity)
{
    size_t used = 0;

    if (capacity == 0)
        return -1;

    /* Read through the newline delimiter or until EOF. */
    while (used + 1 < capacity) {
        char ch;
        ssize_t n = read(fd, &ch, 1);

        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("read");
            return -1;
        }

        if (n == 0)
            break;

        buffer[used++] = ch;

        if (ch == '\n')
            break;
    }

    buffer[used] = '\0';
    return (int)used;
}

static int write_all(int fd, const char *message, size_t length)
{
    size_t sent = 0;

    /* Ensure the entire reply is written to the FIFO. */
    while (sent < length) {
        ssize_t n = write(fd, message + sent, length - sent);

        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("write");
            return -1;
        }

        sent += (size_t)n;
    }

    return 0;
}

int main(void)
{
    const char *in_fifo = "fifo_a_to_b";
    const char *out_fifo = "fifo_b_to_a";
    const char *reply = "Hello Client, message received!\n";
    char request[256];

    if (ensure_fifo(in_fifo) == -1 ||
        ensure_fifo(out_fifo) == -1)
        return EXIT_FAILURE;

    /* Open the incoming FIFO first to match the client's open order. */
    int in_fd = open(in_fifo, O_RDONLY);
    if (in_fd == -1) {
        perror("open client-to-server FIFO");
        return EXIT_FAILURE;
    }

    int out_fd = open(out_fifo, O_WRONLY);
    if (out_fd == -1) {
        perror("open server-to-client FIFO");
        close(in_fd);
        return EXIT_FAILURE;
    }

    int count = read_line(in_fd, request, sizeof(request));
    if (count == -1 || count == 0) {
        if (count == 0)
            fprintf(stderr, "Client closed without sending a message\n");
        close(in_fd);
        close(out_fd);
        return EXIT_FAILURE;
    }

    printf("Server received: %s", request);

    if (write_all(out_fd, reply, strlen(reply)) == -1) {
        close(in_fd);
        close(out_fd);
        return EXIT_FAILURE;
    }

    printf("Server sent: %s", reply);

    close(in_fd);
    close(out_fd);
    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_8_server.c -o question_8_server
 * ./question_8_server
 *
 * Start the client in another terminal.
 * Expected:
 * Server received: Hello Server, this is the client!
 * Server sent: Hello Client, message received!
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_8_client.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_8_server.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_8_client.c -o question_8_client
gcc question_8_server.c -o question_8_server
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_8_server
Server received: Hello Server, this is the client!
Server sent: Hello Client, message received!
*/