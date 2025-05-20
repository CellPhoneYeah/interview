#ifndef EPOLL_NET_H
#define EPOLL_NET_H
#include <string>
#include <atomic>
#include <string.h>
#include <thread>

#include "epoll_connect_handler.h"
#include "epoll_manager.h"
#include "epoll_net_header.h"
namespace ellnet
{
    class EpollManager;
    class EpollNet
    {
    public:
        static EpollNet *GetInstance();
        int SendMsg(std::string msg, const int sessionId);
        int ListenOn(std::string ipaddr, const int port);
        void StartListen(const int sessionId);
        int ConnectTo(std::string ipaddr, const int port);
        void StartConnect(const int sessionId);
        void CloseSocket(int sessionId);
        EpollManager *GetManager() { return epoll_manager_; }
        static void SetManagerState(const bool newState) { EpollNet::manager_state_ = newState; }
        void SetConnectHandler(EpollConnectHandler *handler) { this->connhandler_ = handler; }
        void JoinThread();

    private:
        void StartManager(const int pipe_fd_out);

        EpollNet();
        ~EpollNet();
        explicit EpollNet(const EpollNet &) = delete;
        EpollNet &operator=(const EpollNet &) = delete;
        EpollManager *epoll_manager_;
        EpollConnectHandler *connhandler_;

        void SendCmd(ControlCommand cmd);

        int pipe_fd_[2];
        std::atomic<bool> running_;
        std::thread thread_;
        static std::atomic<bool> manager_state_;
        static EpollNet *instance_;
    };
}

#endif