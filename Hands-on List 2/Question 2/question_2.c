
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Write all requested bytes, handling partial writes and EINTR. */
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
    pid_t pid;
    int status;
    char buffer[128];
    ssize_t n;
    const char message[] = "Hello from the parent process!";

    /* Create the pipe before fork so both processes inherit its ends. */
    if (pipe(fd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* Child: only the read end is needed. */
        if (close(fd[1]) == -1) {
            perror("child close write end");
            close(fd[0]);
            _exit(EXIT_FAILURE);
        }

        /* Receive the parent's bytes until the pipe reaches EOF. */
        for (;;) {
            n = read(fd[0], buffer, sizeof(buffer));

            if (n > 0) {
                if (write_all(STDOUT_FILENO, buffer, (size_t)n) == -1) {
                    perror("child write stdout");
                    close(fd[0]);
                    _exit(EXIT_FAILURE);
                }
            } else if (n == 0) {
                break;
            } else if (errno != EINTR) {
                perror("child read");
                close(fd[0]);
                _exit(EXIT_FAILURE);
            }
        }

        if (write_all(STDOUT_FILENO, "\n", 1) == -1) {
            perror("child write newline");
            close(fd[0]);
            _exit(EXIT_FAILURE);
        }

        if (close(fd[0]) == -1) {
            perror("child close read end");
            _exit(EXIT_FAILURE);
        }

        _exit(EXIT_SUCCESS);
    }

    /* Parent: only the write end is needed. */
    if (close(fd[0]) == -1) {
        perror("parent close read end");
        close(fd[1]);
        waitpid(pid, NULL, 0);
        return EXIT_FAILURE;
    }

    /* Transmit the message bytes to the child. */
    if (write_all(fd[1], message, sizeof(message) - 1) == -1) {
        perror("parent write to pipe");
        close(fd[1]);
        waitpid(pid, NULL, 0);
        return EXIT_FAILURE;
    }

    /* Closing the final writer lets the child observe EOF. */
    if (close(fd[1]) == -1) {
        perror("parent close write end");
        waitpid(pid, NULL, 0);
        return EXIT_FAILURE;
    }

    /* Collect the child's exit status to avoid leaving a zombie. */
    while (waitpid(pid, &status, 0) == -1) {
        if (errno == EINTR)
            continue;

        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
        fprintf(stderr, "Child process failed\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_2.c -o question_2
 * ./question_2
 *
 * Expected output:
 * Hello from the parent process!
 *
 * The parent writes the message to an unnamed pipe.
 * The child reads the bytes and displays them until EOF.
 */


/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_2.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_2.c -o question_2
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_2
Hello from the parent process!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ echo $?
0
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_2
Hello from the parent process!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_2
Hello from the parent process!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_2
Hello from the parent process!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_2 > output.txt
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ cat output.txt
Hello from the parent process!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ printf 'Hello from the parent process!\n' > expected.txt
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ cmp output.txt expected.txt
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ echo $?
0
*/