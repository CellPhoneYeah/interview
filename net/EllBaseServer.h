#include "EasyEllConn.h"
#include <string>
#include <type_traits>

#define LOOP_EVENT_NUM 64
#define SYS_READ_BUFFER_SIZE 1024

class EllBaseServer{
private:
    static timespec ts;
    std::unordered_map<int, EllConn*> _connMap;
    struct kevent event_list[LOOP_EVENT_NUM];
    int _kq;
    char sys_buffer[SYS_READ_BUFFER_SIZE];
    std::string listened_addr;
    int listened_port;
    EasyEllConn* listened_eec;
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
    EllConn* newPipe(int pipe_fd);
    int getEVQ() const {return _kq;}
    std::string getListenedAddr()const {return listened_addr;}
    int getListenedPort()const {return listened_port;}

    int sysHandleReadEv(const struct kevent&ev);
    friend std::ostream& operator<<(std::ostream& os, const EllBaseServer& ebs);
};