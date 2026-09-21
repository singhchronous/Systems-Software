#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;
    int ticket = 100;

    /* Create the ticket file and open it for writing */
    fd = open("ticket.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    /* Store the initial ticket number in the file */
    if (write(fd, &ticket, sizeof(ticket)) != sizeof(ticket))
    {
        perror("write");
        close(fd);
        return 1;
    }

    printf("Initial ticket number: %d\n", ticket);

    close(fd);

    return 0;
}

/*
 * Test:
 * gcc question_17_init.c -o question_17_init
 * ./question_17_init
 *
 * Expected output:
 * Initial ticket number: 100
 *
 * This creates ticket.txt and stores the initial ticket number.
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_17_init.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_17_init.c -o question_17_init
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_17_init
Initial ticket number: 100
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ls -l ticket.txt
-rw-r--r-- 1 shubham-singh shubham-singh 4 Sep 21 19:36 ticket.txt

*/