#include "mainLoop.h"

void mainLoop() {

    int server_fd = init_server_socket();
    int epoll_fd = init_epoll(server_fd);

    printf("%s Hello!\n", __func__);

    while (1) { //change to interrupt
        sleep(1);
        epoll_handler(epoll_fd);
    }

    close(server_fd);
    close(epoll_fd);
}