#ifndef WATCHDOG_EPOLL_CONNECT_HANDLER_H
#define WATCHDOG_EPOLL_CONNECT_HANDLER_H

#include "ellnet/epoll_connect_handler.h"
#include "watchdog/watchdog.h"
#include "spdlog/slog.h"

namespace ellnet
{
    class WatchDogConnectHandler : public EpollConnectHandler
    {
        void handle(const int fd) override
        {
            WatchDog::GetDog(1)->PutConn(fd);
            SPDLOG_INFO("new connectino coming to watch dog");
        }
    };
}

#endif