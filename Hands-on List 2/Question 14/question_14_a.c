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
    printf("Waiting for a message of type 1...\n");
    fflush(stdout);

    ssize_t n = msgrcv(msgid, &msg, sizeof(msg.mtext) - 1, 1, 0);
    if (n == -1) {
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
    gcc question_14_a.c -o question_14_a
    ./question_14_a
    # In another terminal, send a message using Question 13.
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_14_a.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_14_a.c -o question_14_a
./question_14_a
Waiting for a message of type 1...
Message received successfully.
Message type: 1
Message: Hello from Question 13!
Bytes received: 24
*/