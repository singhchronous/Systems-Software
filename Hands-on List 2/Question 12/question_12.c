#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>

static void print_time(const char *label, time_t value)
{
    /* A zero timestamp means the corresponding event is not recorded. */
    if (value == 0) {
        printf("%s: Not recorded (0)\n", label);
        return;
    }

    struct tm *tm_info = localtime(&value);

    if (tm_info == NULL) {
        printf("%s: %lld (epoch seconds)\n",
               label, (long long)value);
        return;
    }

    char formatted[64];

    if (strftime(formatted, sizeof(formatted),
                 "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        printf("%s: %lld (epoch seconds)\n",
               label, (long long)value);
        return;
    }

    printf("%s: %s\n", label, formatted);
}

int main(void)
{
    const char *path = "msgqueue_keyfile";

    /* Ensure the pathname required by ftok() exists. */
    int fd = open(path, O_CREAT | O_RDWR, 0666);

    if (fd == -1) {
        perror("open key file");
        return EXIT_FAILURE;
    }
    close(fd);

    /* Derive the key using the same convention as Question 11. */
    key_t key = ftok(path, 'A');

    if (key == (key_t)-1) {
        perror("ftok");
        return EXIT_FAILURE;
    }

    /* Create the queue if absent, or access the existing queue. */
    int msgid = msgget(key, IPC_CREAT | 0666);

    if (msgid == -1) {
        perror("msgget");
        return EXIT_FAILURE;
    }

    struct msqid_ds info;

    /* IPC_STAT fills msqid_ds with the queue's current metadata. */
    if (msgctl(msgid, IPC_STAT, &info) == -1) {
        perror("msgctl IPC_STAT");
        return EXIT_FAILURE;
    }

    printf("Message Queue Information\n");
    printf("-------------------------\n");
    printf("Queue key: %ld\n", (long)key);
    printf("Queue ID: %d\n", msgid);

    /* (a) Permission bits. */
    printf("\n(a) Access permission: %04o\n",
           (unsigned int)(info.msg_perm.mode & 0777));

    /* (b) Current owner and original creator credentials. */
    printf("\n(b) Owner UID: %u\n", (unsigned int)info.msg_perm.uid);
    printf("    Owner GID: %u\n", (unsigned int)info.msg_perm.gid);
    printf("    Creator UID: %u\n", (unsigned int)info.msg_perm.cuid);
    printf("    Creator GID: %u\n", (unsigned int)info.msg_perm.cgid);

    /* (c) Last send/receive timestamps and associated process IDs. */
    printf("\n(c) Last message sent:\n");
    print_time("    Send time", info.msg_stime);
    printf("    Sender PID: %d\n", (int)info.msg_lspid);

    printf("    Last message received:\n");
    print_time("    Receive time", info.msg_rtime);
    printf("    Receiver PID: %d\n", (int)info.msg_lrpid);

    /* (d) Time at which the queue was last changed. */
    printf("\n(d) Last queue change:\n");
    print_time("    Change time", info.msg_ctime);

    /* (e) Linux-specific current byte usage. */
    printf("\n(e) Current queue size: %llu bytes\n",
           (unsigned long long)info.__msg_cbytes);

    /* (f) Number of messages currently waiting in the queue. */
    printf("\n(f) Number of messages: %llu\n",
           (unsigned long long)info.msg_qnum);

    /* (g) Configured maximum queue capacity in bytes. */
    printf("\n(g) Maximum bytes allowed: %llu\n",
           (unsigned long long)info.msg_qbytes);

    /* (h) Most recent sender and receiver process IDs. */
    printf("\n(h) Last sender PID: %d\n", (int)info.msg_lspid);
    printf("    Last receiver PID: %d\n", (int)info.msg_lrpid);

    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_12.c -o question_12
 * ./question_12
 *
 * Expected output:
 * Queue key/ID, permissions, owner and creator UID/GID,
 * timestamps, current queue size, message count, byte limit,
 * and last sender/receiver PIDs.
 * Values depend on the existing queue and system configuration.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_!2.c
vim question_sudo apt update.c
3 files to edit
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_12.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_12.c -o question_12
./question_12
Message Queue Information
-------------------------
Queue key: 1090651476
Queue ID: 0

(a) Access permission: 0666

(b) Owner UID: 1000
    Owner GID: 1000
    Creator UID: 1000
    Creator GID: 1000

(c) Last message sent:
    Send time: Not recorded (0)
    Sender PID: 0
    Last message received:
    Receive time: Not recorded (0)
    Receiver PID: 0

(d) Last queue change:
    Change time: 2026-10-19 20:44:24

(e) Current queue size: 0 bytes

(f) Number of messages: 0

(g) Maximum bytes allowed: 16384

(h) Last sender PID: 0
    Last receiver PID: 0
*/
