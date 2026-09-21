#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;
    pid_t pid;

    /* Open the file before fork so both processes inherit the descriptor */
    fd = open("file1", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    /* Create the child process */
    pid = fork();

    if (pid == -1)
    {
        perror("fork");
        close(fd);
        return 1;
    }

    /* Child writes to the inherited file descriptor */
    if (pid == 0)
    {
        write(fd, "Child process\n", 14);
    }
    else
    {
        /* Parent writes to the inherited file descriptor */
        write(fd, "Parent process\n", 15);
    }

    close(fd);

    return 0;
}

/*
 * Test:
 * gcc question_21.c -o question_21
 * ./question_21
 *
 * Check the file:
 * cat file1
 *
 * Possible output:
 * Parent process
 * Child process
 *
 * Or:
 * Child process
 * Parent process
 *
 * The order may vary because the parent and child are scheduled
 * independently after fork().
 *
 * The file is opened before fork(), so both processes inherit
 * file descriptors referring to the same open file description.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_21.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_21.c -o question_21
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_21.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_21
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat file1
Parent process
Child process
*/