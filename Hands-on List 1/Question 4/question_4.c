#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;

    /* Open an existing file in read-write mode */
    fd = open("existing.txt", O_RDWR);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    printf("File opened successfully. FD = %d\n", fd);
    close(fd);

    /* O_EXCL fails if the file already exists when used with O_CREAT */
    fd = open("existing.txt", O_RDWR | O_CREAT | O_EXCL, 0644);

    if (fd == -1)
    {
        perror("open with O_EXCL");
    }
    else
    {
        printf("File created successfully. FD = %d\n", fd);
        close(fd);
    }

    return 0;
}

/*
 * Test:
 * echo "Hello Linux" > existing.txt
 * gcc question_4.c -o question_4
 * ./question_4
 *
 * Expected:
 * File opened successfully. FD = 3
 * open with O_EXCL: File exists
 * 
 * Output:
 * 
 * shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_4
File opened successfully. FD = 3
open with O_EXCL: File exists
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ls -l existing.txt
-rw-rw-r-- 1 shubham-singh shubham-singh 12 Sep 20 18:01 existing.txt
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ rm existing.txt
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_4
open: No such file or directory
 */