#include <iostream>
#include "EpollManager.h"
#include <string.h>
#include <thread>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <atomic>

std::atomic<bool> isRun = false;
EpollManager *emgr = new EpollManager();

void sendMsg(int clientfd){
    sleep(5);
    int i = 0;
    std::ostringstream oss;
    std::cout << isRun << " run send msg\n";
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
            std::cout << "stop client sender \n";
            break;
        }
    }
}

void RunClient(int id){
    std::cout << "start client " << id << std::endl;
    int clientfd = emgr->connect_to("127.0.0.1", 8088);
    isRun = true;
    std::thread clientth(sendMsg, clientfd);
    clientth.detach();
    while(1){
        if(isRun){
            emgr->loop();
        }
        else
        {
            std::cout << "stop client " << id << std::endl;
            break;
        }
    }
    isRun = false;
}

int main(){
    for(int i = 0; i < 10; i++){
        std::thread th(RunClient, i);
        th.detach();
        sleep(2);
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