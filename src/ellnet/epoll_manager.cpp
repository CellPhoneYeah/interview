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

#include "epoll_event_context.h"
#include "epoll_net.h"
#include "epoll_net_header.h"
#include "memory_traker.h"

namespace ellnet
{
    std::mutex EpollManager::contextMtx;
    std::mutex EpollManager::isRunningMtx;
    EpollManager *EpollManager::pMgr = nullptr;
    int EpollManager::new_sock_num_ = 0;
    int EpollManager::close_sock_num_ = 0;
    int EpollManager::total_accept_num_ = 0;
    int EpollManager::total_accept_failed_num_ = 0;
    int EpollManager::connected_num_ = 0;
    int EpollManager::init_connect_num_ = 0;
    int EpollManager::init_listen_num_ = 0;
    int EpollManager::listening_num_ = 0;

    std::unordered_map<int, EpollEventContext *> EpollManager::sessionId2contexts_;
    std::unordered_map<int, EpollEventContext *> EpollManager::fd2contexts_;

    int EpollManager::NewFdAndBindContext()
    {
        int newFd = SysNewFd();
        EpollEventContext *newCtx = new EpollEventContext(newFd);
        SPDLOG_INFO("create new ctx and bind {} context size {}", newCtx->GetSessionId(), sizeof(EpollEventContext));
        AddContext(newCtx);
        return newCtx->GetSessionId();
    }

    void EpollManager::AddContext(EpollEventContext *pCtx)
    {
        std::unique_lock<std::mutex> lock(contextMtx);
        int id = pCtx->GetSessionId();
        int fd = pCtx->GetFd();
        EpollEventContext *oldPCtx = sessionId2contexts_[id];
        if (oldPCtx != nullptr)
        {
            SPDLOG_WARN("repeat insert contex {} !!!", id);
            throw std::runtime_error("repeat insert contex");
        }
        OnAddContext(pCtx);
        sessionId2contexts_[id] = pCtx;
        fd2contexts_[fd] = pCtx;
        SPDLOG_INFO("add id {} fd {} left ctx {}", id, fd, sessionId2contexts_.size());
    }

    void EpollManager::OnAddContext(EpollEventContext *pCtx)
    {
        switch (pCtx->GetState())
        {
        case SocketState::CONNECT_WAIT_OPEN:
            init_connect_num_++;
            break;

        case SocketState::LISTEN_WAIT_OPEN:
            init_listen_num_++;
            break;

        case SocketState::PIPE_LISTENING:
            SPDLOG_INFO("epoll manager pipe added {}", pCtx->GetSessionId());
            break;

        case SocketState::CLOSED:
            break;

        default:
            SPDLOG_ERROR("unknow new EpollEventContext state {}", (int)pCtx->GetState());
            break;
        }
    }

    void EpollManager::OnDelContext(EpollEventContext* pCtx){
        if (pCtx != nullptr)
        {
            if (pCtx->IsListening())
            {
                listening_num_--;
            }
            if (pCtx->IsConnecting())
            {
                init_connect_num_--;
            }
            connected_num_--;
        }
    }

