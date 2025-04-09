#ifndef EPOLL_CONNECT_HANDLER_H
#define EPOLL_CONNECT_HANDLER_H
namespace ellnet
{
    class EpollConnectHandler
    {
        virtual void handle(const int fd) = 0;
    };
}

#endif