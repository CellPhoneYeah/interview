#include "epoll_manager.h"

#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include <ctime>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#include "slog.h"

#include "ellnet/epoll_event_context.h"
#include "ellnet/epoll_net.h"
namespace ellnet
{
    std::unordered_map<int, EpollEventContext *> EpollManager::id2contexts_;

    void EpollManager::AddContext(EpollEventContext *eec)
    {
        int id = eec->GetId();
        int fd = eec->GetFd();
        EpollEventContext *pCtx = id2contexts_[id];
        if (pCtx != nullptr)
        {
            SPDLOG_WARN("repeat insert contex {} !!!", id);
        }
        id2contexts_[id] = eec;
        fd2contexts_[fd] = eec;
        SPDLOG_INFO("add id {} fd {} left ctx {}", id, fd, id2contexts_.size());
    }

    void EpollManager::DelContext(const int id)
    {
        auto it = id2contexts_.find(id);
        if (it != id2contexts_.end())
        {
            int fd = it->second->GetFd();
            delete (it->second);
            delete fd2contexts_.find(fd)->second;
            fd2contexts_.erase(fd);
        }
        id2contexts_.erase(id);
        SPDLOG_INFO("del id {} left ctx {}", id, id2contexts_.size());
    }
    EpollEventContext *EpollManager::GetContext(const int id)
    {
        auto it = id2contexts_.find(id);
        if (it == id2contexts_.end())
        {
            return nullptr;
        }
        else
        {
            return it->second;
        }
    }

    EpollManager::EpollManager(const int pipe_in_fd)
    {
        Init(pipe_in_fd);
    }

    EpollManager::~EpollManager()
    {
        if (id2contexts_.size() == 0)
        {
            return;
        }
        auto it = id2contexts_.begin();
        int cur_id;
        while (it != id2contexts_.end())
        {
            cur_id = it->first;
            
            EpollEventContext *pCtx = id2contexts_[cur_id];
            if (pCtx != nullptr)
            {
                delete (pCtx);
            }
            SPDLOG_INFO("del context {}", cur_id);
            SysCloseFd(pCtx->GetFd());
            it = id2contexts_.find(cur_id);
            if (it != id2contexts_.end())
            {
                it = id2contexts_.erase(it);
            }
        }
    }

    int EpollManager::Init(const int pipe_in_fd)
    {
        pipe_fd_ = pipe_in_fd;
        epoll_fd_ = epoll_create1(0);
        total_accept_num_ = 0;
        total_accept_failed_num_ = 0;
        connected_num_ = 0;
        connecting_num_ = 0;
        listening_num_ = 0;

        new_sock_num_ = 0;
        close_sock_num_ = 0;

        NewPipe(pipe_in_fd);
        SPDLOG_INFO("epollmanager init {} ", epoll_fd_);
        return epoll_fd_;
    }

    void EpollManager::SysCloseFd(const int fd)
    {
        if(fd <= 0){
            return;
        }
        ::close(fd);
        close_sock_num_++;
    }

    int EpollManager::SysNewFd()
    {
        int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        new_sock_num_++;
        return fd;
    }

    int EpollManager::LivingCount()
    {
        auto it = id2contexts_.begin();
        int count = 0;
        while (it != id2contexts_.end())
        {
            if (!it->second->IsDead())
            {
                count++;
            }
            it++;
        }
        return count;
    }

