#ifndef EPOLL_HANDLER_H
#define EPOLL_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define INBUF_SIZE      8192
#define OUTBUF_SIZE     8192

enum { EV_SERVER, EV_CLIENT };

struct Connection {
    int type;
    int fd;
};

struct MQTTClient {
    int type;
    int fd;

    uint8_t inbuf[INBUF_SIZE];
    size_t in_len;

    uint8_t outbuf[OUTBUF_SIZE];
    size_t out_len;
    size_t out_sent;
};



int init_server_socket();
int init_epoll(int server_fd);
int epoll_handler(int epoll_fd);

#endif //EPOLL_HANDLER_H