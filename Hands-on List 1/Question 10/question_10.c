#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;
    off_t position;

    /* Open file in read-write mode and truncate existing contents */
    fd = open("file1", O_RDWR | O_CREAT | O_TRUNC, 0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    /* Write the first 10 bytes */
    if (write(fd, "ABCDEFGHIJ", 10) != 10)
    {
        perror("write");
        close(fd);
        return 1;
    }

    /* Move the file offset forward by 10 bytes */
    position = lseek(fd, 10, SEEK_CUR);

    if (position == (off_t)-1)
    {
        perror("lseek");
        close(fd);
        return 1;
    }

    printf("lseek returned: %ld\n", (long)position);

    /* Write another 10 bytes after the gap */
    if (write(fd, "1234567890", 10) != 10)
    {
        perror("write");
        close(fd);
        return 1;
    }

    close(fd);

    return 0;
}

/*
 * Test:
 * gcc question_10.c -o question_10
 * ./question_10
 * od -c file1
 * od -An -tx1 -v file1
 *
 * Expected: 10 zero bytes between the two 10-byte data blocks.
 */

 /*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_10.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_10.c -o question_10
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_10
lseek returned: 20
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ od -c file1
0000000   A   B   C   D   E   F   G   H   I   J  \0  \0  \0  \0  \0  \0
0000020  \0  \0  \0  \0   1   2   3   4   5   6   7   8   9   0
0000036

*/