#include "EasyEllConn.h"
#include <string>

#define LOOP_EVENT_NUM 64
#define SYS_READ_BUFFER_SIZE 1024

class EllBaseServer{
private:
    static timespec ts;
    std::unordered_map<int, EllConn*> _connMap;
    struct kevent event_list[LOOP_EVENT_NUM];
    int _kq;
    char sys_buffer[SYS_READ_BUFFER_SIZE];
public:
    EllBaseServer();
    EllBaseServer(int kq);
    ~EllBaseServer();
    void addConn(EllConn *ec);
    void delConn(int connFd);
    EllConn* getConn(int connFd);
    int loopKQ();
    int handleReadEv(const struct kevent &ev);
    int handleWriteEv(const struct kevent &ev);
    int handleAcceptEv(const struct kevent &ev);
    void newConnection(const int newfd);
    int startListen(std::string addr, int port);
    int connectTo(std::string addr, int port);

    int sysHandleReadEv(const struct kevent&ev);
};