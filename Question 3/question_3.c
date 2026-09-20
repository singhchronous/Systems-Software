#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * Question 3:
 * Create a file using the creat() system call and print
 * the file descriptor returned by the kernel.
 *
 * creat() creates the file if it does not exist and opens
 * it for writing. If the file already exists, its contents
 * are truncated.
 */

int main(void)
{
    int fd;

    fd = creat("example.txt", 0644);

    if (fd == -1)
    {
        perror("creat");
        return 1;
    }

    printf("File descriptor: %d\n", fd);

    /*
     * The file descriptor is a small integer used by the
     * process to refer to the open file.
     */
    close(fd);

    return 0;
}

/*
 * Test:
 *
 *     gcc question_3.c -o question_3
 *     ./question_3
 *
 * Expected output:
 *
 *     File descriptor: 3
 *
 * The exact value is not guaranteed to be 3; it is normally
 * the lowest available file descriptor.
 *
 * Verify the created file:
 *
 *     ls -l example.txt
 * 
 * Output:
 * 
 * shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_3
File descriptor: 3
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ls -l example.txt
-rw-r--r-- 1 shubham-singh shubham-singh 0 Sep 20 16:50 example.txt
 */