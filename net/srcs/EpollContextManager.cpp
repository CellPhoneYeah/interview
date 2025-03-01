#include "EpollContextManager.h"

std::unordered_map<int, EpollEventContext*> EpollContextManager::contexts;

void EpollContextManager::addContext(EpollEventContext * eec)
{
    contexts[eec->getFd()] = eec;
}

void EpollContextManager::delContext(int fd)
{
    if(contexts[fd] != nullptr){
        delete(contexts[fd]);
        contexts[fd] = nullptr;
    }
}
EpollEventContext* EpollContextManager::getContext(int fd)
{
    return contexts[fd];
}