#include <sys/epoll.h>
#include "EpollManager.h"
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "EpollEventContext.h"
#include <unistd.h>
#include <fcntl.h>

void EpollManager::addContext(EpollEventContext * eec)
{
    contexts[eec->getFd()] = eec;
}

void EpollManager::delContext(int fd)
{
    if(contexts[fd] != nullptr){
        delete(contexts[fd]);
    }
    contexts[fd] = nullptr;
}
EpollEventContext* EpollManager::getContext(int fd)
{
    return contexts[fd];
}

int EpollManager::init()
{
    epoll_fd = epoll_create1(0);
    return epoll_fd;
}

int EpollManager::start_listen(std::string ip_str, int port)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_str.c_str());
    addr.sin_port = htons(port);
    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(listenfd < 0){
        std::cout << "sock create failed" << errno << std::endl;
        return -1;
    }
    if(bind(listenfd, (sockaddr*)&addr, len) < 0){
        std::cout << "sock bind failed:" << errno << std::endl;
        return -2;
    }

    struct epoll_event ev;
    ev.data.fd = listenfd;
    EpollEventContext* newCtx = new EpollEventContext(listenfd, true);
    this->addContext(newCtx);
    int flag = EPOLLIN | EPOLLET;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, flag, &ev) < 0){
        this->delContext(epoll_fd);
        delete(newCtx);
        close(epoll_fd);
        return -3;
    }
    return 0;
}

int EpollManager::connect_to(std::string addr, int port)
{
    return 0;
}

int EpollManager::loop()
{
    while(1){
        int eventn = epoll_wait(epoll_fd, event_list, MAX_EPOLL_EVENT_NUM, 0); // 立刻返回结果，不阻塞
        if(eventn < 0){
            if(errno == EINTR){
                // 信号中断
                std::cout << "epoll break by signal " << std::endl;
                continue;
            }
            std::cout << "epoll wait err:" << errno << std::endl;
            break;
        }
        for (int i = 0; i < eventn; i++)
        {
            struct epoll_event ev = event_list[i];
            EpollEventContext* ctx = (EpollEventContext*)ev.data.ptr;
            if(ev.events & EPOLLIN){
                if(ctx->isListening()){
                    do_accept(ev, ctx);
                }else{
                    do_read(ev, ctx);
                }
            }else if(ev.events & EPOLLOUT){
                do_send(ev, ctx);
            }else if(ev.events & EPOLLERR){
                std::cout << "epoll err " << ev.data.fd << std::endl;
                close_fd(ev.data.fd);
            }else if(ev.events & EPOLLHUP){
                std::cout << "remote closed sock " << ev.data.fd << std::endl;
                close_fd(ev.data.fd);
            }
        }
        
    }
    return 0;
}

void EpollManager::do_accept(epoll_event &ev, EpollEventContext *ctx)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int flag = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
    ctx->set_noblocking(ev.data.fd);
    while(1){
        int newFd = accept(ev.data.fd, (sockaddr*)&addr, &len);
        if(newFd < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // 所有连接处理完
                return;
            }
            if(errno == ECONNABORTED){
                // 事件处理前，对方断开
                std::cout << "remote disconnected" << std::endl;
                continue;;
            }
            if(errno == EMFILE){
                std::cout << "file fd limit" << std::endl;
                close_fd(ev.data.fd);
                return;
            } 
            if(errno == ENFILE){
                std::cout << "out of fd " << std::endl;
                close_fd(ev.data.fd);
                return;
            }
            if(errno == EINTR){
                std::cout << "signal break " << std::endl;
                continue;
            }
            std::cout << "unexpect err:" << errno << std::endl;
            close_fd(ev.data.fd);
            return;
        }else{
            struct epoll_event ev;
            ev.data.fd = newFd;
            EpollEventContext* newCtx = new EpollEventContext(newFd, false);
            ev.data.ptr = newCtx;
            ev.events = flag;
            newCtx->set_noblocking(newFd);
            if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newFd, &ev) != 0){
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, newFd, &ev);
            }
        }
    }
}

void EpollManager::do_read(epoll_event &ev, EpollEventContext *ctx)
{
    int current_fd = ev.data.fd;
    while(1){
        int read_bytes = recv(current_fd, ctx->readBuffer.data() + ctx->offsetPos, MAX_EPOLL_READ_SIZE, 0);
        if(read_bytes == 0){
            // 断开连接
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
            EpollManager::delContext(current_fd);
            delete(ctx);
            return;
        }
        if(read_bytes < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // 暂时没数据
                return;
            }
            if(errno == ECONNRESET){
                //对端关闭连接
                std::cout << "remote close connect when recving" << current_fd << std::endl;
                close_fd(current_fd);
                return;
            }
            if(errno == EINTR){
                // 信号中断
                std::cout << "signal break when recving " << current_fd << std::endl;
                continue;;
            }
            if(errno == EBADF){
                std::cout << "bad fd " << current_fd << std::endl;
                close_fd(current_fd);
                return;
            }
            if(errno == ENOTCONN){
                // 已经关闭的连接未从epoll移除可能出现
                std::cout << "old closed epoll fd not del " << current_fd << std::endl;
                close_fd(current_fd);
                return;
            }
            std::cout << "unknow err for epoll fd recv " << current_fd << " err: " << errno << std::endl;
            close_fd(current_fd);
            return;
        }
        ctx->readBytes(read_bytes);
    }
}

void EpollManager::do_send(epoll_event &ev, EpollEventContext *ctx)
{
    int current_fd = ev.data.fd;
    ctx->set_noblocking(current_fd);
    while(1){
        if(ctx->sendQ.size() == 0){
            std::cout << "send queue empty!" << current_fd << std::endl;
            ev.events &= ~EPOLLOUT;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &ev);
            ctx->clearSendQ();
            return;
        }
        auto msg = ctx->sendQ.front();
        int sent = send(current_fd, msg.data() + ctx->offsetPos, msg.size() - ctx->offsetPos, MSG_NOSIGNAL);
        if(sent < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // 发送缓冲满了, 再次注册等待下次发送
                ev.events &= EPOLLOUT | EPOLLET;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &ev);
                return;
            }
            if(errno == EPIPE){
                // 对方关闭连接
                std::cout << " remote closed when send msg " << current_fd << std::endl;
                close_fd(current_fd);
                return;
            }
            if(errno == ENOTSOCK){
                std::cout << " send target not a fd " << current_fd << std::endl;
                close_fd(current_fd);
                return;
            }
            if(errno == EINVAL){
                std::cout << " some args is wrong when send to fd " << current_fd << std::endl;
                close_fd(current_fd);
                return;
            }
            if(errno == ECONNRESET){
                std::cout << " remote reset when send to fd " << current_fd << std::endl;
                close_fd(current_fd);
                return;
            }
            std::cout << " unexpected err when send to fd " << current_fd << " err " << errno << std::endl;
            return;
        }
        if(sent < msg.size()){
            ctx->offsetPos += sent;
            continue;
        }
        ctx->sendQ.pop();
        ctx->offsetPos = 0;
    }
}

void EpollManager::close_fd(int fd)
{
    EpollManager::delContext(fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}
