#include <sys/epoll.h>
#include "EPConn.h"
#include <iostream>
#include "ConnManager.h"

EPConn::~EPConn(){
    std::cout << "delete epconn" << std::endl;
}

int EPConn::getEventFd(epoll_event *ev){
    return ev->data.fd;
}

bool EPConn::acceptSock(int clientfd, epoll_event *ev){
    if (clientfd == _sockfd)
    {
        return true;
    }
    if (ConnManager::getClient(clientfd) == nullptr)
    {
        EPConn *eec = new EPConn(pEbs->getEVQ(), clientfd);
        eec->registerReadEv();
        EllConn::addClient(clientfd, eec);
        return true;
    }
    else
    {
        std::cout << " repeat sock data, check mem leek" << std::endl;
        return false;
    }
}
int EPConn::handleOneProto() override;
void EPConn::onCloseFd() override;
;
int EPConn::doRegisterReadEv(void *data) override;
int EPConn::doUnregisterReadEv() override;
int EPConn::doRegisterWriteEv(void *data) override;
int EPConn::doUnregisterWriteEv() override;

int EPConn::getEventFlag(epoll_event *event) override;
int EPConn::getEventFilter(epoll_event *event) override;
void *EPConn::getEventUdata(epoll_event *event) override;
int EPConn::loopEvent(epoll_event *events, int size) override;

void *EPConn::deleteReadEvent(epoll_event *event) override;