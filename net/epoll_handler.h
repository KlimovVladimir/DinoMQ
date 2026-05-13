#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

int init_server_socket();
int init_epoll(int server_fd);
int epoll_handler(int server_fd, int epoll_fd);