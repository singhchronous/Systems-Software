
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Write all bytes, retrying if the system call is interrupted. */
static int write_all(int fd, const char *buf, size_t len)
{
    size_t offset = 0;

    while (offset < len) {
        ssize_t n = write(fd, buf + offset, len - offset);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0) {
            errno = EIO;
            return -1;
        }

        offset += (size_t)n;
    }

    return 0;
}

/* Read one newline-terminated message, or return zero at EOF. */
static ssize_t read_line(int fd, char *buf, size_t capacity)
{
    size_t used = 0;

    if (capacity < 2) {
        errno = EINVAL;
        return -1;
    }

    while (used < capacity - 1) {
        char ch;
        ssize_t n = read(fd, &ch, 1);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0) {
            if (used == 0)
                return 0;
            break;
        }

        buf[used++] = ch;

        if (ch == '\n')
            break;
    }

    buf[used] = '\0';
    return (ssize_t)used;
}

int main(void)
{
    int p2c[2], c2p[2];
    pid_t pid;
    int status;
    char buffer[128];
    ssize_t n;
    const char request[] = "Hello Child!\n";
    const char reply[] = "Hello Parent!\n";

    /* Create two independent pipes before fork(). */
    if (pipe(p2c) == -1) {
        perror("pipe p2c");
        return EXIT_FAILURE;
    }

    if (pipe(c2p) == -1) {
        perror("pipe c2p");
        close(p2c[0]);
        close(p2c[1]);
        return EXIT_FAILURE;
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        close(p2c[0]);
        close(p2c[1]);
        close(c2p[0]);
        close(c2p[1]);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* Child reads from p2c and writes to c2p. */
        close(p2c[1]);
        close(c2p[0]);

        n = read_line(p2c[0], buffer, sizeof(buffer));
        if (n <= 0) {
            if (n == -1)
                perror("child read");
            close(p2c[0]);
            close(c2p[1]);
            _exit(EXIT_FAILURE);
        }

        /* Display the actual message received from the parent. */
        if (write_all(STDOUT_FILENO, "Child received: ", 16) == -1 ||
            write_all(STDOUT_FILENO, buffer, (size_t)n) == -1) {
            perror("child display");
            close(p2c[0]);
            close(c2p[1]);
            _exit(EXIT_FAILURE);
        }

        /* Send a reply through the second pipe. */
        if (write_all(c2p[1], reply, sizeof(reply) - 1) == -1) {
            perror("child write reply");
            close(p2c[0]);
            close(c2p[1]);
            _exit(EXIT_FAILURE);
        }

        close(p2c[0]);
        close(c2p[1]);
        _exit(EXIT_SUCCESS);
    }

    /* Parent writes to p2c and reads from c2p. */
    close(p2c[0]);
    close(c2p[1]);

    if (write_all(STDOUT_FILENO, "Parent sent: ", 13) == -1 ||
        write_all(STDOUT_FILENO, request, sizeof(request) - 1) == -1) {
        perror("parent display");
        close(p2c[1]);
        close(c2p[0]);
        waitpid(pid, NULL, 0);
        return EXIT_FAILURE;
    }

    if (write_all(p2c[1], request, sizeof(request) - 1) == -1) {
        perror("parent write request");
        close(p2c[1]);
        close(c2p[0]);
        waitpid(pid, NULL, 0);
        return EXIT_FAILURE;
    }

    /* Close the request pipe's writer after sending the message. */
    close(p2c[1]);

    n = read_line(c2p[0], buffer, sizeof(buffer));
    if (n <= 0) {
        if (n == -1)
            perror("parent read reply");
        else
            fprintf(stderr, "No reply received from child\n");

        close(c2p[0]);
        waitpid(pid, NULL, 0);
        return EXIT_FAILURE;
    }

    if (write_all(STDOUT_FILENO, "Parent received: ", 17) == -1 ||
        write_all(STDOUT_FILENO, buffer, (size_t)n) == -1) {
        perror("parent display reply");
        close(c2p[0]);
        waitpid(pid, NULL, 0);
        return EXIT_FAILURE;
    }

    close(c2p[0]);

    /* Collect the child's exit status. */
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
 * gcc question_3.c -o question_3
 * ./question_3
 *
 * Expected output:
 * Parent sent: Hello Child!
 * Child received: Hello Child!
 * Parent received: Hello Parent!
 *
 * The first pipe carries data from parent to child.
 * The second pipe carries the child's reply to the parent.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_3.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_3.c -o question_3
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_3
Parent sent: Hello Child!
Child received: Hello Child!
Parent received: Hello Parent!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ echo $?
0
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_3
Parent sent: Hello Child!
Child received: Hello Child!
Parent received: Hello Parent!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_3
Parent sent: Hello Child!
Child received: Hello Child!
Parent received: Hello Parent!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_3
Parent sent: Hello Child!
Child received: Hello Child!
Parent received: Hello Parent!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_3 > output.txt
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ cat output.txt
Parent sent: Hello Child!
Child received: Hello Child!
Parent received: Hello Parent!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ printf 'Parent sent: Hello Child!\nChild received: Hello Child!\nParent received: Hello Parent!\n' > expected.txt
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ cmp output.txt expected.txt
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ echo $?
0
*/