#include "EllConn.h"
#include <thread>
#include <fcntl.h>
#include <cmath>
#include "ConnManager.h"
#include "EllNetConfig.h"
#include "EventContext.h"
#include <iostream>
#include <cstring>
#include "CustomProto.h"


template<typename TEvent>
void EllConn<TEvent>::initBuff(char* arr, int size){
    std::fill(arr, arr + size, '\0');
}

template<typename TEvent>
EllConn<TEvent>::EllConn(int evQ, int sockfd, int sockType)
{
    _sockfd = sockfd;
    _last_pos = 0;
    initBuff(_ring_buffer, ENConfig.RING_BUFFER_SIZE);
    initBuff(_read_buffer, ENConfig.READ_BUFFER_SIZE);
    _dh = nullptr;
    _read_pos = 0;
    _ec = new EventContext();
    _evQ = evQ;
    _isListenFd = false;
    _sock_type = sockType;
}

template<typename TEvent>
EllConn<TEvent>::EllConn(int evQ, int sockfd)
{
    _sockfd = sockfd;
    _last_pos = 0;
    initBuff(_ring_buffer, ENConfig.RING_BUFFER_SIZE);
    initBuff(_read_buffer, ENConfig.READ_BUFFER_SIZE);
    _dh = nullptr;
    _read_pos = 0;
    _ec = new EventContext();
    _evQ = evQ;
    _isListenFd = false;
    _sock_type = SOCKET_TYPE_SOCK;
}

template<typename TEvent>
EllConn<TEvent>::EllConn(int evQ)
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    _last_pos = 0;
    initBuff(_ring_buffer, ENConfig.RING_BUFFER_SIZE);
    initBuff(_read_buffer, ENConfig.READ_BUFFER_SIZE);
    _dh = nullptr;
    _read_pos = 0;
    _ec = new EventContext();
    _evQ = evQ;
    _isListenFd = false;
    _sock_type = SOCKET_TYPE_SOCK;
}

template<typename TEvent>
EllConn<TEvent>::EllConn()
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    _last_pos = 0;
    initBuff(_ring_buffer, ENConfig.RING_BUFFER_SIZE);
    initBuff(_read_buffer, ENConfig.READ_BUFFER_SIZE);
    _dh = nullptr;
    _read_pos = 0;
    _ec = new EventContext();
    _evQ = -1;
    _isListenFd = false;
    _sock_type = SOCKET_TYPE_SOCK;
}

template<typename TEvent>
void EllConn<TEvent>::close()
{
    if (!isClosed())
    {
        std::cout << "close fd" << _sockfd << std::endl;
        unregisterReadEv();
        unregisterWriteEv();
        onCloseFd();
        ::close(_sockfd);
    }

    _sockfd = INVALID_SOCK;
}

template<typename TEvent>
int EllConn<TEvent>::getEVQ()
{
    return _evQ;
}

template<typename TEvent>
int EllConn<TEvent>::getSock()
{
    return _sockfd;
}

template<typename TEvent>
bool EllConn<TEvent>::isClosed()
{
    return !checkSock();
}

template<typename TEvent>
bool EllConn<TEvent>::checkSock()
{
    return _sockfd != INVALID_SOCK;
}

template<typename TEvent>
bool EllConn<TEvent>::bindEVQ(int evQ)
{
    if(isBindedEVQ())
    {
        return false;
    }
    _evQ = evQ;
    return true;
}

template<typename TEvent>
bool EllConn<TEvent>::isBindedEVQ()
{
    return _evQ != -1;
}

template<typename TEvent>
bool EllConn<TEvent>::canRegisterEv(){
    if (isClosed())
    {
        return false;
    }
    if (!isBindedEVQ())
    {
        return false;
    }
    return true;
}

template<typename TEvent>
int EllConn<TEvent>::registerReadEv(void *udata)
{
    if(!canRegisterEv()){
        return -1;
    }
    fcntl(_sockfd, F_SETFL, O_NONBLOCK); // 设置为非阻塞模式
    if(udata == nullptr){
        udata = this;
    }
   int ret = doRegisterReadEv(udata);
    if(ret > 0){
        _ev_list[READ_EVENT] = 1;
        return ret;
        std::cout << "register read ev success " << _sockfd << std::endl;
    }
    return ret;
}

