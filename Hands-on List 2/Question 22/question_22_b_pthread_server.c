/*
 * Question 22(b): Concurrent TCP Server Using pthread_create()
 * Each accepted client is handled by a detached worker thread.
 *
 * Test:
 *   gcc question_22_b_pthread_server.c -o question_22_b_pthread_server -pthread
 *   ./question_22_b_pthread_server
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
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

void *handle_client(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char reply[BUFFER_SIZE + 100];

    ssize_t n = receive_line(client_fd, buffer, sizeof(buffer));

    if (n == -1) {
        perror("recv");
    } else if (n == 0) {
        printf("Thread %lu: client disconnected without sending data.\n",
               (unsigned long)pthread_self());
    } else {
        printf("Thread %lu received: %s",
               (unsigned long)pthread_self(), buffer);
        fflush(stdout);

        int length = snprintf(reply, sizeof(reply),
                              "Server handled your message: %s", buffer);

        if (length < 0 || (size_t)length >= sizeof(reply)) {
            fprintf(stderr, "Reply is too large.\n");
        } else if (send_all(client_fd, reply, (size_t)length) == -1) {
            perror("send");
        }
    }

    close(client_fd);
    return NULL;
}

int main(void)
{
    int server_fd;
    struct sockaddr_in server_addr;
    pthread_attr_t attr;

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

    if (pthread_attr_init(&attr) != 0) {
        fprintf(stderr, "pthread_attr_init failed.\n");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        fprintf(stderr, "pthread_attr_setdetachstate failed.\n");
        pthread_attr_destroy(&attr);
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Thread-based server listening on port %d...\n", PORT);
    fflush(stdout);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1) {
            if (errno == EINTR)
                continue;

            perror("accept");
            continue;
        }

        int *arg = malloc(sizeof(*arg));

        if (arg == NULL) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        *arg = client_fd;

        pthread_t thread;
        int rc = pthread_create(&thread, &attr, handle_client, arg);

        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            close(client_fd);
            free(arg);
            continue;
        }

        printf("Created thread %lu for a client.\n",
               (unsigned long)thread);
        fflush(stdout);
    }

    pthread_attr_destroy(&attr);
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
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_22_b_pthread_server
Thread-based server listening on port 5000...

Terminal 2:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ printf 'Hello from the threaded client!\n' | nc -N 127.0.0.1 5000
Server handled your message: Hello from the threaded client!

Terminal 1:
Created thread 134757241321152 for a client.
Thread 134757241321152 received: Hello from the threaded client!
*/