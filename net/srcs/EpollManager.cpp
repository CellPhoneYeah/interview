#include <sys/epoll.h>
#include "EpollManager.h"
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "EpollEventContext.h"
#include <unistd.h>
#include <ctime>
#include <spdlog/spdlog.h>

std::unordered_map<int, EpollEventContext*> EpollManager::contexts;

void EpollManager::addContext(EpollEventContext * eec)
{
    contexts[eec->getFd()] = eec;
}

void EpollManager::delContext(int fd)
{
    EpollEventContext* pCtx = contexts[fd];
    if(pCtx != nullptr){
        delete(pCtx);
    }
    spdlog::info("del context {}", fd);
    contexts.erase(fd);
}
EpollEventContext* EpollManager::getContext(int fd)
{
    return contexts[fd];
}

EpollManager::EpollManager()
{
    init();
}

EpollManager::~EpollManager()
{
    if(contexts.size() == 0){
        return;
    }
    auto it = contexts.begin();
    int cur_fd;
    while(it != contexts.end()){
        cur_fd = it->first;
        EpollEventContext* pCtx = contexts[cur_fd];
        if(pCtx != nullptr){
            delete(pCtx);
        }
        spdlog::info("del context {}", cur_fd);
        it = contexts.find(cur_fd);
        if(it != contexts.end()){
            it = contexts.erase(it);
        }
    }
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
        spdlog::info("sock create failed", errno);
        return -1;
    }
    if(bind(listenfd, (sockaddr*)&addr, len) < 0){
        spdlog::info("sock bind failed:", errno);
        return -2;
    }

    if(addr.sin_addr.s_addr == INADDR_NONE){
        spdlog::info("invalid ip addr ", ip_str);
        close(listenfd);
        return -4;
    }

    if(listen(listenfd, 0) < 0){
        spdlog::info("listen failed ", strerror(errno));
        return -5;
    }

    struct epoll_event ev;
    EpollEventContext* newCtx = new EpollEventContext(listenfd, true);
    this->addContext(newCtx);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = newCtx;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &ev) < 0){
        delContext(listenfd);
        spdlog::info("stop listen fd");
        return -3;
    }
    spdlog::info("{} listen to fd ", listenfd);
    last_tick = std::time(nullptr);
    return 0;
}

int EpollManager::connect_to(std::string ip_str, int port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_str.c_str(), &addr.sin_addr);
    addr.sin_port = htons(port);
    int client_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    socklen_t addrlen = sizeof(addr);
    int ret = connect(client_fd, (const sockaddr*)&addr, addrlen);
    if(ret == -1 && errno != EINPROGRESS){
        spdlog::info("{} connect to {}:{} ret:{} errno {} failed!", client_fd, ip_str, port, ret, errno);
        return -1;
    }
    if(ret == 0){
        spdlog::info("connect finish ", client_fd);
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET; // 已经完成连接，直接开始接收数据
        EpollEventContext * ctx = new EpollEventContext(client_fd, false);
        ctx->connectFinish();
        ev.data.ptr = ctx;
        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0){
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
        }
        addContext(ctx);
    }else{
        struct epoll_event ev;
        ev.events = EPOLLOUT | EPOLLET;
        EpollEventContext * ctx = new EpollEventContext(client_fd, false);
        ctx->connectStart();
        ev.data.ptr = ctx;
        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0){
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
        }
        addContext(ctx);
        spdlog::info("waiting connect ", client_fd);
    }
    
    return client_fd;
}

int EpollManager::loop()
{
    int loopcount = 0;
    while(1){
        loopcount ++;
        time_t current_time = std::time(nullptr);
        if(std::difftime(current_time, last_tick) > 10){
            last_tick = current_time;
            spdlog::info(" connection nums :{} loopcount:{}", contexts.size(), loopcount);
        }
        int eventn = epoll_wait(epoll_fd, event_list, MAX_EPOLL_EVENT_NUM, 0); // 立刻返回结果，不阻塞
        if(eventn < 0){
            if(errno == EINTR){
                // 信号中断
                spdlog::info("epoll break by signal ");
                continue;
            }
            spdlog::info("epoll wait err:{}", errno);
            break;
        }
        if(eventn == 0){
            // sleep(1);
            return 0;
        }
        for (int i = 0; i < eventn; i++)
        {
            struct epoll_event ev = event_list[i];
            EpollEventContext* ctx = (EpollEventContext*)ev.data.ptr;
            if(ev.events & EPOLLIN){
                if(ctx->isListening()){
                    do_accept(ev, ctx);
                }else{
                    do_read(ctx);
                }
            }else if(ev.events & EPOLLOUT){
                if(ctx->isConnecting()){
                    do_conn(ev, ctx);
                }
                else
                {
                    do_send(ev, ctx);
                }
            }else if(ev.events & EPOLLERR){
                spdlog::warn("epoll err {}", ctx->getFd());
                close_fd(ctx->getFd());
            }else if(ev.events & EPOLLHUP){
                spdlog::warn("{} epoll wait found remote closed sock {}", i, ctx->getFd());
                close_fd(ctx->getFd());
            }else{
                spdlog::warn("other event ");
                close_fd(ctx->getFd());
            }
        }
    }
    return 0;
}

