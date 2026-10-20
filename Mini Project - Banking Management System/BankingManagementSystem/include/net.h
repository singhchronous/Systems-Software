/*
 * net.h - Socket helpers (slides 104-114).
 */
#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <sys/types.h>
#include "protocol.h"

/* write()/read() exactly n bytes, looping over partial transfers.
 * readn returns n on success, 0 if the peer closed the connection, -1 on error
 * (errno == EINTR is returned to the caller so a signal can end a wait). */
ssize_t writen(int fd, const void *buf, size_t n);
ssize_t readn(int fd, void *buf, size_t n);

int send_request(int fd, const Request *req);
int recv_request(int fd, Request *req);        /* 1 ok, 0 closed, -1 error/EINTR */
int send_response(int fd, const Response *resp);
int recv_response(int fd, Response *resp);     /* 1 ok, 0 closed, -1 error */

/* Client side: socket() + connect(). Returns fd or -1. */
int connect_to_server(const char *ip, int port);

/* Server side: socket() + setsockopt(SO_REUSEADDR) + bind() + listen(). */
int create_listen_socket(int port, int backlog);

#endif
