#if defined(__linux__)
#pragma once
#include <unistd.h>
#include <sstream>
#include <arpa/inet.h>
#include <queue>
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include "CustomProto.h"
#include "EventContext.h"
#include "EllNetConfig.h"

#define EllNetConfig ENC

enum EVENTLIST{
    READ_EVENT,
    WRITE_EVENT
};


class EPConn{
private:
    int readSock(char* buffer, int size);
protected:
    static std::unordered_map<int, EPConn*> clientMap;
    int _sockfd;
    bool _isListenFd;
    int _kq;
    char _ring_buffer[ENC::RING_BUFFER_SIZE];
    char _read_buffer[ENC::READ_BUFFER_SIZE];
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
    static EPConn* getClient(int sockFd);
    static void addClient(int sockFd, EPConn* ec);
    static void delClient(int sockFd);

    EPConn(int kq, int sockfd, int sockType);
    EPConn(int kq, int sockfd);
    EPConn(int kq);
    EPConn();

    virtual ~EPConn() = default;
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
#endif