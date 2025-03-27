#ifndef EPOLL_MANAGER_H
#define EPOLL_MANAGER_H
#include <string.h>

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <atomic>

#include "src/net/include/EpollEventContext.h"
#include "src/net/include/EpollNet.h"

class EpollManager{
public:
    static const int MAX_EPOLL_EVENT_NUM = 1024;
    static const int MAX_EPOLL_READ_SIZE = 1024;
    static void addContext(EpollEventContext*);
    static void delContext(int fd);
    static EpollEventContext* getContext(int fd);
    
    int start_listen(std::string addr, int port);
    int stop_listen(std::string addr, int port);
    int connect_to(std::string addr, int port);
    int loop();
    void do_accept(struct epoll_event &ev, EpollEventContext* ctx);
    void do_read(EpollEventContext *ctx);
    void do_conn(epoll_event &ev, EpollEventContext *ctx);
    void do_send(epoll_event &ev, EpollEventContext *ctx);
    void do_read_pipe(EpollEventContext *ctx);
    void close_fd(int fd);
    bool sendMsg(int fd, const char* msg, int size);
    static int livingCount();
    bool newPipe(int pipe_fd_out);
    void run();
    bool isRunning(){return running;}
    void stop(){running = false;}
    static void startManager(int pipe_fd);
    static int listeningFd(std::string &addr, int port);
    EpollManager(int pipe_in_fd = -1);
    ~EpollManager();
private:
    static std::unordered_map<int, EpollEventContext*> contexts;

    // EpollManager(const EpollManager&) = delete;
    // EpollManager& operator=(const EpollManager&) = delete;
    
    int init(int pipe_in_fd);
    void sys_close_fd(int fd);
    int sys_new_fd();
    int epoll_fd;
    int total_accept_num;
    int total_accept_failed_num;
    int connected_num;
    int connecting_num;
    int listening_num;

    int new_sock_num;
    int close_sock_num;
    struct epoll_event event_list[MAX_EPOLL_EVENT_NUM];
    std::unordered_set<int> listening_fds;
    time_t last_tick;
    int pipe_fd;
    std::atomic<bool> running;
};
#endif