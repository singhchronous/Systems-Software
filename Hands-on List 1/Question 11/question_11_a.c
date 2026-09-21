#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd1, fd2;

    /* Open file in append mode */
    fd1 = open("file1", O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644);

    if (fd1 == -1)
    {
        perror("open");
        return 1;
    }

    /* Duplicate the file descriptor */
    fd2 = dup(fd1);

    if (fd2 == -1)
    {
        perror("dup");
        close(fd1);
        return 1;
    }

    printf("fd1 = %d, fd2 = %d\n", fd1, fd2);

    write(fd1, "Written using fd1\n", 18);
    write(fd2, "Written using fd2\n", 18);

    close(fd1);
    close(fd2);

    return 0;
}

/*
 * Test:
 * gcc question_11_dup.c -o question_11_dup
 * ./question_11_dup
 * cat file1
 *
 * Both writes should appear in file1.
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_11_a.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_11_a.c -o question_11_a
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_11_a
fd1 = 3, fd2 = 4
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat file1
Written using fd1
Written using fd2

*/