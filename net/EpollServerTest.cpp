#include "EpollManager.h"
#include <iostream>
#include <thread>
#include <string.h>
#include <signal.h>
#include <atomic>

std::atomic<bool> running;
EpollManager *emgr = new EpollManager();

void startServer(){
    int listenRet = 0;
    if((listenRet = emgr->start_listen("127.0.0.1", 8088)) < 0){
        std::cout << "listen failed " << listenRet << std::endl;
        return;
    }
    while(running){
        // std::cout << "loop stop signal " << running;
        emgr->loop();
    }
    delete(emgr);
}

void sigintHandler(int sigint){
    if(sigint == SIGINT){
        running = false;
        std::cout << "stop signal " << running;
        exit(0);
    }
}

int main(){
    struct sigaction sa;
    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        std::cerr << "Failed to set signal handler: " << strerror(errno) << std::endl;
        return -1;
    }
    std::cout << "start epoll server" << std::endl;
    running = true;
    std::thread th(startServer);
    th.detach();
    char str[1024];
    while(1){
        char* ret = fgets(str, 1024, stdin);
        if(ret != nullptr && strcmp(str, "stop") == 0){
            running = false;
            break;
        }
    }
    running = false;
    return 0;
}