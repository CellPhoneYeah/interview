#ifndef EPOLL_MANAGER_H
#define EPOLL_MANAGER_H
#include <string.h>

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <atomic>

#include "ellnet/epoll_event_context.h"
#include "ellnet/epoll_net.h"

namespace ellnet
{
    class EpollManager
    {
    public:
        explicit EpollManager(const int pipe_in_fd = -1);
        ~EpollManager();
        static const int kMaxEpollEventNum = 1024;
        static const int kMaxEpollReadSize = 1024;
        static void AddContext(EpollEventContext *);
        static void DelContext(const int fd);
        static EpollEventContext *GetContext(const int fd);

        int StartListen(std::string addr, const int port);
        int StopListen(std::string addr, const int port);
        int ConnectTo(std::string addr, const int port);
        int OpenConnection(const int id);
        int Loop();
        void DoAccept(struct epoll_event &ev, EpollEventContext *ctx);
        void DoRead(EpollEventContext *ctx);
        void DoConn(epoll_event &ev, EpollEventContext *ctx);
        void DoSend(epoll_event &ev, EpollEventContext *ctx);
        void DoReadPipe(EpollEventContext *ctx);
        void CloseFd(const int fd);
        bool SendMsg(const int fd, const char *msg, const int size);
        static int LivingCount();
        bool NewPipe(const int pipe_fd_out);
        void Run();
        bool IsRunning() { return running_; }
        void Stop() { running_ = false; }
        static void StartManager(const int pipe_fd);
        static int ListeningFd(std::string &addr, const int port);
        

    private:
        static std::unordered_map<int, EpollEventContext *> id2contexts_;
        static std::unordered_map<int, EpollEventContext *> fd2contexts_;

        int Init(const int pipe_in_fd);
        void SysCloseFd(const int fd);
        int SysNewFd();

        int epoll_fd_;
        int total_accept_num_;
        int total_accept_failed_num_;
        int connected_num_;
        int connecting_num_;
        int listening_num_;
        int new_sock_num_;
        int close_sock_num_;
        struct epoll_event event_list_[kMaxEpollEventNum];
        std::unordered_set<int> listening_fds_;
        time_t last_tick_;
        int pipe_fd_;
        std::atomic<bool> running_;
    };
}

#endif