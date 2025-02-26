#if defined(__linux__)
#pragma once
#include <unistd.h>
#include <sstream>
#include <arpa/inet.h>
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include "CustomProto.h"
#include "EventContext.h"
#include "EllNetConfig.h"
#include "EllConn.h"

#define EllNetConfig ENC
class EPConn: public EllConn<epoll_event>{
private:
    int readSock(char* buffer, int size);
public:
    EPConn(const EllBaseServer* pEbs, int sockfd, int sockType):EllConn(pEbs, sockfd, sockType){}
    EPConn(const EllBaseServer* pEbs, int sockfd):EllConn(pEbs, sockfd){}
    EPConn(const EllBaseServer* pEbs):EllConn(pEbs){}
    EPConn():EllConn(){}

    ~EPConn() override;
    bool acceptSock(int clientfd, epoll_event* ev) override;
    int handleOneProto() override;
    void onCloseFd() override;;
    int doRegisterReadEv(void* data) override;
    int doUnregisterReadEv() override;
    int doRegisterWriteEv(void* data) override;
    int doUnregisterWriteEv() override;
    int getEventFd(epoll_event*event) override;
    int getEventFlag(epoll_event*event) override;
    int getEventFilter(epoll_event*event) override;
    void* getEventUdata(epoll_event*event) override;
    int loopEvent(epoll_event* events, int size) override;

    void* deleteReadEvent(epoll_event*event) override;
};
#endif