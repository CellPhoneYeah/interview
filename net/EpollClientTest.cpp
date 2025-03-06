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
#include <sstream>

std::atomic<bool> isRun = false;
EpollManager *emgr;

void sendMsg(int pipe_fd_in){
    char buff[64] = "conn";
    write(pipe_fd_in, buff, 64);
    SPDLOG_INFO("sendMsg:{}", buff);
    while(1){
        if(isRun){
            sleep(5);
        }else{
            break;
        }
    }
    // int i = 0;
    // std::ostringstream oss;
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

void RunClient(int pipe_fd_out){
    if(!emgr->newPipe(pipe_fd_out)){
        return;
    }
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
    emgr = new EpollManager();
    struct sigaction sa;
    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        SPDLOG_WARN("Failed to set signal handler: {}", strerror(errno));
        return -1;
    }

    int pipe_fd[2];
    pipe(pipe_fd);
    isRun = true;
    
    for(int i = 0; i < 300; i++){
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        SPDLOG_INFO("start client {} {}", i, oss.str());
        emgr->connect_to("127.0.0.1", 8088);
        // std::thread clientth(sendMsg, pipe_fd[1]);
        // clientth.detach();
    }
    sleep(1);
    RunClient(pipe_fd[0]);
    // while(1){
    //     char str[1024];
    //     char* ret = fgets(str, 1024, stdin);
    //     if(ret != nullptr && strcmp(str, "stop") == 0){
    //         isRun = false;
    //         break;
    //     }
    //     isRun = true;
    // }
    return 0;
}