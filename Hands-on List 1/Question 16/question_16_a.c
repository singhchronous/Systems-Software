#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;
    struct flock lock;

    /* Open the file for reading and writing */
    fd = open("file1", O_RDWR | O_CREAT, 0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    /* Configure an exclusive write lock for the entire file */
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    printf("Requesting write lock...\n");

    /* Wait until the write lock can be acquired */
    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        perror("fcntl");
        close(fd);
        return 1;
    }

    printf("Write lock acquired.\n");
    printf("PID = %d\n", getpid());
    printf("Press Enter to release the lock...\n");

    getchar();

    /* Release the write lock */
    lock.l_type = F_UNLCK;

    if (fcntl(fd, F_SETLK, &lock) == -1)
    {
        perror("unlock");
        close(fd);
        return 1;
    }

    printf("Write lock released.\n");

    close(fd);

    return 0;
}

/*
 * Test:
 * gcc question_16_write.c -o question_16_write
 * ./question_16_write
 *
 * In another terminal, run:
 * ./question_16_write
 *
 * The second process should wait because the first process
 * holds an exclusive write lock on the file.
 *
 * Press Enter in the first terminal to release the lock.
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_16_a.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_16_a.c -o question_16_a
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_16_a
Requesting write lock...
Write lock acquired.
PID = 4971
Press Enter to release the lock...

[Terminal 2:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ pgrep question_16_a
4971
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat /proc/locks
1: FLOCK  ADVISORY  WRITE 1130 08:02:1973321 0 EOF
2: FLOCK  ADVISORY  READ 1364 00:1c:3608 0 EOF
3: FLOCK  ADVISORY  WRITE 1171 00:1c:2163 0 EOF
4: POSIX  ADVISORY  WRITE 4971 08:02:2360487 0 EOF

]

Write lock released.

*/

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_16_a
Requesting write lock...
Write lock acquired.
PID = 5025
Press Enter to release the lock...

[Terminal 2:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_16_a
Requesting write lock...

]
Write lock released.

[Terminal 2:

Write lock acquired.
PID = 5031
Press Enter to release the lock...

Write lock released.

]

*/