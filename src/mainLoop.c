#include "mainLoop.h"

void mainLoop() {

    int server_fd = initServerSocket();
    int epoll_fd = initEpoll(server_fd);

    while (1) { //change to interrupt
        epollHandler();
    }

    close(server_fd);
    close(epoll_fd);
}