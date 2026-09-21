#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd1, fd2;
    char buffer[4096];
    ssize_t bytes_read;

    /* Open source file for reading */
    fd1 = open("file1", O_RDONLY);

    if (fd1 == -1)
    {
        perror("file1");
        return 1;
    }

    /* Create/truncate destination file for writing */
    fd2 = open("file2", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd2 == -1)
    {
        perror("file2");
        close(fd1);
        return 1;
    }

    /* Copy the file in chunks */
    while ((bytes_read = read(fd1, buffer, sizeof(buffer))) > 0)
    {
        ssize_t total_written = 0;

        while (total_written < bytes_read)
        {
            ssize_t n = write(
                fd2,
                buffer + total_written,
                bytes_read - total_written
            );

            if (n == -1)
            {
                perror("write");
                close(fd1);
                close(fd2);
                return 1;
            }

            total_written += n;
        }
    }

    if (bytes_read == -1)
    {
        perror("read");
        close(fd1);
        close(fd2);
        return 1;
    }

    close(fd1);
    close(fd2);

    return 0;
}

/*
 * Test:
 * echo "Hello from file1" > file1
 * gcc question_7.c -o question_7
 * ./question_7
 * cat file2
 * cmp file1 file2
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_7.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ echo "Hello from file1" > file1
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat file1
Hello from file1
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_7.c -o question_7
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_7
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat file2
Hello from file1
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cmp file1 file2
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ md5sum file1 file2
f0d4f2a0941e856a271ec529ddbc73ee  file1
f0d4f2a0941e856a271ec529ddbc73ee  file2

*/