template<typename TEvent>
int EllConn<TEvent>::unregisterReadEv()
{
    if(!checkSock()){
        return -1;
    }
    int ret = doUnregisterReadEv();
    if(ret == 0){
        std::cout << "unregister read ev" << _sockfd << std::endl;
        _ev_list[READ_EVENT] = 0;
    }
    
    return ret;
}

template<typename TEvent>
int EllConn<TEvent>::registerWriteEv(void *udata)
{
    if(!checkSock()){
        return -1;
    }
    int ret = doRegisterWriteEv(udata);
    if(ret == 0){
        std::cout << "register write ev success " << _sockfd << std::endl;
        _ev_list[WRITE_EVENT] = 1;
    }
    return ret;
}

template<typename TEvent>
int EllConn<TEvent>::unregisterWriteEv()
{
    if(!canRegisterEv()){
        return -1;
    }
    int ret = doUnregisterWriteEv();
    if(ret == 0){
        std::cout << "unregister write ev success " << _sockfd << std::endl;
        _ev_list[WRITE_EVENT] = 0;
    }
    
    return ret;
}

template<typename TEvent>
int EllConn<TEvent>::connect(const char *ipaddr, int port)
{
    struct sockaddr_in targetAddr;
    initBuff((char*)&targetAddr, sizeof(targetAddr));
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_addr.s_addr = inet_addr(ipaddr);
    targetAddr.sin_port = htons(port);
    if (::connect(_sockfd, (sockaddr *)&targetAddr, sizeof(targetAddr)) < 0)
    {
        std::stringstream ss;
        ss << "connect to " << ipaddr << ":" << port << "failed";
        perror(ss.str().c_str());
        return -1;
    }
    else
    {
        return 0;
    }
}

template<typename TEvent>
int EllConn<TEvent>::writeSingleData()
{
    if (_ec->writeQ.size() == 0)
    {
        unregisterWriteEv();
        return 0;
    }
    if(_ec->writeQ.front().size() == _ec->offsetPos){
        unregisterWriteEv();
        return 0;
    }
    std::vector<char> data = _ec->writeQ.front();
    int sent = send(_sockfd, data.data() + _ec->offsetPos, data.size() - _ec->offsetPos, MSG_NOSIGNAL | MSG_DONTWAIT);
    std::cout << "sent  " << sent << " data len " << data.size() << std::endl;
    if (sent < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            std::cout << "send failed sys buffer full " << std::endl;
            registerWriteEv();
        }
        if (errno == EPIPE)
        {
            std::cout << "send failed remote conn closed" << std::endl;
            // 对端断开则断开
            unregisterWriteEv();
        }
        else
        {
            std::cout << "send failed unknow sys errno " << errno << std::endl;
            unregisterWriteEv();
        }
        return sent;
    }
    else if (sent < (data.size() - _ec->offsetPos))
    {
        _ec->offsetPos += sent;
        std::cout << "sent1  " << sent << std::endl;
        return sent;
    }
    else
    {
        _ec->writeQ.pop();
        _ec->offsetPos = 0;
        if(_ec->writeQ.size() == 0){
            unregisterWriteEv();
        }
        std::cout << "sent2  " << sent << std::endl;
        return sent;
    }
}

template<typename TEvent>
int EllConn<TEvent>::sendData(char *data, int size)
{
    if (isClosed())
    {
        return -1;
    }
    if(size == 0){
        return -2;
    }
    if(size >= ENConfig.WRITE_BUFFER_SIZE * 10){
        return -3;
    }
    int pushCount = 0;
    if (size < ENConfig.WRITE_BUFFER_SIZE)
    {
        _ec->writeQ.push(std::vector<char>(data, data + size));
        std::cout << "push to WriteQ" << size << std::endl;
        pushCount++;
    }
    else
    {
        int leftSize = size;
        int lastOffset = 0;
        int count = ceil((double)size / ENConfig.WRITE_BUFFER_SIZE);
        for (int i = 0; i < count; i++)
        {
            if (leftSize > 0)
            {
                _ec->writeQ.push(std::vector<char>(data + lastOffset, data + lastOffset + ENConfig.WRITE_BUFFER_SIZE));
                std::cout << "push to WriteQ1" << ENConfig.WRITE_BUFFER_SIZE << std::endl;
            }
            leftSize -= ENConfig.WRITE_BUFFER_SIZE;
            lastOffset += ENConfig.WRITE_BUFFER_SIZE;
            pushCount++;
        }
    }
    registerWriteEv();

    return pushCount;
}

