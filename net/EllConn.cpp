#include "EllConn.h"
#include <thread>
#include <fcntl.h>
#include <cmath>

#define RING_BUFFER_SIZE 10240
#define READ_BUFFER_SIZE 1024
#define WRITE_QUEUE_SIZE 1024
#define INVALID_SOCK (~0)

std::unordered_map<int, EllConn*> EllConn::clientMap;

EllConn *EllConn::getClient(int sockFd)
{
    if (EllConn::clientMap.find(sockFd) != EllConn::clientMap.end())
    {
        return (*EllConn::clientMap.find(sockFd)).second;
    }
    return nullptr;
}

void EllConn::addClient(int sockFd, EllConn *ec)
{
    if (EllConn::clientMap.find(sockFd) == EllConn::clientMap.end())
    {
        std::cout << "new sock " << sockFd << std::endl;
        EllConn::clientMap.insert(std::make_pair(sockFd, ec));
    }
}
void EllConn::delClient(int sockFd)
{
    std::unordered_map<int, EllConn *>::const_iterator it = EllConn::clientMap.find(sockFd);
    if (it != EllConn::clientMap.end())
    {
        std::cout << "del sock " << sockFd << std::endl;
        EllConn::clientMap.erase(sockFd);
        delete((*it).second);
    }
}

EllConn::EllConn(int kq, int sockfd)
{
    _sockfd = sockfd;
    _last_pos = 0;
    bzero(_ring_buffer, RING_BUFFER_SIZE);
    bzero(_read_buffer, READ_BUFFER_SIZE);
    _dh = nullptr;
    _read_pos = 0;
    _ec = new EventContext();
    _kq = kq;
    _isListenFd = false;
}

EllConn::EllConn(int kq)
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    _last_pos = 0;
    bzero(_ring_buffer, RING_BUFFER_SIZE);
    bzero(_read_buffer, READ_BUFFER_SIZE);
    _dh = nullptr;
    _read_pos = 0;
    _ec = new EventContext();
    _kq = kq;
    _isListenFd = false;
}

EllConn::EllConn()
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    _last_pos = 0;
    bzero(_ring_buffer, RING_BUFFER_SIZE);
    bzero(_read_buffer, READ_BUFFER_SIZE);
    _dh = nullptr;
    _read_pos = 0;
    _ec = new EventContext();
    _kq = -1;
    _isListenFd = false;
}

void EllConn::close()
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

int EllConn::getKQ()
{
    return _kq;
}

int EllConn::getSock()
{
    return _sockfd;
}

bool EllConn::isClosed()
{
    return !checkSock();
}

bool EllConn::checkSock()
{
    return _sockfd != INVALID_SOCK;
}

bool EllConn::bindKQ(int kq)
{
    if(isBindedKQ())
    {
        return false;
    }
    _kq = kq;
    return true;
}

bool EllConn::isBindedKQ()
{
    return _kq != -1;
}

int EllConn::registerReadEv(void *udata)
{
    if (isClosed())
    {
        return -1;
    }
    if (!isBindedKQ())
    {
        return -2;
    }
    fcntl(_sockfd, F_SETFL, O_NONBLOCK); // 设置为非阻塞模式
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, udata); // 设置为边缘模式，事件只触发一次
    int ret = kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
    std::cout << "register read ev" << _sockfd << std::endl;
    return ret;
}

void EllConn::unregisterReadEv()
{
    if (isClosed())
    {
        return;
    }
    if (!isBindedKQ())
    {
        return;
    }
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    int ret = kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
    std::cout << "unregister read ev" << _sockfd << std::endl;
    return;
}

int EllConn::registerAcceptEv(void *udata)
{
    if (isClosed())
    {
        return -1;
    }
    if (!isBindedKQ())
    {
        return -2;
    }
    fcntl(_sockfd, F_SETFL, O_NONBLOCK); // 设置为非阻塞模式
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, udata); // 边缘触发
    std::cout << "register accept ev" << _sockfd << std::endl;
    return kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
}

int EllConn::registerWriteEv(void *udata)
{
    if (isClosed())
    {
        return -1;
    }
    if (!isBindedKQ())
    {
        return -2;
    }
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, udata); // 不使用边缘出发的写，因为系统缓冲区满是频繁遇到的事，减少对这种情况的处理
    std::cout << "register accept ev" << _sockfd << std::endl;
    return kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
}

int EllConn::unregisterWriteEv()
{
    if (isClosed())
    {
        return -1;
    }
    if (!isBindedKQ())
    {
        return -2;
    }
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    std::cout << "unregister write ev" << _sockfd << std::endl;
    return kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
}

