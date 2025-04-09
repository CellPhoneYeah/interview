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
    EpollNet *EpollNet::instance_ = nullptr;

    EpollNet::EpollNet()
    {
        running_ = false;
        manager_state_ = false;
        pipe(pipe_fd_);
        StartManager(pipe_fd_[0]);
    }

    EpollNet *EpollNet::GetInstance()
    {
        if (EpollNet::instance_ == nullptr)
        {
            EpollNet::instance_ = new EpollNet();
        }
        return EpollNet::instance_;
    }

    int EpollNet::SendMsg(std::string msg, const int fd)
    {
        return false;
    }

    int EpollNet::ListenOn(std::string ipaddr, const int port)
    {
        ControlCommand cmd;
        cmd.cmd = CMD_START_LISTEN;
        cmd.port = port;
        strcpy(cmd.ipaddr, ipaddr.c_str());
        SendCmd(cmd);
        return 0;
    }

    int EpollNet::StopListen(std::string ipaddr, const int port)
    {
        ControlCommand cmd;
        cmd.cmd = CMD_STOP_LISTEN;
        strcpy(cmd.ipaddr, ipaddr.c_str());
        cmd.port = port;
        SendCmd(cmd);
        return 0;
    }

    int EpollNet::ConnectTo(std::string ipaddr, const int port)
    {
        // 需要考虑怎么同步返回连接套接字，或者直接提供上下文id绑定一个套接字
        ControlCommand cmd;
        cmd.cmd = CMD_CONNECT_TO;
        strcpy(cmd.ipaddr, ipaddr.c_str());
        cmd.port = port;
        SendCmd(cmd);
        return 0;
    }

    void EpollNet::StartManager(const int pipe_fd_out)
    {
        std::thread runEpollTh(EpollManager::StartManager, pipe_fd_out);
        runEpollTh.detach();
    }

    EpollNet::~EpollNet()
    {
        ::close(pipe_fd_[1]);
        delete (epoll_manager_);
        SPDLOG_INFO("release EpollNet instance");
    }

    void EpollNet::SendCmd(const ControlCommand cmd)
    {
        ssize_t written = write(pipe_fd_[1], &cmd, sizeof(cmd));
        if (written == -1)
        {
            SPDLOG_WARN("send cmd failed pipe:{} cmd:{} fd:{} ipaddr:{} port:{} err:{}", pipe_fd_[1], (int)cmd.cmd, cmd.fd, cmd.ipaddr, cmd.port, strerror(errno));
        }
    }

}