void EpollManager::do_accept(epoll_event &ev, EpollEventContext *ctx)
{
    int accept_num = 0;
    int failed_accept_num = 0;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int flag = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
    int currentFd = ctx->getFd();
    // ctx->set_noblocking(currentFd);
    while(1){
        int newFd = accept(currentFd, (sockaddr*)&addr, &len);
        if(newFd < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                spdlog::info("done all accept success {} failed {}", accept_num, failed_accept_num);
                // 所有连接处理完
                return;
            }
            if(errno == ECONNABORTED){
                // 事件处理前，对方断开
                failed_accept_num++;
                spdlog::info("remote disconnected");
                continue;;
            }
            if(errno == EMFILE){
                failed_accept_num++;
                spdlog::info("file fd limit success {} failed {}", accept_num, failed_accept_num);
                close_fd(currentFd);
                return;
            } 
            if(errno == ENFILE){
                failed_accept_num++;
                spdlog::info("out of fd success {} failed {}", accept_num, failed_accept_num);
                close_fd(currentFd);
                return;
            }
            if(errno == EINTR){
                failed_accept_num++;
                spdlog::info("signal break ");
                continue;
            }
            failed_accept_num++;
            spdlog::info("unexpect err:{} success {} failed {}", errno, accept_num, failed_accept_num);
            close_fd(currentFd);
            return;
        }else{
            struct epoll_event ev;
            EpollEventContext* newCtx = new EpollEventContext(newFd, false);
            ev.data.ptr = newCtx;
            ev.events = flag;
            if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newFd, &ev) != 0){
                spdlog::info("accept modify new fd ");
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, newFd, &ev);
            }
            newCtx->set_noblocking(newFd);
            addContext(newCtx);
            char newIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, newIP, 10);
            accept_num++;
            // spdlog::info("accept new fd {} from ip {}:{} ", newFd, std::string(newIP), ntohs(addr.sin_port));
        }
    }
}

void EpollManager::do_read(EpollEventContext *ctx)
{
    int current_fd = ctx->getFd();
    while(1){
        int read_bytes = recv(current_fd, ctx->readBuffer.data() + ctx->offsetPos, MAX_EPOLL_READ_SIZE, 0);
        if(read_bytes == 0){
            // 断开连接
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
            EpollManager::delContext(current_fd);
            spdlog::info(" close fd {}", current_fd);
            return;
        }
        if(read_bytes < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // 暂时没数据
                return;
            }
            if(errno == ECONNRESET){
                //对端关闭连接
                spdlog::info("remote close connect when recving {}", current_fd);
                close_fd(current_fd);
                return;
            }
            if(errno == EINTR){
                // 信号中断
                spdlog::warn("signal break when recving {}", current_fd);
                continue;;
            }
            if(errno == EBADF){
                spdlog::warn("bad fd {}", current_fd);
                close_fd(current_fd);
                return;
            }
            if(errno == ENOTCONN){
                // 已经关闭的连接未从epoll移除可能出现
                spdlog::warn("old closed epoll fd not del {}", current_fd);
                close_fd(current_fd);
                return;
            }
            spdlog::warn("unknow err for epoll fd recv {} err:{} msg: {} offset: {}", current_fd, errno, ctx->readBuffer.data(), ctx->offsetPos);
            close_fd(current_fd);
            return;
        }
        ctx->readBytes(read_bytes);
    }
}

void EpollManager::do_conn(epoll_event &ev, EpollEventContext *ctx)
{
    int currentFd = ctx->getFd();
    int error;
    socklen_t len = sizeof(error);
    spdlog::info("do connect {}", currentFd);
    if(getsockopt(currentFd, SOL_SOCKET, SO_ERROR, &error, &len) == -1){
        spdlog::info("connect failed {} err {}", currentFd, strerror(errno));
        close_fd(currentFd);
        return;
    }
    if(error != 0){
        spdlog::error("connecting failed {} {}", currentFd, strerror(error));
        close_fd(currentFd);
        return;
    }
    ev.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, currentFd, &ev) == -1){
        spdlog::error("epoll mod connect fd failed ");
        return;
    }
    spdlog::info("epoll connect finished fd {}", currentFd);
    ctx->connectFinish();
}

void EpollManager::do_send(epoll_event &ev, EpollEventContext *ctx)
{
    int current_fd = ctx->getFd();
    ctx->set_noblocking(current_fd);
    while(1)
    {
        if(ctx->sendQ.size() == 0){
            ev.events &= ~EPOLLOUT;
            if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &ev) < 0){
                spdlog::info(" modify fd failed {}", errno);
                close_fd(epoll_fd);
                return;
            }
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
                spdlog::info("send buffer full !");
                return;
            }
            if(errno == EPIPE){
                // 对方关闭连接
                spdlog::info(" remote closed when send msg {}",current_fd);
                close_fd(current_fd);
                return;
            }
            if(errno == ENOTSOCK){
                spdlog::info(" send target not a fd {}", current_fd);
                close_fd(current_fd);
                return;
            }
            if(errno == EINVAL){
                spdlog::info(" some args is wrong when send to fd {}", current_fd);
                close_fd(current_fd);
                return;
            }
            if(errno == ECONNRESET){
                spdlog::info(" remote reset when send to fd {}", current_fd);
                close_fd(current_fd);
                return;
            }
            spdlog::info(" unexpected err when send to fd {} err {}", current_fd, errno);
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
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    delContext(fd);
}

bool EpollManager::sendMsg(int fd, const char *msg, int size)
{
    EpollEventContext* ctx = getContext(fd);
    if(ctx == nullptr){
        return false;
    }
    ctx->pushMsgQ(msg, size);
    struct epoll_event ev;
    ev.data.ptr = ctx;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    return true;
}
