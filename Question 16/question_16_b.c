#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;
    struct flock lock;

    /* Open the file for reading */
    fd = open("file1", O_RDONLY | O_CREAT, 0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    /* Configure a shared read lock for the entire file */
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    printf("Requesting read lock...\n");

    /* Wait until the read lock can be acquired */
    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        perror("fcntl");
        close(fd);
        return 1;
    }

    printf("Read lock acquired.\n");
    printf("PID = %d\n", getpid());
    printf("Press Enter to release the lock...\n");

    getchar();

    /* Release the read lock */
    lock.l_type = F_UNLCK;

    if (fcntl(fd, F_SETLK, &lock) == -1)
    {
        perror("unlock");
        close(fd);
        return 1;
    }

    printf("Read lock released.\n");

    close(fd);

    return 0;
}

/*
 * Test:
 * gcc question_16_read.c -o question_16_read
 * ./question_16_read
 *
 * In another terminal, run:
 * ./question_16_read
 *
 * Both processes should acquire the read lock because read locks
 * are shared.
 *
 * While a read lock is held, run the write-lock program:
 * ./question_16_write
 *
 * The writer should wait until all read locks are released.
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_16_b.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_16_b.c -o question_16_b
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_16_b
Requesting read lock...
Read lock acquired.
PID = 5076
Press Enter to release the lock...

[Terminal 2:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_16_b
Requesting read lock...
Read lock acquired.
PID = 5081
Press Enter to release the lock...

]

*/

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_16_b
Requesting read lock...
Read lock acquired.
PID = 5102
Press Enter to release the lock...

[Terminal 2:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_16_a
Requesting write lock...

]

Read Lock released.

[Terminal 2:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_16_a
Requesting write lock...
Write lock acquired.
PID = 5111
Press Enter to release the lock...

Write lock released.

]
*/