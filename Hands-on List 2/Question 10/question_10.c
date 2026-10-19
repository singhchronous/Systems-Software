#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>

static void print_limit(const char *name, rlim_t value)
{
    /* RLIM_INFINITY represents a limit reported as unlimited. */
    if (value == RLIM_INFINITY)
        printf("%s: unlimited\n", name);
    else
        printf("%s: %llu\n", name, (unsigned long long)value);
}

int main(void)
{
    struct rlimit limit;
    int pipefd[2];

    /* Query the process's soft and hard open-file-descriptor limits. */
    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        perror("getrlimit");
        return EXIT_FAILURE;
    }

    printf("Maximum open file descriptor limits:\n");
    print_limit("Soft limit", limit.rlim_cur);
    print_limit("Hard limit", limit.rlim_max);

    /* Create a pipe whose kernel buffer capacity can be queried. */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    /* F_GETPIPE_SZ returns the capacity in bytes on Linux. */
    int pipe_size = fcntl(pipefd[0], F_GETPIPE_SZ);

    if (pipe_size == -1) {
        perror("fcntl F_GETPIPE_SZ");
        close(pipefd[0]);
        close(pipefd[1]);
        return EXIT_FAILURE;
    }

    printf("Pipe capacity: %d bytes\n", pipe_size);

    /* Release both file descriptors allocated by pipe(). */
    close(pipefd[0]);
    close(pipefd[1]);

    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_10.c -o question_10
 * ./question_10
 *
 * Expected output:
 * Maximum open file descriptor limits:
 * Soft limit: <system-dependent value>
 * Hard limit: <system-dependent value>
 * Pipe capacity: <system-dependent value> bytes
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_10.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_10.c -o question_10
./question_10
Maximum open file descriptor limits:
Soft limit: 1024
Hard limit: 524288
Pipe capacity: 65536 bytes
*/