template<typename TEvent>
bool EllConn<TEvent>::bindAddr(std::string ipAddr, int port)
{
    if (!checkSock())
    {
        return false;
    }
    struct sockaddr_in server_addr;
    initBuff((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr(ipAddr.c_str());
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (::bind(_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind err");
        exit(-1);
    }
    initBuff(_bind_ipaddr, ENConfig.IP_ADDRESS_LEN);
    strcpy(_bind_ipaddr, ipAddr.c_str());
    _bind_port = port;
    return true;
}

template<typename TEvent>
bool EllConn<TEvent>::listen()
{
    if (!checkSock())
    {
        return false;
    }
    if(!isBindedEVQ()){
        return false;
    }
    fcntl(_sockfd, F_SETFL, O_NONBLOCK);
    int listenret = ::listen(_sockfd, ENConfig.MAX_LISTEN_CONN);
    if (listenret < 0)
    {
        perror("listen error");
        ::close(_sockfd);
        _sockfd = INVALID_SOCK;
        exit(-1);
    }
    _isListenFd = true;
    return true;
}

template<typename TEvent>
int EllConn<TEvent>::readSock(char* buffer, int size){
    if(isPipe()){
        int n = 0;
        do {
            n = read(_sockfd, buffer, size);
        } while (n == -1 && errno == EINTR);
        return n;
    }else{
        return recv(_sockfd, buffer, size, 0);
    }
}

template<typename TEvent>
int EllConn<TEvent>::readData(TEvent*ev)
{
    int totalLen = 0;
    std::cout << _sockfd << " readdata" << std::endl;
    while (1)
    {
        int len;
        if (_size == 0)
        {
            len = readSock(_ring_buffer, ENConfig.RING_BUFFER_SIZE);
        }
        else
        {
            len = readSock(_ring_buffer + _size, ENConfig.RING_BUFFER_SIZE - _size);
        }
        if (len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << getEventFd(ev) << "read edge mode block" << totalLen << std::endl;
            }
            else if (errno == ECONNRESET)
            {
                std::cout << getEventFd(ev) << "read data pause, remote conn reseted";
            }
            else
            {
                std::cout  << getEventFd(ev) << "read data pause, err:" << errno << std::endl;
            }
            return totalLen;
        }
        if (len == 0)
        {
            std::cout << "read data pause, remote conn closed" << std::endl;
            return totalLen;
        }
        _size += len;
        totalLen += len;
        int leftLen = len;
        int maxloop = 1000;
        int loopCount = 0;
        while (leftLen > 0)
        {
            if (_dh == nullptr)
            {
                if (leftLen >= DATAHEADER_LEN)
                {
                    memcpy(_read_buffer, _ring_buffer + _last_pos, DATAHEADER_LEN);
                    _dh = (DataHeader *)(_read_buffer + _read_pos);
                    _last_pos += DATAHEADER_LEN;
                    _size -= DATAHEADER_LEN;
                    leftLen -= DATAHEADER_LEN;
                    _read_pos = DATAHEADER_LEN;
                }
                else
                {
                    memcpy(_read_buffer + _read_pos, _ring_buffer + _last_pos, leftLen);
                    _last_pos += leftLen;
                    leftLen = 0;
                    _read_pos += leftLen;
                    _size = 0;
                }
            }
            else
            {
                int needReadSize = _dh->dataLen - DATAHEADER_LEN;
                if (leftLen >= needReadSize)
                {
                    memcpy(_read_buffer + _read_pos, _ring_buffer + _last_pos, needReadSize);
                    handleOneProto(ev);
                    _last_pos += needReadSize;
                    leftLen -= needReadSize;
                    _size -= needReadSize;
                    _read_pos = 0;
                    _dh = nullptr;
                }
                else
                {
                    memcpy(_read_buffer + _read_pos, _ring_buffer + _last_pos, leftLen);
                    _last_pos += leftLen;
                    _read_pos += leftLen;
                    leftLen = 0;
                    _size = 0;

                    std::cout << "not enough one proto size" << std::endl;
                }
            }
            loopCount++;
            if (loopCount >= maxloop)
            {
                std::cout << "check the logic any bug for reach the max loop times" << std::endl;
                return -1;
            }
        }
        _last_pos = 0;
    }
}

template<typename TEvent>
int EllConn<TEvent>::loopListenSock(TEvent*events, int size)
{
    if(!checkSock()){
        return -1;
    }
    struct timespec ts = {0, 0};
    int changen = kevent(_kq, nullptr, 0, events, size, &ts);
    if (changen < 0)
    {
        perror("accept error");
        close();
        return -1;
    }
    if(changen == 0){
        sleep(1);
        return 0;
    }
    sockaddr_in client_addr;
    socklen_t clientaddr_len = sizeof(client_addr);
    if(changen > 0){
        std::cout << "kqueue event len:" << changen << std::endl;
    }
    for (size_t i = 0; i < changen; i++)
    {
        Tevent ev = events[i];
        uintptr_t currentfd = getEventFd(ev);

        if (currentfd == getSock() && _isListenFd)
        {
            std::cout << "accept client" << currentfd << std::endl;
            int clientfd = accept(_sockfd, (sockaddr *)&client_addr, &clientaddr_len);
            if (clientfd < 0)
            {
                perror("accept err");
            }
            else
            {
                if (!acceptSock(clientfd, this))
                {
                    ::close(clientfd);
                }
            }
        }
        else if(getEventFilter(ev) == EVFILT_WRITE){
            EllConn* ec = getClient(currentfd);
            std::cout << std::this_thread::get_id() << " th " << i << " write data to:" << ec->getSock() << " ev " << &ev.udata << std::endl;
            for (size_t i = 0; i < 10; i++)
            {
                int sent = ec->writeSingleData();
                if(sent < 0){
                    ec->close();
                    EllConn<TEvent>::delClient(ec->getSock());
                    break;
                }else if(sent == 0){
                    break;
                }
            }
        }
        else if(getEventFilter(ev) == EVFILT_READ)
        {
            if (getEventFlag(ev) | EV_EOF)
            {
                std::cout << " input has closed \n";
                EllConn *ec = getClient(currentfd);
                ec->close();
                EllConn<TEvent>::delClient(currentfd);
                continue;
            }
            if(getEventUdata(ev) != nullptr){
                EventContext* currentEC = (EventContext*)ev.udata;
                if(currentEC == nullptr){
                    std::cout << "write data err, it may case memleek!" << std::endl;
                    continue;
                }
                if(currentEC->socket_type == SOCKET_TYPE_PIPE){
                    std::cout << "pipe get msg" << std::endl;
                    char message[BUFFSIZE];
                    bzero(&message, BUFFSIZE);
                    ssize_t len = read(currentfd, message, BUFFSIZE);
                    std::cout << "read pipe:" << std::string(message) << std::endl;
                    EllConn* targetEcn = getClient(currentEC->targetfd);
                    if(len < 0){
                        std::cout << "pipe err:" << errno << std::endl;
                        deleteReadEvent(&ev);
                        ::close(currentfd);
                        targetEcn->close(); // close socket
                        ConnManager::delClient(currentEC->targetfd);
                        continue;
                    }else if(len == 0){
                        std::cout << "pipe sender closed, so reader close" << std::endl;
                        deleteReadEvent(&ev);
                        ::close(currentfd); // close pipe
                        targetEcn->close(); // close socket
                        ConnManager::delClient(currentEC->targetfd);
                        continue;
                    }
                    
                    if(currentEC->targetfd != INVALID_SOCK){
                        if(targetEcn == nullptr){
                            std::cout << "write data to empty target" << std::endl;
                            continue;
                        }
                        targetEcn->sendData(message, len);
                    }
                }
                continue;
            }
            std::cout << "read data" << ev.ident << std::endl;
            EllConn<TEvent>* ec = ConnManager::getClient(currentfd);
            if(ec == nullptr){
                std::cout << "client lost " << currentfd << std::endl;
            }
            if (ec->readData(ev) <= 0 || (ev.flags & EV_EOF))
            {
                ec->close();
                ConnManager::::delClient(getSock());
            }
        }else{
            std::cout << "unexpected event filter" << ev.filter << std::endl;
        }
    }
    return changen;
};


template<typename TEvent>
void EllConn<TEvent>::clearWriteBuffer(){
    if(_ec != nullptr){
        delete(_ec);
    }
}

template<typename TEvent>
std::ostream &operator<<(std::ostream &os, const EllConn<TEvent> &ec)
{
    os << &ec << "EllConn{_sockfd=" << ec._sockfd << ", ip:" << ec.getBindIp() << ", port:" << ec.getBindPort() << ", readev:" << ec._ev_list[0] << ", writeev:" << ec._ev_list[1] << ", isPipe:" << ec._sock_type << "}\n";
    return os;
}