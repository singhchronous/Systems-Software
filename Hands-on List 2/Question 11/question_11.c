#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    const char *path = "msgqueue_keyfile";

    /* ftok() requires an existing filesystem path. */
    int fd = open(path, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("open key file");
        return EXIT_FAILURE;
    }
    close(fd);

    /* Generate a System V IPC key from the path and project ID. */
    key_t key = ftok(path, 'A');
    if (key == (key_t)-1) {
        perror("ftok");
        return EXIT_FAILURE;
    }

    /* Create the queue if absent; otherwise access the existing queue. */
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        return EXIT_FAILURE;
    }

    printf("Message queue key: %ld\n", (long)key);
    printf("Message queue ID: %d\n", msgid);

    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_11.c -o question_11
 * ./question_11
 *
 * Expected output:
 * Message queue key: <system-dependent key>
 * Message queue ID: <system-dependent ID>
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_11.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_11.c -o question_11
./question_11
Message queue key: 1090651476
Message queue ID: 0
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_11
Message queue key: 1090651476
Message queue ID: 0
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    
0x41020554 0          shubham-si 666        0            0           

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q -i 0

Message Queue msqid=0
uid=1000	gid=1000	cuid=1000	cgid=1000	mode=0666
cbytes=0	qbytes=16384	qnum=0	lspid=0	lrpid=0
send_time=Not set                   
rcv_time=Not set                   
change_time=Mon Oct 19 20:44:24 2026  
*/