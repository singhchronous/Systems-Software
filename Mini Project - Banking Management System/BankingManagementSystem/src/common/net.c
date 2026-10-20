/*
 * net.c - Reliable fixed-size message transfer over TCP.
 *
 * TCP is a byte stream (no message boundaries), so one write() may arrive as
 * several read()s. writen()/readn() loop until the whole struct is moved.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "net.h"

ssize_t writen(int fd, const void *buf, size_t n)
{
    const char *p = buf;
    size_t left = n;
    while (left > 0) {
        /* MSG_NOSIGNAL: a closed peer gives EPIPE instead of killing us with SIGPIPE */
        ssize_t w = send(fd, p, left, MSG_NOSIGNAL);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += w;
        left -= (size_t)w;
    }
    return (ssize_t)n;
}

ssize_t readn(int fd, void *buf, size_t n)
{
    char *p = buf;
    size_t left = n;
    while (left > 0) {
        ssize_t r = read(fd, p, left);
        if (r < 0) {
            if (errno == EINTR && left < n) continue;   /* mid-message: keep going */
            return -1;                                  /* EINTR before any byte: let caller see the signal */
        }
        if (r == 0) return 0;                           /* peer closed */
        p += r;
        left -= (size_t)r;
    }
    return (ssize_t)n;
}

int send_request(int fd, const Request *req)
{
    return writen(fd, req, sizeof *req) == (ssize_t)sizeof *req ? 0 : -1;
}

int recv_request(int fd, Request *req)
{
    ssize_t r = readn(fd, req, sizeof *req);
    if (r == (ssize_t)sizeof *req) {
        /* never trust strings from the network: force NUL termination */
        req->s1[FIELD_LEN - 1] = req->s2[FIELD_LEN - 1] = '\0';
        req->s3[FIELD_LEN - 1] = req->s4[FIELD_LEN - 1] = '\0';
        req->text[TEXT_LEN - 1] = '\0';
        return 1;
    }
    return r == 0 ? 0 : -1;
}

int send_response(int fd, const Response *resp)
{
    return writen(fd, resp, sizeof *resp) == (ssize_t)sizeof *resp ? 0 : -1;
}

int recv_response(int fd, Response *resp)
{
    ssize_t r;
    do {
        r = readn(fd, resp, sizeof *resp);
    } while (r < 0 && errno == EINTR);
    if (r == (ssize_t)sizeof *resp) {
        resp->text[RESP_TEXT_LEN - 1] = '\0';
        return 1;
    }
    return r == 0 ? 0 : -1;
}

int connect_to_server(const char *ip, int port)
{
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) { perror("socket"); return -1; }
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof serv);
    serv.sin_family = AF_INET;
    serv.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip, &serv.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IP address: %s\n", ip);
        close(sd);
        return -1;
    }
    if (connect(sd, (struct sockaddr *)&serv, sizeof serv) < 0) {
        close(sd);
        return -1;
    }
    return sd;
}

int create_listen_socket(int port, int backlog)
{
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) { perror("socket"); return -1; }
    int yes = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof serv);
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons((unsigned short)port);
    if (bind(sd, (struct sockaddr *)&serv, sizeof serv) < 0) {
        perror("bind");
        close(sd);
        return -1;
    }
    if (listen(sd, backlog) < 0) {
        perror("listen");
        close(sd);
        return -1;
    }
    return sd;
}
