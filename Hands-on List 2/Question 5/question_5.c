#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void fail(const char *message)
{
    perror(message);
    _exit(EXIT_FAILURE);
}

int main(void)
{
    int pipe1[2], pipe2[2];
    pid_t ls_pid, grep_pid, wc_pid;

    /* Create pipes for ls -> grep and grep -> wc. */
    if (pipe(pipe1) == -1)
        fail("pipe1");

    if (pipe(pipe2) == -1)
        fail("pipe2");

    /* First child: execute ls -l and redirect its output to pipe1. */
    ls_pid = fork();

    if (ls_pid == -1)
        fail("fork ls");

    if (ls_pid == 0) {
        if (dup2(pipe1[1], STDOUT_FILENO) == -1)
            fail("dup2 ls");

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        char *args[] = {"ls", "-l", NULL};
        execvp(args[0], args);
        fail("execvp ls");
    }

    /* Second child: filter directory lines and send them to pipe2. */
    grep_pid = fork();

    if (grep_pid == -1)
        fail("fork grep");

    if (grep_pid == 0) {
        if (dup2(pipe1[0], STDIN_FILENO) == -1)
            fail("dup2 grep stdin");

        if (dup2(pipe2[1], STDOUT_FILENO) == -1)
            fail("dup2 grep stdout");

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        char *args[] = {"grep", "^d", NULL};
        execvp(args[0], args);
        fail("execvp grep");
    }

    /* Third child: read filtered lines and count them using wc. */
    wc_pid = fork();

    if (wc_pid == -1)
        fail("fork wc");

    if (wc_pid == 0) {
        if (dup2(pipe2[0], STDIN_FILENO) == -1)
            fail("dup2 wc");

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        char *args[] = {"wc", NULL};
        execvp(args[0], args);
        fail("execvp wc");
    }

    /* The parent does not participate in data transfer. */
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    /* Reap all three children. */
    if (waitpid(ls_pid, NULL, 0) == -1)
        perror("waitpid ls");

    if (waitpid(grep_pid, NULL, 0) == -1)
        perror("waitpid grep");

    if (waitpid(wc_pid, NULL, 0) == -1)
        perror("waitpid wc");

    return 0;
}

/*
 * Test:
 * gcc question_5.c -o question_5
 * ./question_5
 *
 * Expected output:
 * Three numbers: lines, words, and bytes from
 * the output of "ls -l | grep ^d".
 * The first number is the directory count.
 * Exact values depend on the current directory.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_5.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_5.c -o question_5
./question_5
      0       0       0
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ls -l | grep '^d' | wc
      0       0       0
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_5
      0       0       0
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ls -l | grep '^d' | wc > expected.txt
./question_5 > actual.txt
diff -u expected.txt actual.txt
echo $?
0
*/