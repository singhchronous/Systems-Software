#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Redirect target to source using dup2(). */
static void redirect_fd(int source, int target)
{
    if (dup2(source, target) == -1) {
        perror("dup2");
        _exit(EXIT_FAILURE);
    }

    if (close(source) == -1) {
        perror("close source");
        _exit(EXIT_FAILURE);
    }
}

int main(void)
{
    int fd[2];
    pid_t ls_pid, wc_pid;
    int status1, status2;

    /* Create the pipe before forking either command. */
    if (pipe(fd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    ls_pid = fork();

    if (ls_pid == -1) {
        perror("fork ls");
        close(fd[0]);
        close(fd[1]);
        return EXIT_FAILURE;
    }

    if (ls_pid == 0) {
        /* Redirect ls standard output to the pipe's write end. */
        close(fd[0]);
        redirect_fd(fd[1], STDOUT_FILENO);

        char *args[] = {"ls", "-l", NULL};
        execvp(args[0], args);
        perror("execvp ls");
        _exit(EXIT_FAILURE);
    }

    wc_pid = fork();

    if (wc_pid == -1) {
        perror("fork wc");
        close(fd[0]);
        close(fd[1]);
        waitpid(ls_pid, NULL, 0);
        return EXIT_FAILURE;
    }

    if (wc_pid == 0) {
        /* Redirect wc standard input to the pipe's read end. */
        close(fd[1]);
        redirect_fd(fd[0], STDIN_FILENO);

        char *args[] = {"wc", NULL};
        execvp(args[0], args);
        perror("execvp wc");
        _exit(EXIT_FAILURE);
    }

    /* Close the parent's copies so wc can receive EOF. */
    close(fd[0]);
    close(fd[1]);

    if (waitpid(ls_pid, &status1, 0) == -1) {
        perror("waitpid ls");
        return EXIT_FAILURE;
    }

    if (waitpid(wc_pid, &status2, 0) == -1) {
        perror("waitpid wc");
        return EXIT_FAILURE;
    }

    if (!WIFEXITED(status1) || WEXITSTATUS(status1) != 0 ||
        !WIFEXITED(status2) || WEXITSTATUS(status2) != 0) {
        fprintf(stderr, "Pipeline command failed\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_4_b.c -o question_4_b
 * ./question_4_b
 *
 * Expected output:
 * Three counts: lines, words, and bytes from ls -l output.
 * The exact values depend on the current directory.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_4_a.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_4_b.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_4_c.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_4_a.c -o question_4_a
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_4_b.c -o question_4_b
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_4_c.c -o question_4_c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_4_a
     13     110     886
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_4_b
     13     110     886
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_4_c
     13     110     886
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ls -l | wc
     13     110     886
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_4_a  
./question_4_b   
./question_4_c
     13     110     886
     13     110     886
     13     110     886
*/