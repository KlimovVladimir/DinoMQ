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

#include "mqttHandler.h"

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

    MQTTMessage msg;

    uint8_t outbuf[OUTBUF_SIZE];
    size_t out_len;
    size_t out_sent;
};

int initServerSocket();
int initEpoll(int server_fd);
int epollHandler(int epoll_fd);

#endif //EPOLL_HANDLER_H