#ifndef EPOLL_NET_H
#define EPOLL_NET_H
#include <string.h>
#include <string>
#include <atomic>

class EpollManager;

enum CommandType{
    CMD_START_LISTEN,
    CMD_STOP_LISTEN,
    CMD_CONNECT_TO,
    CMD_DISCONNECT,
    CMD_EXIT
};

enum ManagerState{
    STOP,
    RUNNING
};

struct ControlCommand{
    CommandType cmd;
    int fd;
    int port;
    char ipaddr[46]; // fix ipv4 ipv6
};

class EpollNet{
public:
    static EpollNet* getInstance();
    int sendMsg(std::string msg, int fd);
    int listenOn(std::string ipaddr, int port);
    int connectTo(std::string ipaddr, int port);
    int stopListen(std::string ipaddr, int port);
    EpollManager* getManager(){return epollManager;}
    static void setManagerState(bool newState){EpollNet::managerState = newState;}
private:
    static void startManager(int pipe_fd_out);

    EpollNet();
    ~EpollNet();
    EpollNet(const EpollNet&) = delete;
    EpollNet& operator=(const EpollNet&) = delete;
    EpollManager* epollManager;
    int pipe_fd[2];

    void sendCmd(ControlCommand cmd);

    static std::atomic<bool> running;
    static std::atomic<bool> managerState;
    static EpollNet* instance;
};
#endif