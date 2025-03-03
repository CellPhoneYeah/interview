#include <unordered_map>
#include <unordered_set>
#include "EpollEventContext.h"
#include <string.h>
#include <string>

class EpollManager{
public:
    static const int MAX_EPOLL_EVENT_NUM = 1024;
    static const int MAX_EPOLL_READ_SIZE = 1024;
    static void addContext(EpollEventContext*);
    static void delContext(int fd);
    static EpollEventContext* getContext(int fd);

    EpollManager();
    ~EpollManager();
    int start_listen(std::string addr, int port);
    int connect_to(std::string addr, int port);
    int loop();
    void do_accept(struct epoll_event &ev, EpollEventContext* ctx);
    void do_read(EpollEventContext *ctx);
    void do_conn(epoll_event &ev, EpollEventContext *ctx);
    void do_send(epoll_event &ev, EpollEventContext *ctx);
    void close_fd(int fd);
    bool sendMsg(int fd, const char* msg, int size);
private:
    static std::unordered_map<int, EpollEventContext*> contexts;
    int init();
    int epoll_fd;
    struct epoll_event event_list[MAX_EPOLL_EVENT_NUM];
    std::unordered_set<int> listening_fds;
    std::unordered_set<int> connecting_fds;
    time_t last_tick;
};