#include "EpollNet.h"

#include <unistd.h>

#include <iostream>
#include <thread>

#include "slog.h"

#include "src/net/include/EpollConnectHandler.h"
#include "src/net/include/EpollManager.h"


std::atomic<bool> EpollNet::managerState{false};
std::atomic<bool> EpollNet::running{false};
EpollNet* EpollNet::instance = nullptr;

EpollNet::EpollNet(){
    pipe(pipe_fd);
    startManager(pipe_fd[0]);
}

EpollNet *EpollNet::getInstance()
{
    if(EpollNet::instance == nullptr){
        EpollNet::instance = new EpollNet();
    }
    return EpollNet::instance;
}

int EpollNet::sendMsg(std::string msg, int fd)
{
    return false;
}

int EpollNet::listenOn(std::string ipaddr, int port)
{
    ControlCommand cmd;
    cmd.cmd = CMD_START_LISTEN;
    cmd.port = port;
    strcpy(cmd.ipaddr, ipaddr.c_str());
    sendCmd(cmd);
    return 0;
}

int EpollNet::stopListen(std::string ipaddr, int port)
{
    ControlCommand cmd;
    cmd.cmd = CMD_STOP_LISTEN;
    strcpy(cmd.ipaddr, ipaddr.c_str());
    cmd.port = port;
    sendCmd(cmd);
    return 0;
}

int EpollNet::connectTo(std::string ipaddr, int port)
{
    ControlCommand cmd;
    cmd.cmd = CMD_CONNECT_TO;
    strcpy(cmd.ipaddr, ipaddr.c_str());
    cmd.port = port;
    sendCmd(cmd);
    return 0;
}

void EpollNet::startManager(int pipe_fd_out)
{
    std::thread runEpollTh(EpollManager::startManager, pipe_fd_out);
    runEpollTh.detach();
}

EpollNet::~EpollNet()
{
    ::close(pipe_fd[1]);
    delete(epollManager);
    SPDLOG_INFO("release EpollNet instance");
}

void EpollNet::sendCmd(ControlCommand cmd)
{
    ssize_t written = write(pipe_fd[1], &cmd, sizeof(cmd));
    if(written == -1){
        SPDLOG_WARN("send cmd failed pipe:{} cmd:{} fd:{} ipaddr:{} port:{} err:{}", pipe_fd[1], (int)cmd.cmd, cmd.fd, cmd.ipaddr, cmd.port, strerror(errno));
    }
}
