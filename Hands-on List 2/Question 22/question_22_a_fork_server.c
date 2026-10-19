/*
 * Question 22(a): Concurrent TCP Server Using fork()
 * Each accepted client is handled by a separate child process.
 *
 * Test:
 *   gcc question_22_a_fork_server.c -o question_22_a_fork_server
 *   ./question_22_a_fork_server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#define PORT 5000
#define BUFFER_SIZE 1024

int send_all(int fd, const char *data, size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        ssize_t n = send(fd, data + sent, length - sent, MSG_NOSIGNAL);

        if (n == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0)
            return -1;

        sent += (size_t)n;
    }

    return 0;
}

ssize_t receive_line(int fd, char *buffer, size_t capacity)
{
    size_t used = 0;

    if (capacity < 2)
        return -1;

    while (used < capacity - 1) {
        char ch;
        ssize_t n = recv(fd, &ch, 1, 0);

        if (n == 0)
            break;

        if (n == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        buffer[used++] = ch;

        if (ch == '\n')
            break;
    }

    buffer[used] = '\0';
    return (ssize_t)used;
}

void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];
    char reply[BUFFER_SIZE + 100];

    ssize_t n = receive_line(client_fd, buffer, sizeof(buffer));

    if (n == -1) {
        perror("recv");
        return;
    }

    if (n == 0) {
        printf("Child %ld: client disconnected without sending data.\n",
               (long)getpid());
        return;
    }

    printf("Child %ld received: %s", (long)getpid(), buffer);
    fflush(stdout);

    int length = snprintf(reply, sizeof(reply),
                          "Server handled your message: %s", buffer);

    if (length < 0 || (size_t)length >= sizeof(reply)) {
        fprintf(stderr, "Reply is too large.\n");
        return;
    }

    if (send_all(client_fd, reply, (size_t)length) == -1)
        perror("send");
}

int main(void)
{
    int server_fd;
    struct sockaddr_in server_addr;

    /*
     * Automatically reap terminated children on Linux/POSIX systems
     * supporting SIGCHLD with SIG_IGN semantics.
     */
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Fork-based server listening on port %d...\n", PORT);
    fflush(stdout);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1) {
            if (errno == EINTR)
                continue;

            perror("accept");
            continue;
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            /* Child: it does not need the listening socket. */
            close(server_fd);

            handle_client(client_fd);

            close(client_fd);
            _exit(EXIT_SUCCESS);
        }

        /* Parent: the child handles this connected socket. */
        close(client_fd);
        printf("Created child process %ld for a client.\n", (long)pid);
        fflush(stdout);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_22_a_fork_server.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_22_b_pthread_server.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_22_a_fork_server.c -o question_22_a_fork_server
gcc question_22_b_pthread_server.c -o question_22_b_pthread_server -pthread

Terminal 1:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_22_a_fork_server
Fork-based server listening on port 5000...

Terminal 2:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ printf 'Hello from client 1!\n' | nc -N 127.0.0.1 5000
Server handled your message: Hello from client 1!

Terminal 1:
Created child process 10125 for a client.
Child 10125 received: Hello from client 1!

Terminal 2:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ printf 'Message from client A\n' | nc -N 127.0.0.1 5000
Server handled your message: Message from client A

Terminal 1:
Created child process 10131 for a client.
Child 10131 received: Message from client A

Terminal 3:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ printf 'Message from client B\n' | nc -N 127.0.0.1 5000
Server handled your message: Message from client B

Terminal 1:
Created child process 10244 for a client.
Child 10244 received: Message from client B
*/