    bool EpollManager::NewPipe(const int pipe_fd)
    {
        struct epoll_event ev;
        EpollEventContext *newCtx = new EpollEventContext(pipe_fd);
        newCtx->SetPipe();
        this->AddContext(newCtx);
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = newCtx;
        EpollEventContext::SetNoblocking(pipe_fd);
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, pipe_fd, &ev) < 0)
        {
            SPDLOG_WARN("new pipe failed {} {} {}", strerror(errno), epoll_fd_, pipe_fd);
            return false;
        }
        return true;
    }

    void EpollManager::Run()
    {
        running_ = true;
        EpollNet::SetManagerState(1);
        while (IsRunning())
        {
            Loop();
        }
        EpollNet::SetManagerState(0);
    }

    void EpollManager::StartManager(const int pipe_fd)
    {
        EpollManager *emgr = new EpollManager(pipe_fd);
        emgr->Run();
    }

    int EpollManager::ListeningFd(std::string &addr, const int port)
    {
        auto it = id2contexts_.begin();
        while (it != id2contexts_.end())
        {
            if (it->second->IsListening(addr, port))
            {
                return it->second->GetFd();
            }
            it++;
        }
        return -1;
    }

    int EpollManager::StartListen(std::string ip_str, const int port)
    {
        SPDLOG_INFO("StartListen {}:{} ", ip_str, port);
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip_str.c_str());

        addr.sin_port = htons(port);
        int listenfd = SysNewFd();
        int opt;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
        {
            SPDLOG_INFO("sock set reuse failed {} !", strerror(errno));
            return -6;
        }
        if (listenfd < 0)
        {
            SPDLOG_INFO("sock create failed", strerror(errno));
            return -1;
        }
        if (bind(listenfd, (sockaddr *)&addr, len) < 0)
        {
            SPDLOG_INFO("sock bind failed:{}", strerror(errno));
            return -2;
        }

        if (addr.sin_addr.s_addr == INADDR_NONE)
        {
            SPDLOG_INFO("invalid ip addr ", ip_str);
            SysCloseFd(listenfd);
            return -4;
        }

        // SOMAXCONN is important
        if (listen(listenfd, SOMAXCONN) < 0)
        {
            SPDLOG_INFO("listen failed ", strerror(errno));
            return -5;
        }

        struct epoll_event ev;
        EpollEventContext *newCtx = new EpollEventContext(listenfd, true);
        newCtx->SetAddrPort(ip_str, port);
        AddContext(newCtx);
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = newCtx;
        EpollEventContext::SetNoblocking(listenfd);
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listenfd, &ev) < 0)
        {
            DelContext(listenfd);
            SPDLOG_INFO("stop listen fd");
            return -3;
        }
        SPDLOG_INFO("{} listen to fd ", listenfd);
        listening_num_++;
        last_tick_ = std::time(nullptr);
        return 0;
    }

    int EpollManager::StopListen(std::string addr, const int port)
    {
        int fd = ListeningFd(addr, port);
        if (fd < 0)
        {
            return 0;
        }
        CloseFd(fd);
    }

    int EpollManager::OpenConnection(int id)
    {
        EpollEventContext* ctx = GetContext(id);
        if(ctx == nullptr){
            SPDLOG_ERROR("open connection failed {}", id);
            return -1;
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ctx->.c_str(), &addr.sin_addr);
        addr.sin_port = htons(port);
        int client_fd = SysNewFd();
        socklen_t addrlen = sizeof(addr);
        int ret = connect(client_fd, (const sockaddr *)&addr, addrlen);
        if (ret == -1 && errno != EINPROGRESS)
        {
            SPDLOG_INFO("{} connect to {}:{} ret:{} errno {} failed!", client_fd, ip_str, port, ret, errno);
            return -1;
        }
        if (ret == 0)
        {
            SPDLOG_INFO("connect finish {}", client_fd);
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET; // 已经完成连接，直接开始接收数据
            EpollEventContext *ctx = new EpollEventContext(client_fd, false);
            ctx->ConnectFinish();
            ev.data.ptr = ctx;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) < 0)
            {
                epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);
            }
            connected_num_++;
            AddContext(ctx);
        }
        else
        {
            struct epoll_event ev;
            ev.events = EPOLLOUT | EPOLLET;
            EpollEventContext *ctx = new EpollEventContext(client_fd, false);
            ctx->ConnectStart();
            ev.data.ptr = ctx;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) < 0)
            {
                epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);
            }
            AddContext(ctx);
            connecting_num_++;
            SPDLOG_INFO("waiting connect {}", client_fd);
        }

        return client_fd;
    }

    int EpollManager::ConnectTo(std::string ip_str, const int port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip_str.c_str(), &addr.sin_addr);
        addr.sin_port = htons(port);
        int client_fd = SysNewFd();
        socklen_t addrlen = sizeof(addr);
        int ret = connect(client_fd, (const sockaddr *)&addr, addrlen);
        if (ret == -1 && errno != EINPROGRESS)
        {
            SPDLOG_INFO("{} connect to {}:{} ret:{} errno {} failed!", client_fd, ip_str, port, ret, errno);
            return -1;
        }
        if (ret == 0)
        {
            SPDLOG_INFO("connect finish {}", client_fd);
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET; // 已经完成连接，直接开始接收数据
            EpollEventContext *ctx = new EpollEventContext(client_fd, false);
            ctx->ConnectFinish();
            ev.data.ptr = ctx;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) < 0)
            {
                epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);
            }
            connected_num_++;
            AddContext(ctx);
        }
        else
        {
            struct epoll_event ev;
            ev.events = EPOLLOUT | EPOLLET;
            EpollEventContext *ctx = new EpollEventContext(client_fd, false);
            ctx->ConnectStart();
            ev.data.ptr = ctx;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) < 0)
            {
                epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);
            }
            AddContext(ctx);
            connecting_num_++;
            SPDLOG_INFO("waiting connect {}", client_fd);
        }

        return client_fd;
    }

    int EpollManager::Loop()
    {
        int Loopcount = 0;
        for(;;)
        {
            if(!IsRunning()){
                break;
            }
            Loopcount++;
            time_t current_time = std::time(nullptr);
            if (std::difftime(current_time, last_tick_) > 20)
            {
                last_tick_ = current_time;
                std::ostringstream oss;
                oss << std::this_thread::get_id();
                SPDLOG_INFO(
                    " connection total :{} listening: {} connected:{} connecting:{} living:{} thread:{}",
                    id2contexts_.size(),
                    listening_num_,
                    connected_num_,
                    connecting_num_,
                    LivingCount(),
                    oss.str()
                );
            }
            struct epoll_event ev_list[kMaxEpollEventNum];
            int eventn = epoll_wait(epoll_fd_, ev_list, kMaxEpollEventNum, -1); //阻塞直到有事件到达
            if (eventn < 0)
            {
                if (errno == EINTR)
                {
                    // 信号中断
                    SPDLOG_INFO("epoll break by signal ");
                    continue;
                }
                SPDLOG_INFO("epoll wait err:{}", std::strerror(errno));
                break;
            }
            if (eventn == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            for (int i = 0; i < eventn; i++)
            {
                struct epoll_event ev = ev_list[i];
                EpollEventContext *ctx = (EpollEventContext *)ev.data.ptr;
                if (ev.events & EPOLLIN)
                {
                    if (ctx->IsListening())
                    {
                        DoAccept(ev, ctx);
                    }
                    else if (ctx->IsPipe())
                    {
                        DoReadPipe(ctx);
                    }
                    else
                    {
                        DoRead(ctx);
                    }
                }
                else if (ev.events & EPOLLOUT)
                {
                    if (ctx->IsConnecting())
                    {
                        DoConn(ev, ctx);
                    }
                    else
                    {
                        DoSend(ev, ctx);
                    }
                }
                else if (ev.events & EPOLLERR)
                {
                    SPDLOG_WARN("epoll err {}", ctx->GetFd());
                    CloseFd(ctx->GetFd());
                }
                else if (ev.events & EPOLLHUP)
                {
                    SPDLOG_WARN("{} epoll wait found remote closed sock {}", i, ctx->GetFd());
                    CloseFd(ctx->GetFd());
                }
                else
                {
                    SPDLOG_WARN("other event ");
                    CloseFd(ctx->GetFd());
                }
            }
        }
        return 0;
    }

    void EpollManager::DoAccept(epoll_event &ev, EpollEventContext *ctx)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int flag = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
        int currentFd = ctx->GetFd();
        int singleAccept = 0;
        for(;;)
        {
            int newFd = accept(currentFd, (sockaddr *)&addr, &len);
            if (newFd < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    SPDLOG_INFO("done all accept connected_num {} single num {} ", connected_num_, singleAccept);
                    // 所有连接处理完
                    break;
                }
                if (errno == ECONNABORTED)
                {
                    // 事件处理前，对方断开
                    total_accept_failed_num_++;
                    SPDLOG_INFO("remote disconnected");
                    continue;
                }
                if (errno == EMFILE)
                {
                    total_accept_failed_num_++;
                    SPDLOG_INFO("file fd limit success {} failed {}", total_accept_num_, total_accept_failed_num_);
                    CloseFd(currentFd);
                    break;
                }
                if (errno == ENFILE)
                {
                    total_accept_failed_num_++;
                    SPDLOG_INFO("out of fd success {} failed {}", total_accept_num_, total_accept_failed_num_);
                    CloseFd(currentFd);
                    break;
                }
                if (errno == EINTR)
                {
                    total_accept_failed_num_++;
                    SPDLOG_INFO("signal break ");
                    break;
                }
                total_accept_failed_num_++;
                SPDLOG_INFO("unexpect err:{} success {} failed {}", errno, total_accept_num_, total_accept_failed_num_);
                CloseFd(currentFd);
                break;
            }
            else
            {
                struct epoll_event ev;
                EpollEventContext *newCtx = new EpollEventContext(newFd, false);
                ev.data.ptr = newCtx;
                ev.events = flag;
                EpollEventContext::SetNoblocking(newFd);
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, newFd, &ev) != 0)
                {
                    SPDLOG_INFO("accept modify new fd ");
                    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, newFd, &ev);
                }
                AddContext(newCtx);
                char newIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &addr.sin_addr, newIP, 10);
                total_accept_num_++;
                connected_num_++;
                singleAccept++;
                SPDLOG_INFO("accept new fd {} from ip {}:{} ", newFd, std::string(newIP), ntohs(addr.sin_port));
            }
        }
    }

    void EpollManager::DoRead(EpollEventContext *ctx)
    {
        int current_fd = ctx->GetFd();
        EpollEventContext::SetNoblocking(current_fd);
        while (1)
        {
            int read_bytes = recv(current_fd, ctx->read_buffer_.data() + ctx->offset_pos_, kMaxEpollReadSize, 0);
            if (read_bytes == 0)
            {
                // 断开连接
                epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, current_fd, nullptr);
                EpollManager::DelContext(current_fd);
                connected_num_--;
                SPDLOG_INFO(" close fd {}", current_fd);
                return;
            }
            if (read_bytes < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // 暂时没数据
                    return;
                }
                if (errno == ECONNRESET)
                {
                    // 对端关闭连接
                    SPDLOG_INFO("remote close connect when recving {}", current_fd);
                    CloseFd(current_fd);
                    return;
                }
                if (errno == EINTR)
                {
                    // 信号中断
                    SPDLOG_WARN("signal break when recving {}", current_fd);
                    continue;
                    ;
                }
                if (errno == EBADF)
                {
                    SPDLOG_WARN("bad fd {}", current_fd);
                    CloseFd(current_fd);
                    return;
                }
                if (errno == ENOTCONN)
                {
                    // 已经关闭的连接未从epoll移除可能出现
                    SPDLOG_WARN("old closed epoll fd not del {}", current_fd);
                    CloseFd(current_fd);
                    return;
                }
                SPDLOG_WARN("unknow err for epoll fd recv {} err:{} msg: {} offset: {}", current_fd, errno, ctx->read_buffer_.data(), ctx->offset_pos_);
                CloseFd(current_fd);
                return;
            }
            ctx->ReadBytes(read_bytes);
        }
    }

    void EpollManager::DoConn(epoll_event &ev, EpollEventContext *ctx)
    {
        int currentFd = ctx->GetFd();
        int errcode;
        socklen_t len = sizeof(errcode);
        // SPDLOG_INFO("do connect {}", currentFd);
        if (getsockopt(currentFd, SOL_SOCKET, SO_ERROR, &errcode, &len) == -1)
        {
            SPDLOG_INFO("connect failed {} err {}", currentFd, strerror(errno));
            CloseFd(currentFd);
            return;
        }
        if (errcode != 0)
        {
            SPDLOG_ERROR("connecting failed {} {}", currentFd, strerror(errcode));
            CloseFd(currentFd);
            return;
        }
        ev.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, currentFd, &ev) == -1)
        {
            SPDLOG_ERROR("epoll mod connect fd failed ");
            return;
        }
        // SPDLOG_INFO("epoll connect finished fd {}", currentFd);
        ctx->ConnectFinish();
        connecting_num_--;
        connected_num_++;
    }

    void EpollManager::DoSend(epoll_event &ev, EpollEventContext *ctx)
    {
        int current_fd = ctx->GetFd();
        EpollEventContext::SetNoblocking(current_fd);
        while (1)
        {
            if (ctx->send_q_.size() == 0)
            {
                ev.events &= ~EPOLLOUT;
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, current_fd, &ev) < 0)
                {
                    SPDLOG_INFO(" modify fd failed {}", errno);
                    CloseFd(epoll_fd_);
                    return;
                }
                ctx->ClearSendQ();
                return;
            }
            auto msg = ctx->send_q_.front();
            int sent = send(current_fd, msg.data() + ctx->offset_pos_, msg.size() - ctx->offset_pos_, MSG_NOSIGNAL);
            if (sent < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // 发送缓冲满了, 再次注册等待下次发送
                    ev.events &= EPOLLOUT | EPOLLET;
                    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, current_fd, &ev);
                    SPDLOG_INFO("send buffer full !");
                    return;
                }
                if (errno == EPIPE)
                {
                    // 对方关闭连接
                    SPDLOG_INFO(" remote closed when send msg {}", current_fd);
                    CloseFd(current_fd);
                    return;
                }
                if (errno == ENOTSOCK)
                {
                    SPDLOG_INFO(" send target not a fd {}", current_fd);
                    CloseFd(current_fd);
                    return;
                }
                if (errno == EINVAL)
                {
                    SPDLOG_INFO(" some args is wrong when send to fd {}", current_fd);
                    CloseFd(current_fd);
                    return;
                }
                if (errno == ECONNRESET)
                {
                    SPDLOG_INFO(" remote reset when send to fd {}", current_fd);
                    CloseFd(current_fd);
                    return;
                }
                SPDLOG_INFO(" unexpected err when send to fd {} err {}", current_fd, errno);
                return;
            }
            if (sent < (int)msg.size())
            {
                ctx->offset_pos_ += sent;
                continue;
            }
            ctx->send_q_.pop();
            ctx->offset_pos_ = 0;
        }
    }

    void EpollManager::DoReadPipe(EpollEventContext *ctx)
    {
        ControlCommand cmd;
        int pipe_fd = ctx->GetFd();
        while (1)
        {
            int n = read(pipe_fd, &cmd, sizeof(cmd));
            if (n == 0)
            {
                SPDLOG_INFO("reader pipe closed {}", pipe_fd);
                close(pipe_fd);
                return;
            }
            if (n < 0)
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    SPDLOG_WARN("read pip err :{}", strerror(errno));
                }
                break;
            }
            SPDLOG_INFO("read from pipe {}", (int)cmd.cmd);
            switch (cmd.cmd)
            {
            case CommandType::CMD_START_LISTEN:
                SPDLOG_INFO("start listen");
                StartListen(cmd.ipaddr, cmd.port);
                break;

            case CommandType::CMD_STOP_LISTEN:
                SPDLOG_INFO("stop listen");
                StopListen(cmd.ipaddr, cmd.port);
                break;

            case CommandType::CMD_CONNECT_TO:
                SPDLOG_INFO("connect");
                ConnectTo(cmd.ipaddr, cmd.port);
                break;

            case CommandType::CMD_OPEN_CONNECT:
                break;

            case CommandType::CMD_DISCONNECT:
                SPDLOG_INFO("disconnect");
                break;

            case CommandType::CMD_EXIT:
                SPDLOG_INFO("exit");
                break;

            default:
                SPDLOG_INFO("unknow cmd:{}", (int)cmd.cmd);
                break;
            }
        }
    }

    void EpollManager::CloseFd(const int fd)
    {
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
        SysCloseFd(fd);
        EpollEventContext *pCtx = GetContext(fd);
        if (pCtx != nullptr)
        {
            if (pCtx->IsListening())
            {
                listening_num_--;
            }
            if (pCtx->IsConnecting())
            {
                connecting_num_--;
            }
            connected_num_--;
        }
        DelContext(fd);
    }

    bool EpollManager::SendMsg(const int fd, const char *msg, const int size)
    {
        EpollEventContext *ctx = GetContext(fd);
        if (ctx == nullptr)
        {
            return false;
        }
        ctx->PushMsgQ(msg, size);
        struct epoll_event ev;
        ev.data.ptr = ctx;
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
        epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
        return true;
    }

}
