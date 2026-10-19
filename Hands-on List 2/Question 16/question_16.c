#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define KEY_FILE "msgqueue_keyfile"
#define PROJECT_ID 'A'

int main(void) {
    /* Obtain the key used by the existing message queue. */
    key_t key = ftok(KEY_FILE, PROJECT_ID);
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    /* Find the queue without creating a new one. */
    int msgid = msgget(key, 0);
    if (msgid == -1) {
        perror("msgget");
        fprintf(stderr, "Message queue does not exist.\n");
        return 1;
    }

    printf("Message queue ID: %d\n", msgid);

    /* Remove the queue and discard any messages still queued. */
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl IPC_RMID");
        return 1;
    }

    printf("Message queue removed successfully.\n");

    return 0;
}

/*
Test:
    gcc question_16.c -o question_16
    ./question_16
    ipcs -q
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_16.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    
0x41020554 0          shubham-si 644        0            0           

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_11.c -o question_11
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_11
Message queue key: 1090651476
Message queue ID: 0
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_16.c -o question_16
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_16
Message queue ID: 0
Message queue removed successfully.
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q -i 0
ipcs: id 0 not found
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_16
msgget: No such file or directory
Message queue does not exist.
*/