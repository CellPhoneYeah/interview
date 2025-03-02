#include "EventContext.h"
#include <string>
#include <iostream>
#include "EpollManager.h"

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

void EventContext::readBytes(int byte_len)
{
    std::cout << std::string(readBuffer.data()) << std::endl;
    readBuffer.erase(readBuffer.begin(), readBuffer.begin() + offsetPos);
    offsetPos = 0;
}

EventContext::~EventContext(){
    std::cout << "release EventContext" << ownfd << std::endl;
}
