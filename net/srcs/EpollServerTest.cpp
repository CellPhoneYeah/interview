#include "EpollManager.h"
#include <iostream>
#include <thread>
#include <string.h>

bool running;

void startServer(){
    EpollManager *emgr = new EpollManager();
    emgr->init();
    emgr->start_listen("127.0.0.1", 8088);
    while(1){
        if(running){
            emgr->loop();
        }
        else{
            std::cout << "stop server" << std::endl;
        }
    }
}

int main(){
    std::cout << "start epoll server" << std::endl;
    running = true;
    std::thread th(startServer);
    th.detach();
    char str[1024];
    while(1){
        fgets(str, 1024, stdin);
        if(strcmp(str, "stop")){
            running = false;
            break;
        }
    }
    return 0;
}