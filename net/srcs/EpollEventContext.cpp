#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include "EpollContextManager.h"
#include <string.h>

int EpollEventContext::handle_event(void *event)
{
    struct epoll_event *ev = (struct epoll_event*)event;
    if(ev->events & EPOLLIN){
        return handle_read_event(ev);
    }
    if(ev->events & EPOLLOUT){
        return handle_write_event(ev);
    }
    return -1;
}

int EpollEventContext::handle_read_event(epoll_event *event)
{
    if(isListening()){
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        while(1){
            int newFd = accept(ownfd, (sockaddr*)&addr, &len);
            if(newFd < 0){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    // 边缘模式下，暂时没有事件,等待下一次事件到来
                    return HANDLE_EVENT_CODE_OK;
                }
                return HANDLE_EVENT_CODE_ACCEPT_FAILED;
            }
            set_noblocking(newFd);
            EpollEventContext* newContext = new EpollEventContext(newFd);
            EpollContextManager::addContext(newContext);
            int flag = fcntl(newFd, F_GETFL, 0);
            flag |= EPOLLIN | EPOLLET; // 边缘触发
            fcntl(newFd, F_SETFL, flag);
            event->events = flag;
            event->data.fd = newFd;
            event->data.ptr = newContext;
            epoll_ctl(ownfd, EPOLL_CTL_ADD, newFd, event);
        }
        return HANDLE_EVENT_CODE_OK;
    }

    readBuffer.resize(1024);
    int readbytes = recv(ownfd, readBuffer.data(), readBuffer.size(), MSG_NOSIGNAL);
    if(readbytes > 0){
        process_data(readBuffer.data(), readbytes);
    }
    if(readbytes == 0){
        // 已经断开连接
        return HANDLE_EVENT_CODE_REMOTE_CONNECT_CLOSED;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        // 边缘触发时，属于正常读取结束
        return HANDLE_EVENT_CODE_OK;
    }
    if (errno == ECONNRESET)
    {
        return HANDLE_EVENT_CODE_CONNECT_RESET;
    }
    std::cout << "recv failed for unknow err" << errno << std::endl;
    return HANDLE_EVENT_CODE_UNKNOW_ERR;
}

int EpollEventContext::handle_write_event(epoll_event *event)
{
    if(sendQ.size() == 0){
        return HANDLE_EVENT_CODE_SEND_QUEUE_EMPTY;
    }
    return 0;
}

void EpollEventContext::set_noblocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

void EpollEventContext::process_data(char *data, int size)
{
    std::cout << "process data" << std::string(data) << std::endl;
}

void EpollEventContext::modify_ev(int fd, int flag, bool enable)
{
    int current_flag = fcntl(fd, F_GETFL, 0);
}

EpollEventContext::~EpollEventContext(){
    std::cout << "release EpollEventContext :" << ownfd << std::endl;
}