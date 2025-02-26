#include <string>
#include <type_traits>

#define LOOP_EVENT_NUM 64
#define SYS_READ_BUFFER_SIZE 1024
class EllConnBase;

class EllBaseServer{
private:
    static timespec ts;
    std::unordered_map<int, EllConnBase*> _connMap;
    int _evq;
    char sys_buffer[SYS_READ_BUFFER_SIZE];
    std::string listened_addr;
    int listened_port;
    EllConnBase* listened_ecb;
public:
    EllBaseServer();
    EllBaseServer(int evq);
    ~EllBaseServer();
    void addConn(EllConnBase *ecb);
    void delConn(int connFd);
    EllConnBase* getConn(int connFd);
    int loopEVQ();
    int handleReadEv(const struct kevent &ev);
    int handleWriteEv(const struct kevent &ev);
    int handleAcceptEv(const struct kevent &ev);
    void newConnection(const int newfd);
    int startListen(std::string addr, int port);
    int connectTo(std::string addr, int port);
    EllConnBase* newPipe(int pipe_fd);
    int getEVQ() const {return _evq;}
    std::string getListenedAddr()const {return listened_addr;}
    int getListenedPort()const {return listened_port;}

    int sysHandleReadEv(const struct kevent&ev);
    friend std::ostream& operator<<(std::ostream& os, const EllBaseServer& ebs);
};