#include <sys/epoll.h>
#include "EpollManager.h"
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "EpollEventContext.h"
#include <unistd.h>
#include <ctime>
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
    std::cout << "del context " << fd << std::endl;
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
        std::cout << "del context " << cur_fd << std::endl;
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
        std::cout << "sock create failed" << errno << std::endl;
        return -1;
    }
    if(bind(listenfd, (sockaddr*)&addr, len) < 0){
        std::cout << "sock bind failed:" << errno << std::endl;
        return -2;
    }

    if(addr.sin_addr.s_addr == INADDR_NONE){
        std::cout << "invalid ip addr " << ip_str << std::endl;
        close(listenfd);
        return -4;
    }

    if(listen(listenfd, 0) < 0){
        std::cout << "listen failed " << strerror(errno) << std::endl;
        return -5;
    }

    struct epoll_event ev;
    EpollEventContext* newCtx = new EpollEventContext(listenfd, true);
    this->addContext(newCtx);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = newCtx;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &ev) < 0){
        delContext(listenfd);
        std::cout << "stop listen fd";
        return -3;
    }
    std::cout << listenfd << " listen to fd " << std::endl;
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
        std::cout << client_fd <<  " connect to " << ip_str << ":" << port << " ret " << ret << " failed!" << errno << std::endl;
        return -1;
    }
    if(ret == 0){
        std::cout << "connect finish " << client_fd << std::endl;
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
        std::cout << "waiting connect " << client_fd << std::endl;
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
            std::cout << " connection nums :" << contexts.size() << " loopcount " << loopcount << std::endl; 
        }
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
        if(eventn == 0){
            // sleep(1);
            // std::cout << "no event ..." << std::endl;
            return 0;
        }
        // std::cout << "event n " << eventn << std::endl;
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
                std::cout << "epoll err " << ctx->getFd() << std::endl;
                close_fd(ctx->getFd());
            }else if(ev.events & EPOLLHUP){
                std::cout << i << " epoll wait found remote closed sock " << ctx->getFd() << std::endl;
                close_fd(ctx->getFd());
            }else{
                std::cout << "other event " << ev.events << std::endl;
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
    // ctx->set_noblocking(currentFd);
    while(1){
        std::cout << "handle accept " << std::endl;
        int newFd = accept(currentFd, (sockaddr*)&addr, &len);
        std::cout << "handle accept newfd " << newFd << " err " << errno << std::endl;
        if(newFd < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                std::cout << "done all accept " << std::endl;
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
                close_fd(currentFd);
                return;
            } 
            if(errno == ENFILE){
                std::cout << "out of fd " << std::endl;
                close_fd(currentFd);
                return;
            }
            if(errno == EINTR){
                std::cout << "signal break " << std::endl;
                continue;
            }
            std::cout << "unexpect err:" << errno << std::endl;
            close_fd(currentFd);
            return;
        }else{
            struct epoll_event ev;
            EpollEventContext* newCtx = new EpollEventContext(newFd, false);
            ev.data.ptr = newCtx;
            ev.events = flag;
            if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newFd, &ev) != 0){
                std::cout << "accept modify new fd \n";
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, newFd, &ev);
            }
            newCtx->set_noblocking(newFd);
            addContext(newCtx);
            char newIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, newIP, 10);
            std::cout << "accept new fd " << newFd << " from ip " << std::string(newIP) << ":" << ntohs(addr.sin_port) << std::endl;
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
            std::cout << " close fd " << current_fd << std::endl;
            return;
        }
        if(read_bytes < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // std::cout << " no data to read .." << std::endl;
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
            std::cout << "unknow err for epoll fd recv " << current_fd << " err: " << errno << " " << ctx->readBuffer.data() << " " << ctx->offsetPos << std::endl;
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
    std::cout << "do connect " << currentFd << std::endl;
    if(getsockopt(currentFd, SOL_SOCKET, SO_ERROR, &error, &len) == -1){
        std::cout << "connect failed " << currentFd << " err " << strerror(errno) << std::endl;
        close_fd(currentFd);
        return;
    }
    if(error != 0){
        std::cerr << "connecting failed " << currentFd << " " << strerror(error) << std::endl;
        close_fd(currentFd);
        return;
    }
    ev.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, currentFd, &ev) == -1){
        perror("epoll mod connect fd failed ");
        return;
    }
    std::cout << "epoll connect finished fd " << currentFd << std::endl;
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
                std::cout << " modify fd failed " << errno << std::endl;
                close_fd(epoll_fd);
                return;
            }
            ctx->clearSendQ();
            // std::cout << "send queue has cleaned!" << current_fd << std::endl;
            return;
        }
        auto msg = ctx->sendQ.front();
        // std::cout << "sending msg :" << msg.data() << std::endl;
        int sent = send(current_fd, msg.data() + ctx->offsetPos, msg.size() - ctx->offsetPos, MSG_NOSIGNAL);
        if(sent < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // 发送缓冲满了, 再次注册等待下次发送
                ev.events &= EPOLLOUT | EPOLLET;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &ev);
                std::cout << "send buffer full !" << std::endl;
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
        // std::cout << "send finish one msg" << std::endl;
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
    // std::cout << " after push queue " << ctx->sendQ.size() << std::endl;
    return true;
}
