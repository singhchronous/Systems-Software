#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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

    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        return 1;
    }

    struct message msg;
    printf("Checking for a message of type 1...\n");

    ssize_t n = msgrcv(msgid, &msg, sizeof(msg.mtext) - 1,
                       1, IPC_NOWAIT);

    if (n == -1) {
        if (errno == ENOMSG) {
            printf("No message of type 1 available. "
                   "Receiver did not wait.\n");
            return 0;
        }

        perror("msgrcv");
        return 1;
    }

    msg.mtext[n] = '\0';

    printf("Message received successfully.\n");
    printf("Message type: %ld\n", msg.mtype);
    printf("Message: %s\n", msg.mtext);
    printf("Bytes received: %zd\n", n);

    return 0;
}

/*
Test:
    gcc question_14_b.c -o question_14_b
    ./question_14_b
    ipcs -q
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_14_b.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_14_b.c -o question_14_b
./question_14_b
Checking for a message of type 1...
No message of type 1 available. Receiver did not wait.
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_13
Message sent successfully.
Message queue ID: 0
Message type: 1
Message: Hello from Question 13!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_14_b
Checking for a message of type 1...
Message received successfully.
Message type: 1
Message: Hello from Question 13!
Bytes received: 24
*/
