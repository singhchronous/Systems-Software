#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

struct record
{
    int id;
    char name[20];
};

int main(int argc, char *argv[])
{
    int fd;
    int record_no;
    struct record record;
    struct flock lock;

    /* Get the record number from the command line */
    if (argc != 2)
    {
        printf("Usage: %s <record_number>\n", argv[0]);
        return 1;
    }

    record_no = atoi(argv[1]);

    if (record_no < 0 || record_no > 2)
    {
        printf("Record number must be 0, 1 or 2.\n");
        return 1;
    }

    /* Open the records file for reading */
    fd = open("records.dat", O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    /* Configure a shared read lock for the selected record */
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = record_no * sizeof(struct record);
    lock.l_len = sizeof(struct record);
    lock.l_pid = getpid();

    printf("Process %d: Waiting for read lock on record %d...\n",
           getpid(), record_no);

    /* Wait until the selected record can be locked */
    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        perror("fcntl");
        close(fd);
        return 1;
    }

    printf("Process %d: Read lock acquired on record %d.\n",
           getpid(), record_no);

    /* Move to the selected record and read it */
    if (lseek(fd, lock.l_start, SEEK_SET) == (off_t)-1)
    {
        perror("lseek");
        close(fd);
        return 1;
    }

    if (read(fd, &record, sizeof(record)) != sizeof(record))
    {
        perror("read");
        close(fd);
        return 1;
    }

    printf("Record %d: ID = %d, Name = %s\n",
           record_no, record.id, record.name);

    /* Release the record lock */
    lock.l_type = F_UNLCK;

    if (fcntl(fd, F_SETLK, &lock) == -1)
    {
        perror("unlock");
        close(fd);
        return 1;
    }

    printf("Record %d unlocked.\n", record_no);

    close(fd);

    return 0;
}

/*
 * Test:
 * gcc question_18_read.c -o question_18_read
 *
 * First create the records:
 * ./question_18_init
 *
 * Read record 1:
 * ./question_18_read 1
 *
 * Expected:
 * Record 1: ID = 2, Name = Bob
 *
 * Run the read program simultaneously from two terminals
 * for the same record. Both read locks should be allowed.
 *
 * Run a write-lock program on the same record while a read
 * lock is held. The writer should wait until the read lock
 * is released.
 */

/*

Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_18_init.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_18_init.c -o question_18_init
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_18_b.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_18_b.c -o question_18_b
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_18_init
Three records created successfully.
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_18_b 0
Process 6343: Waiting for read lock on record 0...
Process 6343: Read lock acquired on record 0.
Record 0: ID = 1, Name = Alice
Record 0 unlocked.

*/