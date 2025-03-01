#include "EventContext.h"
#include <string>
#include <iostream>

EventContext::EventContext(int fd){
    ownfd = fd;
}

EventContext::EventContext(int fd, bool isListen)
{
    ownfd = fd;
    listening = isListen;
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