    void EpollManager::DelContext(const int sessionId)
    {
        std::unique_lock<std::mutex> lock(contextMtx);
        auto it = sessionId2contexts_.find(sessionId);
        if (it != sessionId2contexts_.end())
        {
            EpollEventContext* pCtx = it->second;
            OnDelContext(pCtx);
            int fd = pCtx->GetFd();
            SPDLOG_INFO("to del id {} fd {} ", sessionId, fd);
            delete (pCtx);
            // delete fd2contexts_.find(fd)->second;
            fd2contexts_.erase(fd);
        }
        sessionId2contexts_.erase(sessionId);
        SPDLOG_INFO("del id {} left ctx {}", sessionId, sessionId2contexts_.size());
    }
    EpollEventContext *EpollManager::GetContext(const int sessionId)
    {
        auto it = sessionId2contexts_.find(sessionId);
        if (it == sessionId2contexts_.end())
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
        if (sessionId2contexts_.size() == 0)
        {
            return;
        }
        auto it = sessionId2contexts_.begin();
        int cur_id;
        while (it != sessionId2contexts_.end())
        {
            cur_id = it->first;
            
            EpollEventContext *pCtx = sessionId2contexts_[cur_id];
            if (pCtx != nullptr)
            {
                delete (pCtx);
            }
            SPDLOG_INFO("del context {}", cur_id);
            SysCloseFd(pCtx->GetFd());
            it = sessionId2contexts_.find(cur_id);
            if (it != sessionId2contexts_.end())
            {
                it = sessionId2contexts_.erase(it);
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
        init_connect_num_ = 0;
        listening_num_ = 0;

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
        auto it = sessionId2contexts_.begin();
        int count = 0;
        while (it != sessionId2contexts_.end())
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
        EpollEventContext *newCtx = new EpollEventContext(pipe_fd, SocketState::PIPE_LISTENING);
        newCtx->SetPipe();
        AddContext(newCtx);
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = newCtx;
        newCtx->SetNoblocking();
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, pipe_fd, &ev) < 0)
        {
            SPDLOG_WARN("new pipe failed {} {} {}", strerror(errno), epoll_fd_, pipe_fd);
            ::close(pipe_fd);
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
        try
        {
            pMgr = new EpollManager(pipe_fd);
            pMgr->Run();
            delete pMgr;
            pMgr = nullptr;
            SPDLOG_INFO("EpollManager run done");
        }
        catch (std::exception e)
        {
            SPDLOG_ERROR("Start Manager  run failed : {}", e.what());
        }
    }

    int EpollManager::ListeningFd(std::string &addr, const int port)
    {
        auto it = sessionId2contexts_.begin();
        while (it != sessionId2contexts_.end())
        {
            if (it->second->IsListening(addr, port))
            {
                return it->second->GetFd();
            }
            it++;
        }
        return -1;
    }

    int EpollManager::InitListen(const ControlCommand cmd)
    {
        if(cmd.sessionId <= 0){
            SPDLOG_ERROR("InitListen with illegal sessionId {}", cmd.sessionId);
            return -1;
        }
        if(cmd.ipaddr == nullptr){
            SPDLOG_ERROR("InitListen with a empty ipaddr");
            return -2;
        }
        if(cmd.port <= 0){
            SPDLOG_ERROR("InitListen with illegal port {}", cmd.port);
            return -3;
        }
        const int sessionId = cmd.sessionId;
        const char* ipaddr = cmd.ipaddr;
        const int port = cmd.port;
        EpollEventContext* ctx = GetContext(sessionId);
        if(ctx == nullptr){
            SPDLOG_ERROR("can not get a ctx for sessionId {} to init listen", sessionId);
            return -4;
        }
        SPDLOG_INFO("Init Listen {}:{} ", ipaddr, port);
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ipaddr);
        addr.sin_port = htons(port);
        int listenfd = ctx->GetFd();
        if (addr.sin_addr.s_addr == INADDR_NONE)
        {
            SPDLOG_INFO("invalid ip addr ", ipaddr);
            DelContext(sessionId);
            SysCloseFd(listenfd);
            return -5;
        }
        
        int opt;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
        {
            SPDLOG_INFO("sock set reuse failed {} !", strerror(errno));
            return -6;
        }
        if (bind(listenfd, (sockaddr *)&addr, len) < 0)
        {
            SPDLOG_INFO("sock bind failed:{}", strerror(errno));
            return -7;
        }
        std::string str(cmd.ipaddr);
        ctx->SetAddrPort(str, port);

        return 0;
    }

