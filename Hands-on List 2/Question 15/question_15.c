#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define KEY_FILE "msgqueue_keyfile"
#define PROJECT_ID 'A'
#define NEW_PERMISSIONS 0644

int main(void) {
    /* Prepare the file used to derive the IPC key. */
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

    /* Access the existing queue; do not create a new one. */
    int msgid = msgget(key, 0);
    if (msgid == -1) {
        perror("msgget");
        fprintf(stderr, "Create the queue first using Question 11 or 13.\n");
        return 1;
    }

    struct msqid_ds info;

    /* Read the current queue metadata. */
    if (msgctl(msgid, IPC_STAT, &info) == -1) {
        perror("msgctl IPC_STAT");
        return 1;
    }

    printf("Queue ID: %d\n", msgid);
    printf("Original permissions: %04o\n",
           (unsigned int)(info.msg_perm.mode & 0777));

    /* Replace the access permission bits with 0644. */
    info.msg_perm.mode =
        (info.msg_perm.mode & ~0777) | NEW_PERMISSIONS;

    if (msgctl(msgid, IPC_SET, &info) == -1) {
        perror("msgctl IPC_SET");
        return 1;
    }

    /* Read the metadata again to verify the update. */
    if (msgctl(msgid, IPC_STAT, &info) == -1) {
        perror("msgctl IPC_STAT");
        return 1;
    }

    printf("Updated permissions: %04o\n",
           (unsigned int)(info.msg_perm.mode & 0777));

    return 0;
}

/*
Test:
    gcc question_15.c -o question_15
    ./question_15
    ipcs -q
    ipcs -q -i <queue_id>
*/

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_15.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    
0x41020554 0          shubham-si 666        0            0           

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_11.c -o question_11
./question_11
Message queue key: 1090651476
Message queue ID: 0
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    
0x41020554 0          shubham-si 666        0            0           

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_15.c -o question_15
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_15
Queue ID: 0
Original permissions: 0666
Updated permissions: 0644
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    
0x41020554 0          shubham-si 644        0            0           

shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ipcs -q -i 0

Message Queue msqid=0
uid=1000	gid=1000	cuid=1000	cgid=1000	mode=0644
cbytes=0	qbytes=16384	qnum=0	lspid=9189	lrpid=9191
send_time=Mon Oct 19 23:48:27 2026  
rcv_time=Mon Oct 19 23:48:29 2026  
change_time=Mon Oct 19 23:54:35 2026  
*/