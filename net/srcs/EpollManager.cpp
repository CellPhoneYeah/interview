#include <sys/epoll.h>
#include "EpollManager.h"
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "EpollEventContext.h"
#include <unistd.h>
#include <ctime>
#include "slog.h"
#include <string.h>
#include <thread>
#include <sstream>

std::unordered_map<int, EpollEventContext*> EpollManager::contexts;

void EpollManager::addContext(EpollEventContext * eec)
{
    int fd = eec->getFd();
    EpollEventContext* pCtx = contexts[fd];
    if(pCtx != nullptr){
        SPDLOG_WARN("repeat insert contex {} !!!", fd);
    }
    contexts[fd] = eec;
    SPDLOG_INFO("add fd {} left ctx {}", fd, contexts.size());
}

void EpollManager::delContext(int fd)
{
    auto it = contexts.find(fd);
    if(it != contexts.end()){
        delete(it->second);
    }
    contexts.erase(fd);
    SPDLOG_INFO("del fd {} left ctx {}", fd, contexts.size());
}
EpollEventContext* EpollManager::getContext(int fd)
{
    auto it = contexts.find(fd);
    if(it == contexts.end()){
        return nullptr;
    }else{
        return it->second;
    }
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
        SPDLOG_INFO("del context {}", cur_fd);
        sys_close_fd(cur_fd);
        it = contexts.find(cur_fd);
        if(it != contexts.end()){
            it = contexts.erase(it);
        }
    }
}

int EpollManager::init()
{
    epoll_fd = epoll_create1(0);
    total_accept_num = 0;
    total_accept_failed_num = 0;
    connected_num = 0;
    connecting_num = 0;
    listening_num = 0;

    new_sock_num = 0;
    close_sock_num = 0;
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    SPDLOG_INFO("epollmanager init {} {}", oss.str(), epoll_fd);
    return epoll_fd;
}

void EpollManager::sys_close_fd(int fd)
{
    ::close(fd);
    close_sock_num++;
}

int EpollManager::sys_new_fd()
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    new_sock_num++;
    return fd;
}

int EpollManager::livingCount()
{
    auto it = contexts.begin();
    int count = 0;
    while(it != contexts.end()){
        if(!it->second->isDead()){
            count ++;
        }
        it++;
    }
    return count;
}

bool EpollManager::newPipe(int pipe_fd_out)
{
    struct epoll_event ev;
    EpollEventContext* newCtx = new EpollEventContext(pipe_fd_out);
    newCtx->setPipe();
    this->addContext(newCtx);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = newCtx;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_fd_out, &ev) < 0){
        SPDLOG_WARN("new pipe failed {}", strerror(errno));
        return false;
    }
    return true;
}

int EpollManager::start_listen(std::string ip_str, int port)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_str.c_str());
    
    addr.sin_port = htons(port);
    int listenfd = sys_new_fd();
    int opt;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0){
        SPDLOG_INFO("sock set reuse failed {} !", strerror(errno));
        return -6;
    }
    if(listenfd < 0){
        SPDLOG_INFO("sock create failed", errno);
        return -1;
    }
    if(bind(listenfd, (sockaddr*)&addr, len) < 0){
        SPDLOG_INFO("sock bind failed:{}", errno);
        return -2;
    }

    if(addr.sin_addr.s_addr == INADDR_NONE){
        SPDLOG_INFO("invalid ip addr ", ip_str);
        sys_close_fd(listenfd);
        return -4;
    }

    if(listen(listenfd, SOMAXCONN) < 0){
        SPDLOG_INFO("listen failed ", strerror(errno));
        return -5;
    }

    struct epoll_event ev;
    EpollEventContext* newCtx = new EpollEventContext(listenfd, true);
    this->addContext(newCtx);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = newCtx;
    newCtx->set_noblocking(listenfd);
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &ev) < 0){
        delContext(listenfd);
        SPDLOG_INFO("stop listen fd");
        return -3;
    }
    SPDLOG_INFO("{} listen to fd ", listenfd);
    listening_num++;
    last_tick = std::time(nullptr);
    return 0;
}

int EpollManager::connect_to(std::string ip_str, int port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_str.c_str(), &addr.sin_addr);
    addr.sin_port = htons(port);
    int client_fd = sys_new_fd();
    socklen_t addrlen = sizeof(addr);
    int ret = connect(client_fd, (const sockaddr*)&addr, addrlen);
    if(ret == -1 && errno != EINPROGRESS){
        SPDLOG_INFO("{} connect to {}:{} ret:{} errno {} failed!", client_fd, ip_str, port, ret, errno);
        return -1;
    }
    if(ret == 0){
        SPDLOG_INFO("connect finish {}", client_fd);
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET; // 已经完成连接，直接开始接收数据
        EpollEventContext * ctx = new EpollEventContext(client_fd, false);
        ctx->connectFinish();
        ev.data.ptr = ctx;
        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0){
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
        }
        connected_num++;
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
        connecting_num++;
        SPDLOG_INFO("waiting connect {}", client_fd);
    }
    
    return client_fd;
}

