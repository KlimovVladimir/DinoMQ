#include "epollHandler.h"

#define PORT 3344
#define MAX_EVENTS 64
#define BUF_SIZE 1024

static int g_epoll_fd;

static int setSocketNonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int initServerSocket() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        exit(1);
    }

    setSocketNonblocking(server_fd);

    return server_fd;
}

void enableClientWrite(struct MQTTClient *client) {
    struct epoll_event ev;

    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.ptr = client;

    epoll_ctl(g_epoll_fd, EPOLL_CTL_MOD, client->fd, &ev);
}

void disableClientWrite(struct MQTTClient *client) {
    struct epoll_event ev;

    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = client;

    epoll_ctl(g_epoll_fd, EPOLL_CTL_MOD, client->fd, &ev);
}

int queuePacket(struct MQTTClient *client, const uint8_t *data, size_t len) {
    if (client->out_len + len > sizeof(client->outbuf)) {
        return -1;
    }

    memcpy(client->outbuf + client->out_len, data, len);
    client->out_len += len;

    return 0;
}

void closeClient(struct MQTTClient *client) {
    if (!client)
        return;

    epoll_ctl(g_epoll_fd, EPOLL_CTL_DEL, client->fd, NULL);
    close(client->fd);
    free(client);
}

static void handleClientRead(struct MQTTClient *client) {
    while (1) {
        if (client->in_len >= sizeof(client->inbuf)) {
            logPrint(LOG_ERR, "[%s] Overflow! Client disconnected: fd=%d\n", __func__, client->fd);
            closeClient(client);
            return;
        }

        ssize_t count = read(client->fd, client->inbuf + client->in_len, sizeof(client->inbuf) - client->in_len);
        
        if (count == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }

            perror("read");
            closeClient(client);
            return;
        }

        if (count == 0) {
            logPrint(LOG_INFO, "[%s] Client disconnected: fd=%d\n", __func__, client->fd);
            closeClient(client);
            return;
        }

        client->in_len += count;

        int status = parseMQTTPacket(client);
        while (status == PACKET_PARSED) {
            int session_status = handleMQTTMessage(client);

            if (session_status == MQTT_CLOSE) {
                closeClient(client);
                return;
            }

            memmove(client->inbuf, client->inbuf + client->msg.totalLength, client->in_len - client->msg.totalLength);
            client->in_len -= client->msg.totalLength;
            memset(&client->msg, 0, sizeof(MQTTMessage));
            status = parseMQTTPacket(client);
        }

        // count = write(client->fd, client->inbuf, client->in_len);

        // if (count > 0) {
        //     memmove(client->inbuf, client->inbuf + count, client->in_len - count);
        //     client->in_len -= count;
        // }

    }
}

int initEpoll(int server_fd) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(1);
    }

    g_epoll_fd = epoll_fd;

    struct Connection *server = malloc(sizeof(struct Connection));
    server->type = EV_SERVER;
    server->fd = server_fd;

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = server;

    if (epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, server->fd, &event) == -1) {
        perror("epoll_ctl server_fd");
        exit(1);
    }

    logPrint(LOG_INFO, "[%s] Server listening on port %d\n", __func__, PORT);

    return g_epoll_fd;
}

int epollHandler() {
    struct epoll_event events[MAX_EVENTS];

    int n = epoll_wait(g_epoll_fd, events, MAX_EVENTS, 100);
    if (n == -1) {
        perror("epoll_wait");
        return -1;
    }
    else if (n == 0)
        return 0;

    for (int i = 0; i < n; i++) {
        //int fd = events[i].data.fd;
        struct Connection *conn = events[i].data.ptr;

        if (events[i].events & EPOLLIN) {
            if (conn->type == EV_SERVER) {
                while (1) {
                    int client_fd = accept(conn->fd, NULL, NULL);

                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }

                        perror("accept");
                        break;
                    }

                    setSocketNonblocking(client_fd);

                    struct MQTTClient *client = calloc(1, sizeof(struct MQTTClient));

                    client->fd = client_fd;
                    client->type = EV_CLIENT;

                    struct epoll_event client_event;
                    client_event.events = EPOLLIN | EPOLLET;
                    client_event.data.ptr = client;

                    if (epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) {
                        perror("epoll_ctl client_fd");
                        close(client_fd);
                        free(client);
                    }

                    logPrint(LOG_INFO, "[%s] Client connected: fd=%d\n", __func__, client_fd);
                }
            } else {
                struct MQTTClient *client = (struct MQTTClient *)conn;
                handleClientRead(client);
            }
        }
        else if (events[i].events & EPOLLOUT) {
            struct MQTTClient *client = (struct MQTTClient *)conn;

            while (client->out_sent < client->out_len) {
                ssize_t count = write(client->fd, client->outbuf + client->out_sent, client->out_len - client->out_sent);

                if (count == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break;

                    perror("write");
                    closeClient(client);
                    break;
                }

                client->out_sent += count;
            }

            if (client->out_sent >= client->out_len) {
                client->out_len = 0;
                client->out_sent = 0;
                disableClientWrite(client);
            }
        }
    }
    
    return 0;
}