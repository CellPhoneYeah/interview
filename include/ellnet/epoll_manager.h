#ifndef EPOLL_MANAGER_H
#define EPOLL_MANAGER_H
#include <string.h>

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <atomic>
#include <mutex>
#include <sys/epoll.h>

#include "epoll_event_context.h"
#include "epoll_net_header.h"

namespace ellnet
{
    class EpollManager
    {
    public:
        explicit EpollManager(const int pipe_in_fd = -1);
        ~EpollManager();
        static const int kMaxEpollEventNum = 64;
        static const int kMaxEpollReadSize = 64;
        static EpollManager* pMgr;
        static void AddContext(EpollEventContext *);
        static void DelContext(const int sessionId);
        static int NewFdAndBindContext();
        static EpollEventContext *GetContext(const int sessionId);
        int Loop();
        void DoAccept(struct epoll_event &ev, EpollEventContext *ctx);
        void DoRead(EpollEventContext *ctx);
        void DoConn(epoll_event &ev, EpollEventContext *ctx);
        void DoSend(epoll_event &ev, EpollEventContext *ctx);
        void DoReadPipe(EpollEventContext *ctx);
        void CloseFdAndDelCtx(const int fd);
        static int LivingCount();
        bool NewPipe(const int pipe_fd_out);
        void Run();
        bool IsRunning() { return running_; }
        static void Stop();
        static void StartManager(const int pipe_fd);
        static int ListeningFd(std::string &addr, const int port);

        static void OnDelContext(EpollEventContext* ctx);
        static void OnAddContext(EpollEventContext* ctx);
        

    private:
        int OpenConnection(const int id);

        int InitListen(const ControlCommand cmd);
        int StartListen(const ControlCommand cmd);
        int InitConnect(const ControlCommand cmd);
        int StartConnect(const ControlCommand cmd);
        int CloseSocket(const ControlCommand cmd);
        int SendMsg(const ControlCommand cmd);

        static std::mutex contextMtx;
        static std::mutex isRunningMtx;
        static std::unordered_map<int, EpollEventContext *> sessionId2contexts_;
        static std::unordered_map<int, EpollEventContext *> fd2contexts_;

        int Init(const int pipe_in_fd);
        static void SysCloseFd(const int fd);
        static int SysNewFd();
        static void ChangeCtxState(EpollEventContext*ctx, SocketState newState);

        int epoll_fd_;
        static int total_accept_num_;
        static int total_accept_failed_num_;
        static int connected_num_;
        static int init_connect_num_;
        static int listening_num_;
        static int init_listen_num_;
        static int new_sock_num_;
        static int close_sock_num_;
        struct epoll_event event_list_[kMaxEpollEventNum];
        std::unordered_set<int> listening_fds_;
        time_t last_tick_;
        int pipe_fd_;
        std::atomic<bool> running_;
    };
}

#endif