#include "EllConn.h"
#include <thread>
#include <fcntl.h>
#include <cmath>
#include "EllBaseServer.h"

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
        std::cout << "new sock " << sockFd << "conn " << *ec << std::endl;
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

EllConn::EllConn(const EllBaseServer* pEbs, int sockfd, int sockType)
{
    _sockfd = sockfd;
    _last_pos = 0;
    bzero(_ring_buffer, RING_BUFFER_SIZE);
    bzero(_read_buffer, READ_BUFFER_SIZE);
    _dh = nullptr;
    _read_pos = 0;
    _ec = new EventContext();
    _kq = pEbs->getEVQ();
    _isListenFd = false;
    _sock_type = sockType;
}

EllConn::EllConn(const EllBaseServer* pEbs, int sockfd)
{
    _sockfd = sockfd;
    _last_pos = 0;
    bzero(_ring_buffer, RING_BUFFER_SIZE);
    bzero(_read_buffer, READ_BUFFER_SIZE);
    _dh = nullptr;
    _read_pos = 0;
    _ec = new EventContext();
    _kq = pEbs->getEVQ();
    _isListenFd = false;
    _sock_type = SOCKET_TYPE_SOCK;
}

EllConn::EllConn(const EllBaseServer* pEbs)
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    _last_pos = 0;
    bzero(_ring_buffer, RING_BUFFER_SIZE);
    bzero(_read_buffer, READ_BUFFER_SIZE);
    _dh = nullptr;
    _read_pos = 0;
    _ec = new EventContext();
    _kq = pEbs->getEVQ();
    _isListenFd = false;
    _sock_type = SOCKET_TYPE_SOCK;
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
    _sock_type = SOCKET_TYPE_SOCK;
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

bool EllConn::canRegisterEv(){
    if (isClosed())
    {
        return false;
    }
    if (!isBindedKQ())
    {
        return false;
    }
    return true;
}

int EllConn::registerReadEv(void *udata)
{
    if(!canRegisterEv()){
        return -1;
    }
    fcntl(_sockfd, F_SETFL, O_NONBLOCK); // 设置为非阻塞模式
    struct kevent event_change;
    if(udata == nullptr){
        udata = this;
    }
    EV_SET(&event_change, _sockfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, udata); // 设置为边缘模式，事件只触发一次
    int ret = kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
    std::cout << "register read ev " << _sockfd << std::endl;
    _ev_list[READ_EVENT] = 1;
    return ret;
}

int EllConn::unregisterReadEv()
{
    if(!canRegisterEv()){
        return -1;
    }
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    int ret = kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
    std::cout << "unregister read ev" << _sockfd << std::endl;
    _ev_list[READ_EVENT] = 0;
    return ret;
}

int EllConn::registerAcceptEv(void *udata)
{
    if(!canRegisterEv()){
        return -1;
    }
    fcntl(_sockfd, F_SETFL, O_NONBLOCK); // 设置为非阻塞模式
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, udata); // 边缘触发
    std::cout << "register accept ev" << _sockfd << std::endl;
    _ev_list[READ_EVENT] = 1;
    return kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
}

int EllConn::registerWriteEv(void *udata)
{
    if(!canRegisterEv()){
        return -1;
    }
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, udata); // 不使用边缘出发的写，因为系统缓冲区满是频繁遇到的事，减少对这种情况的处理
    std::cout << "register accept ev" << _sockfd << std::endl;
    _ev_list[WRITE_EVENT] = 1;
    return kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
}

int EllConn::unregisterWriteEv()
{
    if(!canRegisterEv()){
        return -1;
    }
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    std::cout << "unregister write ev" << _sockfd << std::endl;
    _ev_list[WRITE_EVENT] = 0;
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
    bzero(_bind_ipaddr, IP_ADDRESS_LEN);
    strcpy(_bind_ipaddr, ipAddr.c_str());
    _bind_port = port;
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
    _isListenFd = true;
    return true;
}

int EllConn::readSock(char* buffer, int size){
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

int EllConn::readData(const struct kevent &ev)
{
    int totalLen = 0;
    std::cout << _sockfd << " readdata" << std::endl;
    while (1)
    {
        int len;
        if (_size == 0)
        {
            len = readSock(_ring_buffer, RING_BUFFER_SIZE);
        }
        else
        {
            len = readSock(_ring_buffer + _size, RING_BUFFER_SIZE - _size);
        }
        if (len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << ev.ident << "read edge mode block" << totalLen << std::endl;
            }
            else if (errno == ECONNRESET)
            {
                std::cout << ev.ident << "read data pause, remote conn reseted";
            }
            else
            {
                std::cout  << ev.ident << "read data pause, err:" << errno << std::endl;
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

void EllConn::clearWriteBuffer(){
    if(_ec != nullptr){
        delete(_ec);
    }
}

std::ostream &operator<<(std::ostream &os, const EllConn &ec)
{
    os << &ec << "EllConn{_sockfd=" << ec._sockfd << ", ip:" << ec.getBindIp() << ", port:" << ec.getBindPort() << ", readev:" << ec._ev_list[0] << ", writeev:" << ec._ev_list[1] << ", isPipe:" << ec._sock_type << "}\n";
    return os;
}