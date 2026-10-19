#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define KEY_FILE "msgqueue_keyfile"
#define PROJECT_ID 'A'

struct message {
    long mtype;
    char mtext[100];
};

int main(void) {
    /* Create or open the file used to generate the IPC key. */
    int fd = open(KEY_FILE, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    close(fd);

    key_t key = ftok(KEY_FILE, PROJECT_ID);
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    /* Obtain the queue associated with this key. */
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        return 1;
    }

    struct message msg;
    msg.mtype = 1;
    snprintf(msg.mtext, sizeof(msg.mtext),
             "Hello from Question 13!");

    /* The payload size excludes the leading long mtype. */
    if (msgsnd(msgid, &msg, strlen(msg.mtext) + 1, 0) == -1) {
        perror("msgsnd");
        return 1;
    }

    printf("Message sent successfully.\n");
    printf("Message queue ID: %d\n", msgid);
    printf("Message type: %ld\n", msg.mtype);
    printf("Message: %s\n", msg.mtext);

    return 0;
}

/*
Test:
    gcc question_13.c -o question_13
    ./question_13
    ipcs -q
    ipcs -q -i <queue_id>
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_13.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_13.c -o question_13
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_13
Message sent successfully.
Message queue ID: 0
Message type: 1
Message: Hello from Question 13!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    
0x41020554 0          shubham-si 666        24           1           

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q -i 0

Message Queue msqid=0
uid=1000	gid=1000	cuid=1000	cgid=1000	mode=0666
cbytes=24	qbytes=16384	qnum=1	lspid=9081	lrpid=0
send_time=Mon Oct 19 23:36:36 2026  
rcv_time=Not set                   
change_time=Mon Oct 19 20:44:24 2026  

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    
0x41020554 0          shubham-si 666        24           1           
*/