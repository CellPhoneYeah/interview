#include "EpollManager.h"
#include <iostream>
#include <thread>
#include <string.h>

bool running;

void startServer(){
    EpollManager *emgr = new EpollManager();
    int listenRet = 0;
    if((listenRet = emgr->start_listen("127.0.0.1", 8088)) < 0){
        std::cout << "listen failed " << listenRet << std::endl;
        return;
    }
    while(1){
        if(running){
            emgr->loop();
        }
        else{
            std::cout << "stop server" << std::endl;
        }
    }
    delete(emgr);
}

int main(){
    std::cout << "start epoll server" << std::endl;
    running = true;
    std::thread th(startServer);
    th.detach();
    char str[1024];
    while(1){
        fgets(str, 1024, stdin);
        if(strcmp(str, "stop") == 0){
            running = false;
            break;
        }
    }
    return 0;
}