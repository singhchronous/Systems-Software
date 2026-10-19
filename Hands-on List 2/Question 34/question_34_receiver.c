#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

void handler(int signo)
{
    const char msg[] = "SIGSTOP handler executed.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    printf("Receiver PID: %ld\n", (long)getpid());
    fflush(stdout);

    if (sigaction(SIGSTOP, &sa, NULL) == -1) {
        perror("Cannot register SIGSTOP handler");
    }

    printf("Receiver is waiting. Send SIGSTOP from the other program.\n");
    fflush(stdout);

    while (1)
        pause();

    return 0;
}

/*
Output:

Terminal 1:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_34_receiver.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_34_sender.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_34_receiver.c -o question_34_receiver
gcc question_34_sender.c -o question_34_sender
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_34_receiver
Receiver PID: 11603
Cannot register SIGSTOP handler: Invalid argument
Receiver is waiting. Send SIGSTOP from the other program.

Terminal 2:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_34_sender 11603
Sending SIGSTOP to PID 11603...
SIGSTOP sent successfully.

Terminal 1:
[1]+  Stopped                    ./question_34_receiver

Terminal 2:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ps -o pid,ppid,stat,cmd -p 11603
    PID    PPID STAT CMD
  11603    8104 T    ./question_34_receiver
*/