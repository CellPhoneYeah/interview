#pragma once
#include <unistd.h>
#include <sstream>
#include <arpa/inet.h>
#include <sys/event.h>
#include <queue>
#include <vector>
#include <unordered_map>
#include "proto.h"

#define RING_BUFFER_SIZE 10240
#define READ_BUFFER_SIZE 1024
#define WRITE_BUFFER_SIZE 64
#define WRITE_QUEUE_SIZE 1024
#define IP_ADDRESS_LEN 40
#define INVALID_SOCK (~0)

enum SOCKET_TYPE{
    SOCKET_TYPE_SOCK,
    SOCKET_TYPE_PIPE
};

enum EVENTLIST{
    READ_EVENT,
    WRITE_EVENT
};

struct EventContext;

class EllConn{
private:
    int readSock(char* buffer, int size);
protected:
    static std::unordered_map<int, EllConn*> clientMap;
    int _sockfd;
    bool _isListenFd;
    int _kq;
    char _ring_buffer[RING_BUFFER_SIZE];
    char _read_buffer[READ_BUFFER_SIZE];
    int _read_pos;
    int _last_pos;
    int _size;
    DataHeader* _dh;
    EventContext* _ec;
    char _bind_ipaddr[IP_ADDRESS_LEN];
    int _bind_port;
    int _sock_type;
    int _ev_list[4]; // [read write]
public:
    static EllConn* getClient(int sockFd);
    static void addClient(int sockFd, EllConn* ec);
    static void delClient(int sockFd);

    EllConn(int kq, int sockfd, int sockType);
    EllConn(int kq, int sockfd);
    EllConn(int kq);
    EllConn();

    virtual ~EllConn() = default;
    void close();
    int getKQ();

    int getSock();
    bool isClosed();
    bool isPipe(){return _sock_type == SOCKET_TYPE_PIPE;};

    bool checkSock();
    bool bindKQ(int kq);
    bool isBindedKQ();

    bool canRegisterEv();
    int registerReadEv(void* udata = nullptr);
    int unregisterReadEv();
    int registerAcceptEv(void* udata = nullptr);
    int registerWriteEv(void* udata = nullptr);
    int unregisterWriteEv();

    int connect(const char* ipaddr, int port);
    int writeSingleData();
    void clearWriteBuffer();

    int sendData(char* data, int size);

    bool bindAddr(std::string ipAddr, int port);

    bool listen();
    bool isListening(){ return _isListenFd; }
    int readData(const struct kevent &ev);
    int loopListenSock(struct kevent *events, int size);

    virtual bool acceptSock(int clientfd, EllConn* parentEC) = 0;
    virtual int handleOneProto() = 0;
    virtual void onCloseFd() = 0;
    const char* getBindIp()const {return _bind_ipaddr;}
    const int getBindPort() const {return _bind_port;}
    friend std::ostream& operator<<(std::ostream& os, const EllConn &eec);
};

struct EventContext{
    int fd = 0;
    std::queue<std::vector<char> > writeQ;
    int offsetPos = 0;
    int socket_type = SOCKET_TYPE_SOCK;
    int targetfd;
    EllConn* ec;
};