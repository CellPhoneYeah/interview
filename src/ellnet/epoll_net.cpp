#include "epoll_net.h"

#include <unistd.h>

#include <iostream>
#include <thread>

#include "slog.h"

#include "ellnet/epoll_connect_handler.h"
#include "ellnet/epoll_manager.h"

namespace ellnet
{
    std::atomic<bool> EpollNet::manager_state_{false};
    EpollNet *EpollNet::instance_ = new EpollNet();

    EpollNet::EpollNet()
    {
        SPDLOG_INFO("epollnet init");
        running_ = false;
        manager_state_ = false;
        pipe(pipe_fd_);
        StartManager(pipe_fd_[0]);
        SPDLOG_INFO("epollnet init done");
    }

    EpollNet *EpollNet::GetInstance()
    {
        return EpollNet::instance_;
    }

    int EpollNet::SendMsg(std::string msg, const int sessionId)
    {
        if(EpollManager::GetContext(sessionId) == nullptr){
            SPDLOG_WARN("send msg session context not found {}", sessionId);
            return -1;
        }

        ControlCommand cmd;
        cmd.cmd = CMD_SEND_MSG;
        cmd.sessionId = sessionId;
        cmd.msgSize = msg.size();
        cmd.msg = new char[cmd.msgSize + 1];
        strcpy(cmd.msg, msg.c_str());
        SPDLOG_INFO("send msg cmd sent {} -> {}", sessionId, msg);
        SendCmd(cmd);
        SPDLOG_INFO("send msg cmd sent {} -> {}", sessionId, msg);
        return 0;
    }

    int EpollNet::ListenOn(std::string ipaddr, const int port)
    {
        if(ipaddr.empty()){
            SPDLOG_ERROR("using empty addr to init listen {}", ipaddr);
            return -1;
        }
        if(port <= 0){
            SPDLOG_ERROR("using illegal port init listen {}", port);
            return -2;
        }
        int sessionId = EpollManager::NewFdAndBindContext();
        if(sessionId < 0){
            SPDLOG_ERROR("init listen session failed {}", sessionId);
            return -3;
        }
        ControlCommand cmd;
        cmd.cmd = CMD_INIT_LISTEN;
        cmd.port = port;
        cmd.sessionId = sessionId;
        strcpy(cmd.ipaddr, ipaddr.c_str());
        SendCmd(cmd);
        return sessionId;
    }

    void EpollNet::StartListen(const int sessionId)
    {
        ControlCommand cmd;
        cmd.cmd = CMD_START_LISTEN;
        if(EpollManager::GetContext(sessionId) == nullptr){
            SPDLOG_WARN("start listen session context not found {}", sessionId);
            return;
        }
        cmd.sessionId = sessionId;
        SendCmd(cmd);
    }

    void EpollNet::CloseSocket(int sessionId)
    {
        ControlCommand cmd;
        cmd.cmd = CMD_CLOSE;
        cmd.sessionId = sessionId;
        if(EpollManager::GetContext(sessionId) == nullptr){
            SPDLOG_WARN("stop listen session context not found {}", sessionId);
            return;
        }
        SendCmd(cmd);
    }

    int EpollNet::ConnectTo(std::string ipaddr, const int port)
    {
        if(ipaddr.empty()){
            SPDLOG_ERROR("using empty addr to init connect {}", ipaddr);
            return -1;
        }
        if(port <= 0){
            SPDLOG_ERROR("using illegal port init connect {}", port);
            return -2;
        }
        int sessionId = EpollManager::NewFdAndBindContext();
        if(sessionId < 0){
            SPDLOG_ERROR("init connect session failed {}", sessionId);
            return -3;
        }
        ControlCommand cmd;
        cmd.sessionId = sessionId;
        cmd.cmd = CMD_INIT_CONNECT;
        strcpy(cmd.ipaddr, ipaddr.c_str());
        cmd.port = port;
        SendCmd(cmd);
        return sessionId;
    }

    void EpollNet::StartConnect(const int sessionId)
    {
        ControlCommand cmd;
        cmd.cmd = CMD_START_CONNECT;
        cmd.sessionId = sessionId;
        SPDLOG_INFO("start connect session {}", sessionId);
        SendCmd(cmd);
    }

    void EpollNet::JoinThread()
    {
        if(this->thread_.joinable()){
            this->thread_.join();
        }
    }

    void EpollNet::StartManager(const int pipe_fd_out)
    {
        thread_ = std::thread(EpollManager::StartManager, pipe_fd_out);
    }

    EpollNet::~EpollNet()
    {
        ::close(pipe_fd_[1]);
        delete (epoll_manager_);
        SPDLOG_INFO("release EpollNet instance");
    }

    void EpollNet::SendCmd(const ControlCommand cmd)
    {
        try
        {
            SPDLOG_INFO("send cmd:{} fd:{} sessionId:{} ipaddr:{} port:{} msgSize:{} msg:{}", 
                (int)cmd.cmd,
                cmd.fd,
                cmd.sessionId,
                cmd.ipaddr,
                cmd.port,
                cmd.msgSize,
                cmd.msg ? cmd.msg : "null");
            ssize_t written = write(pipe_fd_[1], &cmd, sizeof(ControlCommand));
            if (written == -1)
            {
                SPDLOG_WARN("send cmd failed pipe:{} cmd:{} fd:{} ipaddr:{} port:{} err:{}", pipe_fd_[1], (int)cmd.cmd, cmd.fd, cmd.ipaddr, cmd.port, strerror(errno));
            }
        }
        catch (const std::exception &e)
        {
            SPDLOG_ERROR("send cmd failed {}", e.what());
        }
    }

    void EpollNet::close()
    {
        SPDLOG_INFO("close epoll net");
        EpollManager::Stop();
        SPDLOG_INFO("epoll net closed");
    }

}
