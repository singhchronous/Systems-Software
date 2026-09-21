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

    /* Open the records file for reading and writing */
    fd = open("records.dat", O_RDWR);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    /* Configure an exclusive lock for the selected record */
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = record_no * sizeof(struct record);
    lock.l_len = sizeof(struct record);
    lock.l_pid = getpid();

    printf("Process %d: Waiting for write lock on record %d...\n",
           getpid(), record_no);

    /* Wait until the selected record can be locked */
    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        perror("fcntl");
        close(fd);
        return 1;
    }

    printf("Process %d: Write lock acquired on record %d.\n",
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

    printf("Current record: ID = %d, Name = %s\n",
           record.id, record.name);

    /* Modify the record */
    record.id++;

    /* Move back and write the modified record */
    if (lseek(fd, lock.l_start, SEEK_SET) == (off_t)-1)
    {
        perror("lseek");
        close(fd);
        return 1;
    }

    if (write(fd, &record, sizeof(record)) != sizeof(record))
    {
        perror("write");
        close(fd);
        return 1;
    }

    printf("Modified record: ID = %d, Name = %s\n",
           record.id, record.name);

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
 * gcc question_18_write.c -o question_18_write
 *
 * First create the records:
 * ./question_18_init
 *
 * Modify record 0:
 * ./question_18_write 0
 *
 * Expected:
 * Current record: ID = 1, Name = Alice
 * Modified record: ID = 2, Name = Alice
 *
 * To test record-level locking, run the same command from
 * two terminals. The second process should wait while the
 * first process holds the write lock on the same record.
 *
 * Processes locking different records should not conflict.
 */

/*

Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_18_a.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_18_a.c -o question_18_a
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
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_18_a 0
Process 6349: Waiting for write lock on record 0...
Process 6349: Write lock acquired on record 0.
Current record: ID = 1, Name = Alice
Modified record: ID = 2, Name = Alice
Record 0 unlocked.

*/