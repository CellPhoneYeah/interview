#include "EllConn.h"

#define RING_BUFFER_SIZE 10240
#define READ_BUFFER_SIZE 1024
#define WRITE_BUFFER_SIZE 4096
#define WRITE_QUEUE_SIZE 1024
#define INVALID_SOCK (~0)

std::unordered_map<int, EllConn*> EllConn::clientMap;

void EllConn::registerWrite()
{
    struct kevent ev;
    EV_SET(&ev, _sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, _wc);
    kevent(_kq, &ev, 1, nullptr, 0, nullptr);
}

void EllConn::unregisterWrite()
{
    struct kevent ev;
    EV_SET(&ev, _sockfd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    kevent(_kq, &ev, 1, nullptr, 0, nullptr);
}

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
        EllConn::clientMap.insert(std::make_pair(sockFd, ec));
    }
}
void EllConn::delClient(int sockFd)
{
    std::unordered_map<int, EllConn *>::const_iterator it = EllConn::clientMap.find(sockFd);
    if (it != EllConn::clientMap.end())
    {
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
    _wc = new WriteContext();
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
    _wc = new WriteContext();
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
    _wc = new WriteContext();
    _kq = -1;
    _isListenFd = false;
}

void EllConn::close()
{
    if (!isClosed())
    {
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
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, udata);
    int ret = kevent(_kq, &event_change, 1, nullptr, 0, nullptr);
    return ret;
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
    struct kevent event_change;
    EV_SET(&event_change, _sockfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, udata);
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
    EV_SET(&event_change, _sockfd, EVFILT_WRITE, EV_ADD, 0, 0, udata);
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
    if (_wc->writeQ.size() == 0)
    {
        return 0;
    }
    std::vector<char> data = _wc->writeQ.front();
    int sent = send(_sockfd, data.data() + _wc->offsetPos, data.size() - _wc->offsetPos, MSG_NOSIGNAL);
    if (sent < 0)
    {
        if (errno == EPIPE)
        {
            std::cout << "remote conn closed" << std::endl;
        }
        else
        {
            std::cout << "sys buffer full " << std::endl;
        }
        // 边缘触发， 事件自动取消注册了
        return sent;
    }
    else if (sent < data.size())
    {
        _wc->offsetPos += sent;
        registerWriteEv(); // 边缘触发，需要重新注册事件
        return sent;
    }
    else
    {
        _wc->writeQ.pop();
        _wc->offsetPos = 0;
        if(_wc->writeQ.size() > 0){
            registerWriteEv();
        }
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
    int pushCount = 0;
    char * newData = new char[size];
    if (size < WRITE_BUFFER_SIZE)
    {
        _wc->writeQ.push(std::vector<char>(newData, newData + size));
        pushCount++;
    }
    else
    {
        int leftSize = size;
        int lastOffset = 0;
        for (int i = 0; i < size / WRITE_BUFFER_SIZE; i++)
        {
            if (leftSize > 0)
            {
                _wc->writeQ.push(std::vector<char>(data + lastOffset, data + lastOffset + WRITE_BUFFER_SIZE));
            }
            leftSize -= WRITE_BUFFER_SIZE;
            lastOffset += WRITE_BUFFER_SIZE;
            pushCount++;
        }
    }
    registerWrite();

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
    int len;
    if (_size == 0)
    {
        len = recv(_sockfd, _ring_buffer, RING_BUFFER_SIZE, 0);
    }
    else
    {
        len = recv(_sockfd, _ring_buffer + _size, RING_BUFFER_SIZE - _size, 0);
    }
    if (len == -1)
    {
        if (errno == ECONNRESET)
        {
            std::cout << "read data pause, remote conn reseted";
        }
        else
        {
            std::cout << "read data pause, err:" << errno;
        }
        return len;
    }
    if (len == 0)
    {
        std::cout << "read data pause, remote conn closed";
        return 0;
    }
    _size += len;
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
            return -1;
        }
    }
    _last_pos = 0;
    return len;
}

int EllConn::loopListenSock(struct kevent *events, int size)
{
    int changen = kevent(_kq, nullptr, 0, events, size, nullptr);
    if (changen < 0)
    {
        perror("accept error");
        close();
        return -1;
    }
    sockaddr_in client_addr;
    socklen_t clientaddr_len = sizeof(client_addr);
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
            std::cout << "write data to:" << ec->getSock() << std::endl;
            for (size_t i = 0; i < 10; i++)
            {
                if(ec->writeSingleData() <= 0){
                    break;
                }
            }
        }
        else if(ev.filter == EVFILT_READ)
        {
            if(ev.udata != nullptr){
                WriteContext* currentWc = (WriteContext*)ev.udata;
                if(currentWc == nullptr){
                    std::cout << "write data err, it may case memleek!" << std::endl;
                    continue;
                }
                if(currentWc->socket_type == SOCKET_TYPE_PIPE){
                    std::cout << "pipe get msg" << std::endl;
                    char message[BUFFSIZE];
                    bzero(&message, BUFFSIZE);
                    ssize_t len = read(ev.ident, message, BUFFSIZE);
                    std::cout << "read pipe:" << std::string(message) << std::endl;
                    if(len < 0){
                        std::cout << "pipe err:" << errno << std::endl;
                        ::close(ev.ident);
                        continue;
                    }
                    if (len == 0)
                    {
                        std::cout << "pipe sender closed, so reader close" << std::endl;
                        ::close(ev.ident);
                        continue;
                    }
                    if(currentWc->targetfd != INVALID_SOCK){
                        EllConn* targetEc = getClient(currentWc->targetfd);
                        if(targetEc == nullptr){
                            std::cout << "write data to empty target" << std::endl;
                            continue;
                        }
                        targetEc->sendData(message, len);
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
            }
        }else{
            std::cout << "unexpected event filter" << ev.filter << std::endl;
        }
    }
    std::cout << "change n " << changen << std::endl;
    sleep(1);
    return changen;
};