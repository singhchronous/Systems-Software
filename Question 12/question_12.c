#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;
    int flags;

    /* Open the file in read-write mode */
    fd = open("file1", O_RDWR | O_CREAT, 0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    /* Get the file status flags associated with the file descriptor */
    flags = fcntl(fd, F_GETFL);

    if (flags == -1)
    {
        perror("fcntl");
        close(fd);
        return 1;
    }

    /* Extract and identify the file access mode */
    if ((flags & O_ACCMODE) == O_RDONLY)
        printf("File opened in read-only mode.\n");
    else if ((flags & O_ACCMODE) == O_WRONLY)
        printf("File opened in write-only mode.\n");
    else if ((flags & O_ACCMODE) == O_RDWR)
        printf("File opened in read-write mode.\n");

    close(fd);

    return 0;
}

/*
 * Test:
 * gcc question_12.c -o question_12
 * ./question_12
 *
 * Expected output:
 * File opened in read-write mode.
 *
 * To test other modes, change O_RDWR to O_RDONLY or O_WRONLY.
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_12.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_12.c -o question_12
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_12
File opened in read-write mode.
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ls -l file1
-rw-rw-r-- 1 shubham-singh shubham-singh 36 Sep 21 12:09 file1

*/