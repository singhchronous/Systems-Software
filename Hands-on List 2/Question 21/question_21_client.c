/*
 * Question 21: Inter-Machine Communication Using TCP Sockets
 * Client: connects to the configured server IPv4 address and port,
 * sends a newline-terminated message, receives the server reply,
 * and closes its socket.
 *
 * Local test:
 *   gcc question_21_client.c -o question_21_client
 *   ./question_21_client
 *
 * For different machines, replace 127.0.0.1 with the server's
 * reachable IPv4 address before compiling.
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
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    const char message[] = "Hello from the client!\n";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    /* Use loopback for testing both programs on the same machine. */
    if (inet_pton(AF_INET, "127.0.0.1",
                  &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IP address.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("Connected to server.\n");

    if (send_all(sockfd, message, strlen(message)) == -1) {
        perror("send");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("Sent to server: %s", message);

    ssize_t received = receive_line(sockfd, buffer, sizeof(buffer));

    if (received == -1) {
        perror("recv");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (received == 0) {
        printf("Server closed the connection without a reply.\n");
    } else {
        printf("Received from server: %s", buffer);
    }

    close(sockfd);
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