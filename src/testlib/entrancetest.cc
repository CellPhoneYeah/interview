#include <thread>
#include <string>

#include "spdlog/slog.h"
#include "ellnet/epoll_net.h"

std::string host = "127.0.0.1";
int port = 8088;

int main(){
    ellnet::EpollNet* instance = ellnet::EpollNet::GetInstance();
    SPDLOG_INFO("connect to {}:{}", host, port);
    for(int i = 0; i < 10; i++){
        if(instance->ConnectTo(host, port) != 0){
            SPDLOG_INFO("connect to {}:{} failed", host, port);
            return -1;
        }
    }
    
    instance->SendMsg("test msg ", 1);
    return 0;
}