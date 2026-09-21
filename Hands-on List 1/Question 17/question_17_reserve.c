#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;
    int ticket;
    struct flock lock;

    /* Open the ticket file for reading and writing */
    fd = open("ticket.txt", O_RDWR);

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

    printf("Process %d: Waiting for lock...\n", getpid());

    /* Wait until the write lock can be acquired */
    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        perror("fcntl");
        close(fd);
        return 1;
    }

    printf("Process %d: Lock acquired.\n", getpid());

    /* Read the current ticket number */
    if (read(fd, &ticket, sizeof(ticket)) != sizeof(ticket))
    {
        perror("read");
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        close(fd);
        return 1;
    }

    /* Increment the ticket number */
    ticket++;

    /* Move the file offset back to the beginning */
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1)
    {
        perror("lseek");
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        close(fd);
        return 1;
    }

    /* Write the updated ticket number back to the file */
    if (write(fd, &ticket, sizeof(ticket)) != sizeof(ticket))
    {
        perror("write");
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        close(fd);
        return 1;
    }

    printf("Process %d: New ticket number = %d\n", getpid(), ticket);

    /* Release the write lock */
    lock.l_type = F_UNLCK;

    if (fcntl(fd, F_SETLK, &lock) == -1)
    {
        perror("unlock");
        close(fd);
        return 1;
    }

    close(fd);

    return 0;
}

/*
 * Test:
 * gcc question_17_reserve.c -o question_17_reserve
 *
 * First initialize the ticket file:
 * ./question_17_init
 *
 * Reserve one ticket:
 * ./question_17_reserve
 *
 * To test concurrent reservations:
 * ./question_17_init
 * ./question_17_reserve &
 * ./question_17_reserve &
 * ./question_17_reserve &
 * ./question_17_reserve &
 * ./question_17_reserve &
 * wait
 *
 * Expected:
 * Five different ticket numbers should be generated:
 * 101, 102, 103, 104 and 105.
 *
 * The order of the processes is not guaranteed, but no ticket
 * number should be assigned twice.
 */

/*

Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_17_init.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_17_reserve.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_17_init.c -o question_17_init
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_17_reserve.c -o question_17_reserve
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_17_init
Initial ticket number: 100
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ls -l ticket.txt
-rw-r--r-- 1 shubham-singh shubham-singh 4 Sep 21 19:36 ticket.txt
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_17_reserve
Process 5260: Waiting for lock...
Process 5260: Lock acquired.
Process 5260: New ticket number = 101
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_17_reserve
Process 5270: Waiting for lock...
Process 5270: Lock acquired.
Process 5270: New ticket number = 102
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_17_init
Initial ticket number: 100
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_17_reserve &
./question_17_reserve &
./question_17_reserve &
./question_17_reserve &
./question_17_reserve &
wait
[1] 5287
[2] 5288
[3] 5289
Process 5287: Waiting for lock...
Process 5287: Lock acquired.
[4] 5290
Process 5287: New ticket number = 101
Process 5288: Waiting for lock...
Process 5288: Lock acquired.
Process 5288: New ticket number = 102
[5] 5291
Process 5289: Waiting for lock...
Process 5289: Lock acquired.
Process 5289: New ticket number = 103
Process 5290: Waiting for lock...
Process 5290: Lock acquired.
Process 5290: New ticket number = 104
Process 5291: Waiting for lock...
Process 5291: Lock acquired.
Process 5291: New ticket number = 105

*/