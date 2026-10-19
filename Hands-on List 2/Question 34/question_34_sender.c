#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <receiver_pid>\n", argv[0]);
        return 1;
    }

    char *end;
    errno = 0;
    long value = strtol(argv[1], &end, 10);

    if (errno != 0 || *argv[1] == '\0' || *end != '\0' ||
        value <= 0 || value > 2147483647L) {
        fprintf(stderr, "Invalid PID.\n");
        return 1;
    }

    pid_t receiver_pid = (pid_t)value;

    printf("Sending SIGSTOP to PID %ld...\n", (long)receiver_pid);

    if (kill(receiver_pid, SIGSTOP) == -1) {
        perror("kill");
        return 1;
    }

    printf("SIGSTOP sent successfully.\n");
    return 0;
}

/