    int EpollManager::StartListen(ControlCommand cmd)
    {
        if(cmd.sessionId <= 0){
            SPDLOG_ERROR("InitListen with illegal sessionId {}", cmd.sessionId);
            return -1;
        }
        const int sessionId = cmd.sessionId;
        EpollEventContext* ctx = GetContext(sessionId);
        if(ctx == nullptr){
            SPDLOG_ERROR("can not get a ctx for sessionId {} to start listen", sessionId);
            return -2;
        }
        
        int listenfd = ctx->GetFd();

        // SOMAXCONN is important
        if (listen(listenfd, SOMAXCONN) < 0)
        {
            SPDLOG_INFO("listen failed ", strerror(errno));
            return -3;
        }

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = ctx;
        ctx->SetNoblocking();
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listenfd, &ev) < 0)
        {
            CloseFdAndDelCtx(sessionId);
            SPDLOG_INFO("stop listen fd {}", listenfd);
            return -4;
        }
        ChangeCtxState(ctx, LISTENING);
        SPDLOG_INFO("listen to fd {}", listenfd);
        return 0;
    }

    int EpollManager::CloseSocket(const ControlCommand cmd)
    {
        if(cmd.sessionId <= 0){
            SPDLOG_ERROR("CloseSocket with illegal sessionId {}", cmd.sessionId);
            return -1;
        }
        const int sessionId = cmd.sessionId;
        EpollEventContext* ctx = GetContext(sessionId);
        if(ctx == nullptr){
            SPDLOG_ERROR("not found a ctx for sessionId {} to close ", sessionId);
            return -2;
        }
        
        int listenfd = ctx->GetFd();

        switch (ctx->GetState())
        {
        case PIPE_LISTENING:
            SPDLOG_ERROR("can not close pipe session {} for cmd !", sessionId);
            return -4;
        case CLOSED:
            SPDLOG_ERROR("session {} has been closed, can not repeat close a socket!", sessionId);
            return -5;
        case LISTEN_WAIT_OPEN:
            SPDLOG_ERROR("to close a session {} whitch waiting listen open!", sessionId);
            CloseFdAndDelCtx(sessionId);
            break;

        case CONNECT_WAIT_OPEN:
            SPDLOG_ERROR("to close a session {} whitch waiting connect open!", sessionId);
            CloseFdAndDelCtx(sessionId);
            break;

        case LISTENING:
            SPDLOG_ERROR("to close a session {} whitch listening!", sessionId);
            CloseFdAndDelCtx(sessionId);
            break;

        case CONNECTED:
            SPDLOG_ERROR("to close a session {} whitch connected!", sessionId);
            CloseFdAndDelCtx(sessionId);
            break;

        default:
            SPDLOG_ERROR("unknow state {} to close!", (int)ctx->GetState());
            return -6;
        }
        return 0;
    }

    // 假如connect使用非阻塞模式，需要监听到out事件来感知连接建立完成
    int EpollManager::OpenConnection(int id)
    {
        EpollEventContext* ctx = GetContext(id);
        if(ctx == nullptr){
            SPDLOG_ERROR("open connection failed {}", id);
            return -1;
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        if(ctx->GetIpAddr().empty()){
            SPDLOG_ERROR("open connection failed case ipaddr empty {}", id);
            return -1;
        }
        inet_pton(AF_INET, ctx->GetIpAddr().c_str(), &addr.sin_addr);
        addr.sin_port = htons(ctx->GetPort());
        int client_fd = SysNewFd();
        socklen_t addrlen = sizeof(addr);
        int ret = connect(client_fd, (const sockaddr *)&addr, addrlen);
        if (ret == -1 && errno != EINPROGRESS)
        {
            SPDLOG_INFO("{} connect to {}:{} ret:{} errno {} failed!", client_fd, ctx->GetIpAddr(), ctx->GetPort(), ret, strerror(errno));
            return -1;
        }
        if (ret == 0)
        {
            SPDLOG_INFO("connect finish {}", client_fd);
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET; // 已经完成连接，直接开始接收数据
            ctx->SetFd(client_fd);
            ChangeCtxState(ctx, CONNECTED);
            ctx->finishOpen();
            ev.data.ptr = ctx;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) < 0)
            {
                epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);
            }
            connected_num_++;
        }
        else
        {
            struct epoll_event ev;
            ev.events = EPOLLOUT | EPOLLET;
            ctx->ConnectStart();
            ctx->finishOpen();
            ev.data.ptr = ctx;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) < 0)
            {
                epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);
            }
            init_connect_num_++;
            SPDLOG_INFO("waiting connect {}", client_fd);
        }

        return client_fd;
    }


    int EpollManager::InitConnect(const ControlCommand cmd){
        try
        {
            const std::string ip_str(cmd.ipaddr);
            const int port = cmd.port;
            const int sessionId = cmd.sessionId;
            if (cmd.sessionId <= 0)
            {
                SPDLOG_ERROR("InitConnect with illegal sessionId {}", sessionId);
                return -1;
            }
            if (ip_str.empty())
            {
                SPDLOG_ERROR("InitConnect with empty ip {}", ip_str);
                return -2;
            }
            if (port <= 0)
            {
                SPDLOG_ERROR("InitConnect with illegal port {}", port);
                return -3;
            }
            EpollEventContext *ctx = GetContext(sessionId);
            if (ctx == nullptr)
            {
                SPDLOG_ERROR("InitConnect can not find ctx for session {}", sessionId);
                return -4;
            }
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            inet_pton(AF_INET, ip_str.c_str(), &addr.sin_addr);
            addr.sin_port = htons(port);

            int client_fd = ctx->GetFd();
            socklen_t addrlen = sizeof(addr);
            ctx->SetBlocking();
            int ret = connect(client_fd, (const sockaddr *)&addr, addrlen);
            if (ret == -1 && errno != EINPROGRESS)
            {
                SPDLOG_INFO("{} connect to {}:{} ret:{} errno {} failed!", client_fd, ip_str, port, ret, strerror(errno));
                return -5;
            }
            // 创建连接后，不注册事件到epoll，需要等待StartConnect再开始监听事件
            if (ret == 0)
            {
                SPDLOG_INFO("connect to finish {}", client_fd);
                ctx->SetNoblocking(); // 建立连接完成后再使用非阻塞模式
            }
            else
            {
                SPDLOG_INFO("connect failed {}", ret);
                CloseFdAndDelCtx(sessionId);
                return -6;
            }
        }
        catch (const std::exception& e)
        {
            SPDLOG_ERROR("init connect failed {}", e.what());
            return -7;
        }

        return 0;
    }

    // InitConnect 已经初始了Context的上下文，只需要将fd注册事件到epoll
    int EpollManager::StartConnect(const ControlCommand cmd)
    {
        try
        {
            SPDLOG_INFO("StartConnect ");
            const int sessionId = cmd.sessionId;
            if (sessionId <= 0)
            {
                SPDLOG_ERROR("StartConnect with illegal sessionId {}", sessionId);
                return -1;
            }
            SPDLOG_INFO("StartConnect ");
            EpollEventContext *ctx = GetContext(sessionId);
            if (ctx == nullptr)
            {
                SPDLOG_ERROR("StartConnect can not find ctx for session {}", sessionId);
                return -2;
            }
            SPDLOG_INFO("StartConnect ");
            const int client_fd = ctx->GetFd();
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET; // 已经完成连接，直接开始接收数据
            ev.data.ptr = ctx;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) < 0)
            {
                SPDLOG_INFO("StartConnect ");
                epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);
            }
            ChangeCtxState(ctx, CONNECTED);
            SPDLOG_INFO("StartConnect ");

            return 0;
        }
        catch (std::exception e)
        {
            SPDLOG_ERROR("StartConnect failed {}", e.what());
            return -3;
        }
    }

    int EpollManager::Loop()
    {
        int Loopcount = 0;
        for (;;)
        {
            Loopcount++;
            time_t current_time = std::time(nullptr);
            if (std::difftime(current_time, last_tick_) > 20)
            {
                last_tick_ = current_time;
                std::ostringstream oss;
                oss << std::this_thread::get_id();
                SPDLOG_INFO(
                    " connection total :{} listening: {} connected:{} connecting:{} living:{} thread:{}",
                    sessionId2contexts_.size(),
                    listening_num_,
                    connected_num_,
                    init_connect_num_,
                    LivingCount(),
                    oss.str());
            }
            try
            {
                struct epoll_event ev_list[kMaxEpollEventNum];
                int eventn = epoll_wait(epoll_fd_, ev_list, kMaxEpollEventNum, -1); // 阻塞直到有事件到达
                if (!IsRunning())
                {
                    SPDLOG_INFO("epoll break by signal and not running, exit loop");
                    return 0;
                }
                if (eventn < 0)
                {
                    if (errno == EINTR)
                    {
                        // 信号中断
                        SPDLOG_INFO("epoll break by signal ");
                        running_ = false;
                        return 0;
                    }
                    SPDLOG_INFO("epoll wait err:{}", std::strerror(errno));
                    break;
                }
                if (eventn == 0)
                {
                    SPDLOG_INFO("epoll nothing to do ");
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
                        // if (ctx->IsConnecting())
                        // {
                        //     DoConn(ev, ctx);
                        // }
                        // else
                        // {
                        DoSend(ev, ctx);
                        // }
                    }
                    else if (ev.events & EPOLLERR)
                    {
                        SPDLOG_WARN("epoll err {}", ctx->GetFd());
                        CloseFdAndDelCtx(ctx->GetFd());
                    }
                    else if (ev.events & EPOLLHUP)
                    {
                        SPDLOG_WARN("{} epoll wait found remote closed sock {}", i, ctx->GetFd());
                        CloseFdAndDelCtx(ctx->GetFd());
                    }
                    else
                    {
                        SPDLOG_WARN("other event ");
                        CloseFdAndDelCtx(ctx->GetFd());
                    }
                }
            }
            catch (std::exception e)
            {
                SPDLOG_ERROR("epoll loop err {}", e.what());
                break;
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
                    CloseFdAndDelCtx(currentFd);
                    break;
                }
                if (errno == ENFILE)
                {
                    total_accept_failed_num_++;
                    SPDLOG_INFO("out of fd success {} failed {}", total_accept_num_, total_accept_failed_num_);
                    CloseFdAndDelCtx(currentFd);
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
                CloseFdAndDelCtx(currentFd);
                break;
            }
            else
            {
                struct epoll_event ev;
                EpollEventContext *newCtx = new EpollEventContext(newFd, false);
                ev.data.ptr = newCtx;
                ev.events = flag;
                newCtx->SetNoblocking();
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, newFd, &ev) != 0)
                {
                    SPDLOG_INFO("accept modify new fd ");
                    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, newFd, &ev);
                }
                SPDLOG_INFO("accept and create new Ctx {} ", newCtx->GetSessionId());
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
        const int current_fd = ctx->GetFd();
        const int sessionId = ctx->GetSessionId();
        ctx->SetNoblocking();
        while (1)
        {
            int read_bytes = recv(current_fd, ctx->read_buffer_.data() + ctx->read_offset_, kMaxEpollReadSize, 0);
            if (read_bytes == 0)
            {
                // 断开连接
                epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, current_fd, nullptr);
                EpollManager::DelContext(sessionId);
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
                    CloseFdAndDelCtx(sessionId);
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
                    CloseFdAndDelCtx(sessionId);
                    return;
                }
                if (errno == ENOTCONN)
                {
                    // 已经关闭的连接未从epoll移除可能出现
                    SPDLOG_WARN("old closed epoll fd not del {}", current_fd);
                    CloseFdAndDelCtx(sessionId);
                    return;
                }
                SPDLOG_WARN("unknow err for epoll fd recv {} err:{} msg: {} offset: {}", current_fd, strerror(errno), ctx->read_buffer_.data(), ctx->read_offset_);
                CloseFdAndDelCtx(sessionId);
                return;
            }
            ctx->ReadBytes(read_bytes);
        }
    }

    // 非阻塞模式connect完成创建连接时出发out事件的处理逻辑
    void EpollManager::DoConn(epoll_event &ev, EpollEventContext *ctx)
    {
        int currentFd = ctx->GetFd();
        const int sessionId = ctx->GetSessionId();
        int errcode;
        socklen_t len = sizeof(errcode);
        // SPDLOG_INFO("do connect {}", currentFd);
        if (getsockopt(currentFd, SOL_SOCKET, SO_ERROR, &errcode, &len) == -1)
        {
            SPDLOG_INFO("connect failed {} err {}", currentFd, strerror(errno));
            CloseFdAndDelCtx(sessionId);
            return;
        }
        if (errcode != 0)
        {
            SPDLOG_ERROR("connecting failed {} {}", currentFd, strerror(errcode));
            CloseFdAndDelCtx(sessionId);
            return;
        }
        ev.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, currentFd, &ev) == -1)
        {
            SPDLOG_ERROR("epoll mod connect fd failed ");
            return;
        }
        // SPDLOG_INFO("epoll connect finished fd {}", currentFd);
        ChangeCtxState(ctx, CONNECTED);
    }

    void EpollManager::DoSend(epoll_event &ev, EpollEventContext *ctx)
    {
        const int current_fd = ctx->GetFd();
        const int sessionId = ctx->GetSessionId();
        ctx->SetNoblocking();
        while (1)
        {
            if (ctx->send_q_.size() == 0)
            {
                ev.events &= ~EPOLLOUT;
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, current_fd, &ev) < 0)
                {
                    SPDLOG_INFO(" modify fd failed {}", strerror(errno));
                    CloseFdAndDelCtx(sessionId);
                    return;
                }
                ctx->ClearSendQ();
                return;
            }
            auto msg = ctx->send_q_.front();
            SPDLOG_INFO("do send msg:{}", std::string(msg.data()));
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
                    CloseFdAndDelCtx(sessionId);
                    return;
                }
                if (errno == ENOTSOCK)
                {
                    SPDLOG_INFO(" send target not a fd {}", current_fd);
                    CloseFdAndDelCtx(sessionId);
                    return;
                }
                if (errno == EINVAL)
                {
                    SPDLOG_INFO(" some args is wrong when send to fd {}", current_fd);
                    CloseFdAndDelCtx(sessionId);
                    return;
                }
                if (errno == ECONNRESET)
                {
                    SPDLOG_INFO(" remote reset when send to fd {}", current_fd);
                    CloseFdAndDelCtx(sessionId);
                    return;
                }
                SPDLOG_INFO(" unexpected err when send to fd {} err {}", current_fd, strerror(errno));
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
            if (!IsRunning())
            {
                return;
            }
            int n = read(pipe_fd, &cmd, sizeof(cmd));
            if (n == 0)
            {
                SPDLOG_INFO("reader pipe closed {}", pipe_fd);
                close(pipe_fd);
                // todo if or not close entire epoll manager
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
            case CommandType::CMD_INIT_LISTEN:
            {
                SPDLOG_INFO("{} init listen {}:{}", cmd.sessionId, cmd.ipaddr, cmd.port);
                int ret = InitListen(cmd);
                if (ret != 0)
                {
                    SPDLOG_ERROR("{} {}:{} init listen failed {}", cmd.sessionId, cmd.ipaddr, cmd.port, ret);
                }
                break;
            }

            case CommandType::CMD_INIT_CONNECT:
            {
                SPDLOG_INFO("init connect {} {}:{}", cmd.sessionId, cmd.ipaddr, cmd.port);
                int ret = InitConnect(cmd);
                if (ret != 0)
                {
                    SPDLOG_ERROR("{} {}:{} init connect failed {}", cmd.sessionId, cmd.ipaddr, cmd.port, ret);
                }
                break;
            }

            case CommandType::CMD_CLOSE:
            {
                SPDLOG_INFO("stop listen {}:{}", cmd.ipaddr, cmd.port);
                int ret = CloseSocket(cmd);
                if (ret != 0)
                {
                    SPDLOG_ERROR("{} close failed {}", cmd.sessionId, ret);
                }
                break;
            }

            case CommandType::CMD_START_LISTEN:
            {
                SPDLOG_INFO("start listen {}", cmd.sessionId);
                int ret = StartListen(cmd);
                if (ret != 0)
                {
                    SPDLOG_ERROR("{} start listen failed {}", cmd.sessionId, ret);
                }
                break;
            }

            case CommandType::CMD_START_CONNECT:
            {
                SPDLOG_INFO("connect {} {}:{}", cmd.sessionId, cmd.ipaddr, cmd.port);
                int ret = StartConnect(cmd);
                if (ret != 0)
                {
                    SPDLOG_ERROR("{} start connect failed {}", cmd.sessionId, ret);
                }
                break;
            }

            case CommandType::CMD_EXIT:
            {
                SPDLOG_INFO("exit");
                Stop();
                break;
            }

            case CommandType::CMD_SEND_MSG:
            {
                SPDLOG_INFO("send msg {}", cmd.sessionId);
                int ret = SendMsg(cmd);
                if(ret != 0){
                    SPDLOG_ERROR("{} send msg failed {}", cmd.sessionId, ret);
                }
                break;
            }

            default:
            {
                SPDLOG_INFO("unknow cmd:{}", (int)cmd.cmd);
                break;
            }
            }
        }
    }

    void EpollManager::CloseFdAndDelCtx(const int sessionId)
    {
        EpollEventContext *pCtx = GetContext(sessionId);
        const int fd = pCtx->GetFd();
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
        SysCloseFd(fd);
        DelContext(sessionId);
    }

    int EpollManager::SendMsg(const ControlCommand cmd)
    {
        SPDLOG_INFO("{} handling SendMsg cmd {}", cmd.sessionId, cmd.msg);
        EpollEventContext *ctx = GetContext(cmd.sessionId);
        if (ctx == nullptr)
        {
            return 1;
        }
        const int fd = ctx->GetFd();
        ctx->PushMsgQ(cmd.msg, cmd.msgSize);
        struct epoll_event ev;
        ev.data.ptr = ctx;
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
        epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
        return 0;
    }

    void EpollManager::ChangeCtxState(EpollEventContext*ctx, SocketState newState){
        SocketState oldState = ctx->GetState();
        switch (oldState)
        {
        case LISTEN_WAIT_OPEN:
            init_listen_num_--;
            break;
        case CONNECT_WAIT_OPEN:
            init_connect_num_--;
            break;
        case LISTENING:
            listening_num_--;
            break;
        case CONNECTED:
            connected_num_--;
            break;
        default:
            break;
        }

        switch (newState)
        {
        case LISTEN_WAIT_OPEN:
            init_listen_num_++;
            break;
        case CONNECT_WAIT_OPEN:
            init_connect_num_++;
            break;
        case LISTENING:
            listening_num_++;
            break;
        case CONNECTED:
            connected_num_++;
            break;
        case CLOSED:
            close_sock_num_++;
            break;
        default:
            break;
        }

        ctx->SetState(newState);
    }

    void EpollManager::Stop()
    {
        if(pMgr == nullptr){
            SPDLOG_INFO("epoll manager already stoped");
            return;
        }
        if (!pMgr->IsRunning())
        {
            SPDLOG_INFO("epoll manager already stoped");
            return;
        }
        std::lock_guard<std::mutex> lock(pMgr->isRunningMtx);
        pMgr->running_ = false;
        while (pMgr != nullptr)
        {
            SPDLOG_INFO("wait epoll manager stoped");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        SPDLOG_INFO("epoll manager stoped");
    }

}
