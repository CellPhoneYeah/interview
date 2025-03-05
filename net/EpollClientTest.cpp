#include <iostream>
#include "EpollManager.h"
#include <string.h>
#include <thread>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <atomic>
#include "slog.h"
#include <signal.h>
#include <arpa/inet.h>

std::atomic<bool> isRun = false;
EpollManager *emgr = new EpollManager();

void sendMsg(int clientfd){
    // int i = 0;
    // std::ostringstream oss;
    std::string msg = "test msg " + clientfd;
        int loop = 10;
        while(1){
            if(loop < 0){
                break;
            }
            loop--;
            int sent = send(clientfd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
            if(sent < 0){
                SPDLOG_WARN("err stop client {}", clientfd);
                break;
            }
            SPDLOG_INFO("sended msg {}", clientfd);
            sleep(5);
        }
    if(clientfd > 20){
        // SPDLOG_INFO(" close {}", clientfd);
        // emgr->close_fd(clientfd);
    }
    // while(1){
    //     if(isRun){
    //         oss.str("");
    //         oss << "hello " << i;
    //         std::string str = oss.str();
    //         // std::cout << "try send msg " << clientfd << std::endl;
    //         // if(!emgr->sendMsg(clientfd, str.c_str(), str.size())){
    //         //     std::cout << " send msg failed " << clientfd << std::endl;
    //         //     break;
    //         // }
    //         i++;
    //         sleep(1);
    //     }
    //     else
    //     {
    //         SPDLOG_INFO("stop client sender ");
    //         break;
    //     }
    // }
}

void RunClient(){
    while(1){
        if(isRun){
            emgr->loop();
        }
        else
        {
            SPDLOG_INFO("stop client loop");
            break;
        }
    }
    isRun = false;
}

void sigintHandler(int sigint){
    if(sigint == SIGINT){
        isRun = false;
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

    isRun = true;
    std::thread th(RunClient);
    th.detach();
    for(int i = 0; i < 10; i++){
        SPDLOG_INFO("start client {}", i);
        int clientfd = emgr->connect_to("127.0.0.1", 8088);
        std::thread clientth(sendMsg, clientfd);
        clientth.detach();
    }
    while(1){
        char str[1024];
        char* ret = fgets(str, 1024, stdin);
        if(ret != nullptr && strcmp(str, "stop") == 0){
            isRun = false;
            break;
        }
        isRun = true;
    }
    return 0;
}