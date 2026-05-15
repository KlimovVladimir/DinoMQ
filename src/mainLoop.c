#include "mainLoop.h"

void mainLoop() {

    int server_fd = initServerSocket();
    int epoll_fd = initEpoll(server_fd);

    printf("%s Hello!\n", __func__);

    while (1) { //change to interrupt
        sleep(1);
        epollHandler();
    }

    close(server_fd);
    close(epoll_fd);
}