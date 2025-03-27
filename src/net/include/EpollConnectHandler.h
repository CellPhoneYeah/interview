#ifndef EPOLL_CONNECT_HANDLER_H
#define EPOLL_CONNECT_HANDLER_H
class EpollConnectHandler{
    virtual void handle(int fd) = 0;
};
#endif