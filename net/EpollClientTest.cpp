#include <iostream>
#include "EpollManager.h"
#include <string.h>
#include <thread>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <atomic>
#include <spdlog/spdlog.h>

std::atomic<bool> isRun = false;
EpollManager *emgr = new EpollManager();

void sendMsg(int clientfd){
    sleep(5);
    int i = 0;
    std::ostringstream oss;
    spdlog::info(" run send msg {}", i);
    while(1){
        if(isRun){
            oss.str("");
            oss << "hello " << i;
            std::string str = oss.str();
            // std::cout << "try send msg " << clientfd << std::endl;
            // if(!emgr->sendMsg(clientfd, str.c_str(), str.size())){
            //     std::cout << " send msg failed " << clientfd << std::endl;
            //     break;
            // }
            i++;
            sleep(1);
        }
        else
        {
            spdlog::info("stop client sender ");
            break;
        }
    }
}

void RunClient(){
    while(1){
        if(isRun){
            emgr->loop();
        }
        else
        {
            spdlog::info("stop client loop");
            break;
        }
    }
    isRun = false;
}

int main(){
    isRun = true;
    std::thread th(RunClient);
    th.detach();
    for(int i = 0; i < 100; i++){
        spdlog::info("start client {}", i);
        emgr->connect_to("127.0.0.1", 8088);
        // std::thread clientth(sendMsg, clientfd);
        // clientth.detach();
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