int EpollManager::loop()
{
    int loopcount = 0;
    while(1){
        loopcount ++;
        time_t current_time = std::time(nullptr);
        if(std::difftime(current_time, last_tick) > 5){
            last_tick = current_time;
            std::ostringstream oss;
            oss << std::this_thread::get_id();
            SPDLOG_INFO(" connection total :{} listening: {} connected:{} connecting:{} living:{} thread:{}", contexts.size(), listening_num, connected_num, connecting_num, livingCount(), oss.str());
        }
        struct epoll_event ev_list[MAX_EPOLL_EVENT_NUM];
        int eventn = epoll_wait(epoll_fd, ev_list, MAX_EPOLL_EVENT_NUM, 100); // 立刻返回结果，不阻塞
        if(eventn < 0){
            if(errno == EINTR){
                // 信号中断
                SPDLOG_INFO("epoll break by signal ");
                continue;
            }
            SPDLOG_INFO("epoll wait err:{}", errno);
            break;
        }
        if(eventn == 0){
            // SPDLOG_INFO("epoll nothing {} total_accept {} failed {}", contexts.size(), total_accept_num, total_accept_failed_num);
            return 0;
        }
        // SPDLOG_INFO(" get epoll event {}", eventn);
        for (int i = 0; i < eventn; i++)
        {
            // SPDLOG_INFO(" connection total :{} listening: {} connected:{} connecting:{}", contexts.size(), listening_num, connected_num, connecting_num);
            struct epoll_event ev = ev_list[i];
            EpollEventContext* ctx = (EpollEventContext*)ev.data.ptr;
            if(ev.events & EPOLLIN){
                if(ctx->isListening()){
                    do_accept(ev, ctx);
                }else if(ctx->isPipe()){
                    do_read_pipe(ctx);
                }
                else{
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
                SPDLOG_WARN("epoll err {}", ctx->getFd());
                close_fd(ctx->getFd());
            }else if(ev.events & EPOLLHUP){
                SPDLOG_WARN("{} epoll wait found remote closed sock {}", i, ctx->getFd());
                close_fd(ctx->getFd());
            }else{
                SPDLOG_WARN("other event ");
                close_fd(ctx->getFd());
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
    int currentFd = ctx->getFd();
    ctx->set_noblocking(currentFd);
    int singleAccept = 0;
    while(1){
        int newFd = accept(currentFd, (sockaddr*)&addr, &len);
        if(newFd < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                SPDLOG_INFO("done all accept connected_num {} single num {} ", connected_num, singleAccept);
                // 所有连接处理完
                break;
            }
            if(errno == ECONNABORTED){
                // 事件处理前，对方断开
                total_accept_failed_num++;
                SPDLOG_INFO("remote disconnected");
                continue;
            }
            if(errno == EMFILE){
                total_accept_failed_num++;
                SPDLOG_INFO("file fd limit success {} failed {}", total_accept_num, total_accept_failed_num);
                close_fd(currentFd);
                break;
            } 
            if(errno == ENFILE){
                total_accept_failed_num++;
                SPDLOG_INFO("out of fd success {} failed {}", total_accept_num, total_accept_failed_num);
                close_fd(currentFd);
                break;
            }
            if(errno == EINTR){
                total_accept_failed_num++;
                SPDLOG_INFO("signal break ");
                break;
            }
            total_accept_failed_num++;
            SPDLOG_INFO("unexpect err:{} success {} failed {}", errno, total_accept_num, total_accept_failed_num);
            close_fd(currentFd);
            break;
        }else{
            struct epoll_event ev;
            EpollEventContext* newCtx = new EpollEventContext(newFd, false);
            ev.data.ptr = newCtx;
            ev.events = flag;
            newCtx->set_noblocking(newFd);
            if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newFd, &ev) != 0){
                SPDLOG_INFO("accept modify new fd ");
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, newFd, &ev);
            }
            addContext(newCtx);
            char newIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, newIP, 10);
            total_accept_num++;
            connected_num++;
            singleAccept++;
            SPDLOG_INFO("accept new fd {} from ip {}:{} ", newFd, std::string(newIP), ntohs(addr.sin_port));
        }
    }
}

