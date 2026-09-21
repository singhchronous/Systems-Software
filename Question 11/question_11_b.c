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

    /* Duplicate fd1 as descriptor 10 */
    fd2 = dup2(fd1, 10);

    if (fd2 == -1)
    {
        perror("dup2");
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
 * gcc question_11_dup2.c -o question_11_dup2
 * ./question_11_dup2
 * cat file1
 *
 * Both writes should appear in file1.
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_11_b.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_11_b.c -o question_11_b
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_11_b
fd1 = 3, fd2 = 10
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat file1
Written using fd1
Written using fd2

*/