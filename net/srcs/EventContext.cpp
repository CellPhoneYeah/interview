#include "EventContext.h"
#include <string.h>
#include <iostream>
#include "EpollManager.h"
#include "slog.h"

EventContext::EventContext(int fd){
    ownfd = fd;
    readBuffer.resize(EpollManager::MAX_EPOLL_READ_SIZE);
}

EventContext::EventContext(int fd, bool isListen)
{
    ownfd = fd;
    listening = isListen;
    readBuffer.resize(EpollManager::MAX_EPOLL_READ_SIZE);
}

void EventContext::clearSendQ()
{
    std::queue<std::vector<char>>().swap(sendQ);
    offsetPos = 0;
}

void EventContext::setListening(std::string &ipaddr, int port)
{
    listening = true;
    strcpy(this->ipaddr, ipaddr.c_str());
    this->port = port;
}

bool EventContext::isListening(std::string &ipaddr, int port)
{
    if(!listening){
        return false;
    }
    return strcmp(this->ipaddr, ipaddr.c_str()) && this->port == port;
}

void EventContext::readBytes(int byte_len)
{
    // std::cout << std::string(readBuffer.data()) << std::endl;
    SPDLOG_INFO("recv msg from {}", ownfd);
    living();
    readBuffer.erase(readBuffer.begin(), readBuffer.begin() + offsetPos);
    offsetPos = 0;
}

EventContext::~EventContext(){
    // std::cout << "release EventContext" << ownfd << std::endl;
}