int EllConn::connect(const char *ipaddr, int port)
{
    struct sockaddr_in targetAddr;
    bzero(&targetAddr, sizeof(targetAddr));
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

int EllConn::writeSingleData()
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

int EllConn::sendData(char *data, int size)
{
    if (isClosed())
    {
        return -1;
    }
    if(size == 0){
        return -2;
    }
    if(size >= WRITE_BUFFER_SIZE * 10){
        return -3;
    }
    int pushCount = 0;
    if (size < WRITE_BUFFER_SIZE)
    {
        _ec->writeQ.push(std::vector<char>(data, data + size));
        std::cout << "push to WriteQ" << size << std::endl;
        pushCount++;
    }
    else
    {
        int leftSize = size;
        int lastOffset = 0;
        int count = ceil((double)size / WRITE_BUFFER_SIZE);
        for (int i = 0; i < count; i++)
        {
            if (leftSize > 0)
            {
                _ec->writeQ.push(std::vector<char>(data + lastOffset, data + lastOffset + WRITE_BUFFER_SIZE));
                std::cout << "push to WriteQ1" << WRITE_BUFFER_SIZE << std::endl;
            }
            leftSize -= WRITE_BUFFER_SIZE;
            lastOffset += WRITE_BUFFER_SIZE;
            pushCount++;
        }
    }
    registerWriteEv();

    return pushCount;
}

bool EllConn::bindAddr(std::string ipAddr, int port)
{
    if (!checkSock())
    {
        return false;
    }
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr(ipAddr.c_str());
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (::bind(_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind err");
        exit(-1);
    }
    return true;
}

bool EllConn::listen()
{
    if (!checkSock())
    {
        return false;
    }
    if(!isBindedKQ()){
        return false;
    }
    fcntl(_sockfd, F_SETFL, O_NONBLOCK);
    int listenret = ::listen(_sockfd, MAX_CANON);
    if (listenret < 0)
    {
        perror("listen error");
        ::close(_sockfd);
        _sockfd = INVALID_SOCK;
        exit(-1);
    }
    registerReadEv();
    _isListenFd = true;
    return true;
}

int EllConn::readData()
{
    int totalLen = 0;
    while (1)
    {
        int len;
        if (_size == 0)
        {
            len = recv(_sockfd, _ring_buffer, RING_BUFFER_SIZE, 0);
        }
        else
        {
            len = recv(_sockfd, _ring_buffer + _size, RING_BUFFER_SIZE - _size, 0);
        }
        if (len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << "read edge mode block" << std::endl;
            }
            if (errno == ECONNRESET)
            {
                std::cout << "read data pause, remote conn reseted";
            }
            else
            {
                std::cout << "read data pause, err:" << errno << std::endl;
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
                    handleOneProto();
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

int EllConn::loopListenSock(struct kevent *events, int size)
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
        struct kevent ev = events[i];
        uintptr_t currentfd = ev.ident;
        if (currentfd == getSock() && _isListenFd)
        {
            std::cout << "accept client" << events[i].ident << std::endl;
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
        else if(ev.filter == EVFILT_WRITE){
            EllConn* ec = getClient(currentfd);
            std::cout << std::this_thread::get_id() << " th " << i << " write data to:" << ec->getSock() << " ev " << &ev.udata << std::endl;
            for (size_t i = 0; i < 10; i++)
            {
                int sent = ec->writeSingleData();
                if(sent < 0){
                    ec->close();
                    EllConn::delClient(ec->getSock());
                    break;
                }else if(sent == 0){
                    break;
                }
            }
        }
        else if(ev.filter == EVFILT_READ)
        {
            if(ev.udata != nullptr){
                EventContext* currentEC = (EventContext*)ev.udata;
                if(currentEC == nullptr){
                    std::cout << "write data err, it may case memleek!" << std::endl;
                    continue;
                }
                if(currentEC->socket_type == SOCKET_TYPE_PIPE){
                    std::cout << "pipe get msg" << std::endl;
                    char message[BUFFSIZE];
                    bzero(&message, BUFFSIZE);
                    ssize_t len = read(ev.ident, message, BUFFSIZE);
                    std::cout << "read pipe:" << std::string(message) << std::endl;
                    EllConn* targetEcn = getClient(currentEC->targetfd);
                    if(len < 0){
                        std::cout << "pipe err:" << errno << std::endl;
                        struct kevent event_change;
                        EV_SET(&event_change, ev.ident, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
                        kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
                        ::close(ev.ident);
                        targetEcn->close(); // close socket
                        EllConn::delClient(currentEC->targetfd);
                        continue;
                    }else if(len == 0){
                        std::cout << "pipe sender closed, so reader close" << std::endl;
                        struct kevent event_change;
                        EV_SET(&event_change, ev.ident, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
                        kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
                        ::close(ev.ident); // close pipe
                        targetEcn->close(); // close socket
                        EllConn::delClient(currentEC->targetfd);
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
            EllConn* ec = getClient(currentfd);
            if(ec == nullptr){
                std::cout << "client lost " << currentfd << std::endl;
            }
            if (ec->readData() <= 0)
            {
                ec->close();
                EllConn::delClient(getSock());
            }
        }else{
            std::cout << "unexpected event filter" << ev.filter << std::endl;
        }
    }
    return changen;
};


void EllConn::clearWriteBuffer(){
    if(_ec != nullptr){
        delete(_ec);
    }
}