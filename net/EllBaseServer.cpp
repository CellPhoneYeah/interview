#include "EllBaseServer.h"
#include <sstream>
#include <iostream>
#include "EllNetConfig.h"
#include "EllConnBase.h"

struct timespec EllBaseServer::ts = {0, 0};

EllBaseServer::EllBaseServer()
{
    _evq = createEVQ();
    listened_addr.clear();
    listened_port = -1;
    listened_ecb = nullptr;
}

EllBaseServer::~EllBaseServer(){
    auto it = _connMap.begin();
    while(it != _connMap.end()){
        delete(it->second);
        it++;
    }
}

void EllBaseServer::addConn(EllConnBase *ecb)
{
    if (ecb == nullptr)
    {
        return;
    }
    if (ecb->getSock() == INVALID_SOCK)
    {
        return;
    }
    _connMap[ecb->getSock()] = ecb;
    ecb->bindEVQ(_evq);
    std::cout << "addConn" << ecb->getSock() << " :" << ecb << std::endl;
}
void EllBaseServer::delConn(int connFd)
{
    EllConnBase *ecb = _connMap[connFd];
    if (ecb == nullptr)
    {
        _connMap.erase(connFd);
        return;
    }
    std::cout << "delConn" << ecb->getSock() << " :" << ecb << std::endl;
    ecb->close();
    _connMap.erase(connFd);
    delete (ecb);
}

EllConnBase *EllBaseServer::getConn(int connFd)
{
    return _connMap[connFd];
}

int EllBaseServer::handleReadEv(const struct kevent &ev)
{
    int sockfd = ev.ident;
    EllConnBase *ecb = getConn(sockfd);
    if (ec == nullptr)
    {
        return 0;
    }
    if (ec->isListening())
    {
        return handleAcceptEv(ev);
    }
    if (ev.udata != nullptr)
    {
        EllConn *ec = (EllConn *)ev.udata;
        if (ec != nullptr)
        {
            int totalLen = ec->readData(ev);
            return totalLen;
        }
        else
        {
            return sysHandleReadEv(ev);
        }
    }
    return 0;
}

int EllBaseServer::sysHandleReadEv(const struct kevent &ev)
{
    int currentFd = ev.ident;
    int totalLen = 0;
    while (1)
    {
        int len = recv(currentFd, sys_buffer, SYS_READ_BUFFER_SIZE, 0);
        if (len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << " edge mode read finish " << currentFd << std::endl;
            }
            else
            {
                std::cout << " other read finish " << currentFd << std::endl;
            }
            return totalLen;
        }
        if (len == 0)
        {
            std::cout << " remote closed fd " << currentFd << std::endl;
            ::close(currentFd);
            return totalLen;
        }
        totalLen += len;
    }
}

int EllBaseServer::handleAcceptEv(const struct kevent &ev)
{
    int listenFd = ev.ident;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    socklen_t sz = sizeof(addr);
    int port;
    int acceptn = 0;
    while (1)
    {
        // 边缘触发，需要一次处理多个连接
        int newfd = accept(listenFd, (sockaddr *)&addr, &sz);
        if (newfd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << "finish once accept loop" << std::endl;
                break;
            }
            else
            {
                newConnection(newfd);
                acceptn++;
            }
        }
    }
    return acceptn;
}

void EllBaseServer::newConnection(const int newfd)
{
    std::cout << "accept one sock " << newfd << std::endl;
    EasyEllConn *eec = new EasyEllConn(this);
    eec->registerReadEv(); // 注册边缘模式读事件
    addConn(eec);
}

int EllBaseServer::handleWriteEv(const struct kevent &ev)
{
    std::cout << "handleWrite" << ev.ident << std::endl;
    int sockfd = ev.ident;
    EllConn *ec = getConn(sockfd);
    if (ec == nullptr)
    {
        return 0;
    }
    if (ev.udata != nullptr)
    {
        EventContext *evc = (EventContext *)ev.udata;
        if (evc != nullptr)
        {
            int sent = ec->writeSingleData();
            if (sent < 0 && errno == EPIPE)
            {
                ec->unregisterWriteEv();
                delConn(ec->getSock());
            }
            return sent;
        }
        else
        {
            std::cout << " write message udata cast failed " << ev.ident << std::endl;
            ec->unregisterWriteEv();
        }
        return 0;
    }
    else
    {
        std::cout << " write message udata null " << ev.ident << std::endl;
        ec->unregisterWriteEv();
        return 0;
    }
}

int EllBaseServer::loopEVQ()
{
    if (_kq <= 0)
    {
        return -1;
    }
    int eventn = kevent(_kq, nullptr, 0, event_list, LOOP_EVENT_NUM, &ts);
    if (eventn < 0)
    {
        std::cout << "illegal kq err:" << _kq << errno << std::endl;
        return eventn;
    }
    if (eventn == 0)
    {
        return 0;
    }
    for (int i = 0; i < eventn; i++)
    {
        struct kevent ev = event_list[i];
        switch (ev.filter)
        {
        case EVFILT_READ:
        {
            handleReadEv(ev);
            break;
        }
        case EVFILT_WRITE:
        {
            handleWriteEv(ev);
            break;
        }

        default:
        {
            std::cout << "unexpected event filter:" << ev.filter << std::endl;
            break;
        }
        }
    }
    return eventn;
}

int EllBaseServer::startListen(std::string addr, int port){
    if(!listened_addr.empty()) {
        std::cout << "has listened an address " << listened_addr << std::endl;
        exit(-1);
    }
    EasyEllConn *eecl = new EasyEllConn(this);
    if(!eecl->bindAddr(addr, port)){
        std::cout << "listen run1";
        exit(-2);
    }
    if(!eecl->listen()){
        std::cout << "listen run2";
        exit(-3);
    }
    addConn(eecl);
    eecl->registerReadEv();
    listened_addr = addr;
    listened_port = port;
    return eecl->getSock();
}

int EllBaseServer::connectTo(std::string addr, int port){
    EasyEllConn *eec = new EasyEllConn(this);
    if (eec->connect(addr.c_str(), port) < 0)
    {
        perror("connect err");
        exit(-1);
    }
    eec->registerReadEv();
    addConn(eec);
    return eec->getSock();
}

EllConn* EllBaseServer::newPipe(int pipe_fd){
    EasyEllConn *pipeConn = new EasyEllConn(this, pipe_fd, SOCKET_TYPE_PIPE);
    pipeConn->registerReadEv();
    addConn(pipeConn);
    std::cout << "new pipe conn " << pipe_fd << std::endl;
    return pipeConn;
}

std::ostream& operator<<(std::ostream& os, const EllBaseServer& ebs){
    std::ostringstream oss;
    for (std::unordered_map<int, EllConn*>::const_iterator i = ebs._connMap.begin(); i != ebs._connMap.end(); i++)
    {
        if(i->second != nullptr){
            oss << "{fd:" << i->first << ",conn:" << *i->second << "}";
        }else{
            oss << "{fd:" << i->first << ",conn:nullptr" << "}";
        }
    }
    
    os << "EllBaseServer{kq:" << ebs._kq << ", connmap{" << oss.str() << "}\n";
    return os;
}