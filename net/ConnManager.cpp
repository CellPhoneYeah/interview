#include "EllConnBase.h"
#include <unordered_map>
#include <iostream>
#include "ConnManager.h"

std::unordered_map<int, EllConnBase*> ConnManager::clientMap;

EllConnBase *ConnManager::getClient(int sockFd)
{
    if (ConnManager::clientMap.find(sockFd) != ConnManager::clientMap.end())
    {
        return (*ConnManager::clientMap.find(sockFd)).second;
    }
    return nullptr;
}

void ConnManager::addClient(int sockFd, EllConnBase *ecb)
{
    if (ConnManager::clientMap.find(sockFd) == ConnManager::clientMap.end())
    {
        std::cout << "new sock " << sockFd << "conn " << ecb << std::endl;
        ConnManager::clientMap.insert(std::make_pair(sockFd, ecb));
    }
}

void ConnManager::delClient(int sockFd)
{
    std::unordered_map<int, EllConnBase*>::const_iterator it = ConnManager::clientMap.find(sockFd);
    if (it != ConnManager::clientMap.end())
    {
        std::cout << "del sock " << sockFd << std::endl;
        ConnManager::clientMap.erase(sockFd);
        delete((*it).second);
    }
}