#ifndef WATCH_DOG_EPOLL_CONNECT_HANDLER_H
#define WATCH_DOG_EPOLL_CONNECT_HANDLER_H

#include "ellnet/EpollConnectHandler.h"
#include "watchdog/watchdog.h"
#include "spdlog/slog.h"

class WatchDogConnectHandler: public EpollConnectHandler{
    void handle(int fd) override {
        watchdog::getDog(1)->putConn(fd);
        SPDLOG_INFO("new connectino coming to watch dog");
    }
};
#endif