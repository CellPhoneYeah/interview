#ifndef EPOLL_NET_H
#define EPOLL_NET_H
#include <string>
#include <atomic>
#include <string.h>

#include "ellnet/epoll_connect_handler.h"
#include "ellnet/epoll_manager.h"
namespace ellnet
{
    enum CommandType
    {
        CMD_START_LISTEN,
        CMD_STOP_LISTEN,
        CMD_CONNECT_TO,
        CMD_DISCONNECT,
        CMD_OPEN_CONNECT,
        CMD_EXIT
    };

    enum ManagerState
    {
        STOP,
        RUNNING
    };

    struct ControlCommand
    {
        CommandType cmd;
        int fd;
        int port;
        char ipaddr[46]; // fix ipv4 ipv6
    };

    class EpollManager;

    class EpollNet
    {
    public:
        static EpollNet *GetInstance();
        int SendMsg(std::string msg, const int fd);
        int ListenOn(std::string ipaddr, const int port);
        int ConnectTo(std::string ipaddr, const int port);
        int StopListen(std::string ipaddr, const int port);
        EpollManager *GetManager() { return epoll_manager_; }
        static void SetManagerState(const bool newState) { EpollNet::manager_state_ = newState; }
        void SetConnectHandler(EpollConnectHandler *handler) { this->connhandler_ = handler; }

    private:
        static void StartManager(const int pipe_fd_out);

        EpollNet();
        ~EpollNet();
        explicit EpollNet(const EpollNet &) = delete;
        EpollNet &operator=(const EpollNet &) = delete;
        EpollManager *epoll_manager_;
        EpollConnectHandler *connhandler_;

        void SendCmd(ControlCommand cmd);

        int pipe_fd_[2];
        std::atomic<bool> running_;
        static std::atomic<bool> manager_state_;
        static EpollNet *instance_;
    };
}

#endif