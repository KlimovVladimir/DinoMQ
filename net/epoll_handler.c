#include "epoll_handler.h"

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

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl server_fd");
        exit(1);
    }

    printf("[%s] Server listening on port %d\n", __func__, PORT);

    return epoll_fd;
}

int epoll_handler() {
    int server_fd = init_server_socket();
    int epoll_fd = init_epoll(server_fd);

    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                while (1) {
                    int client_fd = accept(server_fd, NULL, NULL);

                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }

                        perror("accept");
                        break;
                    }

                    set_nonblocking(client_fd);

                    struct epoll_event client_event;
                    client_event.events = EPOLLIN | EPOLLET;
                    client_event.data.fd = client_fd;

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) {
                        perror("epoll_ctl client_fd");
                        close(client_fd);
                    }

                    printf("Client connected: fd=%d\n", client_fd);
                }
            } else {
                char buf[BUF_SIZE];

                while (1) {
                    ssize_t count = read(fd, buf, sizeof(buf));

                    if (count == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }

                        perror("read");
                        close(fd);
                        break;
                    }

                    if (count == 0) {
                        printf("[%s] Client disconnected: fd=%d\n", __func__, fd);
                        close(fd);
                        break;
                    }

                    write(fd, buf, count);
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    return 0;
}