#include "EllConn.h"

std::unordered_map<int, EllConn*> ConnManager::clientMap;

EllConn *ConnManager::getClient(int sockFd)
{
    if (ConnManager::clientMap.find(sockFd) != ConnManager::clientMap.end())
    {
        return (*ConnManager::clientMap.find(sockFd)).second;
    }
    return nullptr;
}

void ConnManager::addClient(int sockFd, EllConn *ec)
{
    if (ConnManager::clientMap.find(sockFd) == ConnManager::clientMap.end())
    {
        std::cout << "new sock " << sockFd << "conn " << *ec << std::endl;
        ConnManager::clientMap.insert(std::make_pair(sockFd, ec));
    }
}
void ConnManager::delClient(int sockFd)
{
    std::unordered_map<int, ConnManager *>::const_iterator it = ConnManager::clientMap.find(sockFd);
    if (it != ConnManager::clientMap.end())
    {
        std::cout << "del sock " << sockFd << std::endl;
        ConnManager::clientMap.erase(sockFd);
        delete((*it).second);
    }
}