/*
 * Question 21: Inter-Machine Communication Using TCP Sockets
 * Server: binds to port 5000, accepts one client, receives a
 * newline-terminated message, sends a reply, and closes sockets.
 *
 * Test:
 *   gcc question_21_server.c -o question_21_server
 *   ./question_21_server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 5000
#define BUFFER_SIZE 1024

int send_all(int fd, const char *data, size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        ssize_t n = send(fd, data + sent, length - sent, 0);

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

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    const char reply[] = "Hello from the server!\n";

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

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d...\n", PORT);
    fflush(stdout);

    client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                       &client_len);

    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Client connected: %s\n",
           inet_ntoa(client_addr.sin_addr));

    ssize_t received = receive_line(client_fd, buffer, sizeof(buffer));

    if (received == -1) {
        perror("recv");
        close(client_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (received == 0) {
        printf("Client disconnected without sending a message.\n");
    } else {
        printf("Received from client: %s", buffer);

        if (send_all(client_fd, reply, strlen(reply)) == -1) {
            perror("send");
            close(client_fd);
            close(server_fd);
            return EXIT_FAILURE;
        }

        printf("Reply sent to client.\n");
    }

    close(client_fd);
    close(server_fd);
    return EXIT_SUCCESS;
}

/*
Output:
Terminal 1:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_21_server.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_21_client.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_21_server.c -o question_21_server
gcc question_21_client.c -o question_21_client
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_21_server
Server listening on port 5000...

Terminal 2:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_21_client
Connected to server.
Sent to server: Hello from the client!
Received from server: Hello from the server!

Terminal 1:
Client connected: 127.0.0.1
Received from client: Hello from the client!
Reply sent to client.
*/