#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

struct record
{
    int id;
    char name[20];
};

int main(void)
{
    int fd;
    struct record records[3] = {
        {1, "Alice"},
        {2, "Bob"},
        {3, "Charlie"}
    };

    /* Create the file and store three fixed-size records */
    fd = open("records.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    /* Write all three records to the file */
    if (write(fd, records, sizeof(records)) != sizeof(records))
    {
        perror("write");
        close(fd);
        return 1;
    }

    printf("Three records created successfully.\n");

    close(fd);

    return 0;
}

/*
 * Test:
 * gcc question_18_init.c -o question_18_init
 * ./question_18_init
 *
 * Expected output:
 * Three records created successfully.
 *
 * This creates records.dat containing three records:
 * Record 0 -> ID 1, Alice
 * Record 1 -> ID 2, Bob
 * Record 2 -> ID 3, Charlie
 */

/*

Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_18_init.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_18_init.c -o question_18_init
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_18_init
Three records created successfully.


*/