void EpollManager::do_read(EpollEventContext *ctx)
{
    int current_fd = ctx->getFd();
    ctx->set_noblocking(current_fd);
    while(1){
        int read_bytes = recv(current_fd, ctx->readBuffer.data() + ctx->offsetPos, MAX_EPOLL_READ_SIZE, 0);
        if(read_bytes == 0){
            // 断开连接
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
            EpollManager::delContext(current_fd);
            connected_num--;
            SPDLOG_INFO(" close fd {}", current_fd);
            return;
        }
        if(read_bytes < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // 暂时没数据
                return;
            }
            if(errno == ECONNRESET){
                //对端关闭连接
                SPDLOG_INFO("remote close connect when recving {}", current_fd);
                close_fd(current_fd);
                return;
            }
            if(errno == EINTR){
                // 信号中断
                SPDLOG_WARN("signal break when recving {}", current_fd);
                continue;;
            }
            if(errno == EBADF){
                SPDLOG_WARN("bad fd {}", current_fd);
                close_fd(current_fd);
                return;
            }
            if(errno == ENOTCONN){
                // 已经关闭的连接未从epoll移除可能出现
                SPDLOG_WARN("old closed epoll fd not del {}", current_fd);
                close_fd(current_fd);
                return;
            }
            SPDLOG_WARN("unknow err for epoll fd recv {} err:{} msg: {} offset: {}", current_fd, errno, ctx->readBuffer.data(), ctx->offsetPos);
            close_fd(current_fd);
            return;
        }
        ctx->readBytes(read_bytes);
    }
}

void EpollManager::do_conn(epoll_event &ev, EpollEventContext *ctx)
{
    int currentFd = ctx->getFd();
    int errcode;
    socklen_t len = sizeof(errcode);
    // SPDLOG_INFO("do connect {}", currentFd);
    if(getsockopt(currentFd, SOL_SOCKET, SO_ERROR, &errcode, &len) == -1){
        SPDLOG_INFO("connect failed {} err {}", currentFd, strerror(errno));
        close_fd(currentFd);
        return;
    }
    if(errcode != 0){
        SPDLOG_ERROR("connecting failed {} {}", currentFd, strerror(errcode));
        close_fd(currentFd);
        return;
    }
    ev.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, currentFd, &ev) == -1){
        SPDLOG_ERROR("epoll mod connect fd failed ");
        return;
    }
    // SPDLOG_INFO("epoll connect finished fd {}", currentFd);
    ctx->connectFinish();
    connecting_num--;
    connected_num++;
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
                SPDLOG_INFO(" modify fd failed {}", errno);
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
                SPDLOG_INFO("send buffer full !");
                return;
            }
            if(errno == EPIPE){
                // 对方关闭连接
                SPDLOG_INFO(" remote closed when send msg {}",current_fd);
                close_fd(current_fd);
                return;
            }
            if(errno == ENOTSOCK){
                SPDLOG_INFO(" send target not a fd {}", current_fd);
                close_fd(current_fd);
                return;
            }
            if(errno == EINVAL){
                SPDLOG_INFO(" some args is wrong when send to fd {}", current_fd);
                close_fd(current_fd);
                return;
            }
            if(errno == ECONNRESET){
                SPDLOG_INFO(" remote reset when send to fd {}", current_fd);
                close_fd(current_fd);
                return;
            }
            SPDLOG_INFO(" unexpected err when send to fd {} err {}", current_fd, errno);
            return;
        }
        if(sent < (int)msg.size()){
            ctx->offsetPos += sent;
            continue;
        }
        ctx->sendQ.pop();
        ctx->offsetPos = 0;
    }
}

void EpollManager::do_read_pipe(EpollEventContext *ctx)
{
    char buff[64];
    int pipe_fd = ctx->getFd();
    while(1){
        int n = read(pipe_fd, buff, 64);
        if(n == 0){
            SPDLOG_INFO("reader pipe closed {}", pipe_fd);
            close(pipe_fd);
            return;
        }
        if(n < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
            }else{
                SPDLOG_WARN("read pip err :{}", strerror(errno));
            }
            break;
        }
        std::string str(buff);
        SPDLOG_INFO("read from pipe {}", buff);
        if(strcmp(str.c_str(), "conn") == 0){
            connect_to("127.0.0.1", 8088);
        }else{
            SPDLOG_INFO("unknow pipe cmd: {}", str);
        }
    }
    
}

void EpollManager::close_fd(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    sys_close_fd(fd);
    EpollEventContext* pCtx = getContext(fd);
    if(pCtx != nullptr){
        if(pCtx->isListening()){
            listening_num --;
        }
        if(pCtx->isConnecting()){
            connecting_num --;
        }
        connected_num --;
    }
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
