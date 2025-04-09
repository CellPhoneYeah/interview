#include <iostream>
#include <thread>
#include <atomic>

#include <string.h>
#include <signal.h>

#include "slog.h"

#include "ellnet/epoll_net.h"

std::atomic<bool> running;
EpollManager *emgr;

// void startServer(){
//     emgr = new EpollManager();
//     int listenRet = 0;
//     if((listenRet = emgr->start_listen("127.0.0.1", 8088)) < 0){
//         SPDLOG_INFO("listen failed {}", listenRet);
//         return;
//     }
//     while(running){
//         emgr->loop();
//     }
//     delete(emgr);
// }

void sigintHandler(int sigint){
    if(sigint == SIGINT){
        running = false;
        SPDLOG_INFO("stop signal waiting for close...");
        sleep(5);
        exit(0);
    }
}

int main(){
    struct sigaction sa;
    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        SPDLOG_WARN("Failed to set signal handler: {}", strerror(errno));
        return -1;
    }
    SPDLOG_INFO("start epoll server");
    running = true;
    // std::thread th(startServer);
    // th.detach();

    EpollNet::getInstance();
    sleep(5);
    EpollNet::getInstance()->listenOn("127.0.0.1", 8088);

    sleep(30);
    EpollNet::getInstance()->stopListen("127.0.0.1", 8088);
    
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