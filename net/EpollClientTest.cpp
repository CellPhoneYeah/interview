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
            std::cout << "try send msg " << clientfd << std::endl;
            if(!emgr->sendMsg(clientfd, str.c_str(), str.size())){
                std::cout << " send msg failed " << clientfd << std::endl;
                break;
            }
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

void RunClient(){
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
            std::cout << "stop client \n";
            break;
        }
    }
    isRun = false;
}

int main(){
    std::thread th(RunClient);
    th.detach();
    std::cout << "start client ...\n";
    while(1){
        char str[1024];
        fgets(str, 1024, stdin);
        if(strcmp(str, "stop") == 0){
            isRun = false;
            break;
        }
        isRun = true;
    }
    return 0;
}