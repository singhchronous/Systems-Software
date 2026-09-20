#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;
    char line[1024];
    int i = 0;
    char ch;
    ssize_t n;

    /* Open file in read-only mode */
    fd = open("file1", O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    /* Read characters and display each line */
    while ((n = read(fd, &ch, 1)) > 0)
    {
        line[i++] = ch;

        if (ch == '\n' || i == sizeof(line) - 1)
        {
            line[i] = '\0';
            write(STDOUT_FILENO, line, i);
            i = 0;
        }
    }

    if (n == -1)
    {
        perror("read");
        close(fd);
        return 1;
    }

    /* Handle a final line without a newline */
    if (i > 0)
    {
        write(STDOUT_FILENO, line, i);
    }

    close(fd);

    return 0;
}

/*
 * Test:
 * echo -e "Line 1\nLine 2\nLine 3" > file1
 * gcc question_8.c -o question_8
 * ./question_8
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_8.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat > file1
Hello Linux
IIIT Bangalore
Systems Softwareshubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat file1
Hello Linux
IIIT Bangalore
Systems Softwareshubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_8.c -o question_8
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_8
Hello Linux
IIIT Bangalore
Systems Software

*/