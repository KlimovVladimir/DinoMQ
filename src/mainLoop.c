#include "mainLoop.h"

void mainLoop() {

    //while (1) { //change to interrupt
        printf("%s Hello!\n", __func__);
        sleep(1);
        epoll_handler();
    //}
}