#include "EpollContextManager.h"
#include "slog.h"

// std::unordered_map<int, EpollEventContext*> EpollContextManager::contexts;

// void EpollContextManager::addContext(EpollEventContext * eec)
// {
//     contexts[eec->getFd()] = eec;
//     SPDLOG_INFO("add fd {} left ctx {}", eec->getFd(), contexts.size());
// }

// void EpollContextManager::delContext(int fd)
// {
//     if(contexts[fd] != nullptr){
//         delete(contexts[fd]);
//         contexts[fd] = nullptr;
//     }
//     SPDLOG_INFO("del fd {} left ctx {}", fd, contexts.size());
// }
// EpollEventContext* EpollContextManager::getContext(int fd)
// {
//     return contexts[fd];
// }