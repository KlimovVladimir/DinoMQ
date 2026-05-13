#include "epollHandler.h"

#define PORT 3344
#define MAX_EVENTS 64
#define BUF_SIZE 1024

static int set_socket_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int init_server_socket() {
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

    set_socket_nonblocking(server_fd);

    return server_fd;
}

int init_epoll(int server_fd) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(1);
    }

    struct Connection *server = malloc(sizeof(struct Connection));
    server->type = EV_SERVER;
    server->fd = server_fd;

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = server;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server->fd, &event) == -1) {
        perror("epoll_ctl server_fd");
        exit(1);
    }

    printf("[%s] Server listening on port %d\n", __func__, PORT);

    return epoll_fd;
}

int epoll_handler(int epoll_fd) {
    struct epoll_event events[MAX_EVENTS];

    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 100);
    if (n == -1) {
        perror("epoll_wait");
        return -1;
    }
    else if (n == 0)
        return 0;

    for (int i = 0; i < n; i++) {
        //int fd = events[i].data.fd;
        struct Connection *conn = events[i].data.ptr;

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

                set_nonblocking(client_fd);

                struct MQTTClient *client = malloc(sizeof(struct MQTTClient));

                client->fd = client_fd;
                client->type = EV_CLIENT;
                client->in_len = 0;
                client->out_len = 0;
                client->out_sent = 0;

                struct epoll_event client_event;
                client_event.events = EPOLLIN | EPOLLET;
                client_event.data.ptr = client;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) {
                    perror("epoll_ctl client_fd");
                    close(client_fd);
                    free(client);
                }

                printf("[%s] Client connected: fd=%d\n", __func__, client_fd);
            }
        } else {
            while (1) {
                struct MQTTClient *client = (struct MQTTClient *)conn;

                if (client->in_len >= sizeof(client->inbuf)) {
                    printf("[%s] Overflow! Client disconnected: fd=%d\n", __func__, client->fd);
                    close(client->fd);
                    free(client);
                    break;
                }

                ssize_t count = read(client->fd, client->inbuf + client->in_len, sizeof(client->inbuf) - client->in_len);
                
                if (count == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }

                    perror("read");
                    close(client->fd);
                    free(client);
                    break;
                }

                if (count == 0) {
                    printf("[%s] Client disconnected: fd=%d\n", __func__, client->fd);
                    close(client->fd);
                    free(client);
                    break;
                }

                client->in_len += count;

                // count = write(client->fd, client->inbuf, client->in_len);

                // if (count > 0) {
                //     memmove(client->inbuf, client->inbuf + count, client->in_len - count);
                //     client->in_len -= count;
                // }

            }
        }
    }
    
    return 0;
}