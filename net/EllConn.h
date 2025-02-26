#include <unistd.h>
#include <sstream>
#include <arpa/inet.h>
#include <queue>
#include <vector>
#include <unordered_map>
#include "CustomProto.h"
#include "EllNetConfig.h"
class EllBaseServer;

template<typename TEvent>
class EllConn{
private:
    int readSock(char* buffer, int size);
    void initBuff(char* arr, int size);
protected:
    int _sockfd;
    bool _isListenFd;
    int _evQ;
    char _ring_buffer[ENConfig.RING_BUFFER_SIZE];
    char _read_buffer[ENConfig.READ_BUFFER_SIZE];
    int _read_pos;
    int _last_pos;
    int _size;
    DataHeader* _dh;
    EventContext* _ec;
    char _bind_ipaddr[ENConfig.IP_ADDRESS_LEN];
    int _bind_port;
    int _sock_type;
    int _ev_list[4]; // [read write]
    const EllBaseServer* pEbs;

    bool isBindedKQ();
    bool checkSock();
    bool canRegisterEv();
    
public:
    EllConn(const EllBaseServer* pEbs, int sockfd, int sockType);
    EllConn(const EllBaseServer* pEbs, int sockfd);
    EllConn(const EllBaseServer* pEbs);
    EllConn();

    virtual ~EllConn() = default;
    void close();
    int getEVQ() {return _evQ;};

    int getSock() {return _sockfd;};
    bool isClosed();
    bool isPipe(){return _sock_type == SOCKET_TYPE_PIPE;};

    bool checkSock();
    bool bindEVQ(int kq);
    bool isBindedEVQ();

    bool canRegisterEv();
    int registerReadEv(void* udata = nullptr);
    int unregisterReadEv();
    int registerWriteEv(void* udata = nullptr);
    int unregisterWriteEv();

    int connect(const char* ipaddr, int port);
    int writeSingleData();
    int sendData(char* data, int size);
    bool bindAddr(std::string ipAddr, int port);

    bool listen();
    bool isListening(){ return _isListenFd; }
    int readData(TEvent*ev);
    int loopListenSock(TEvent *events, int size);
    const char *getBindIp() const { return _bind_ipaddr; }
    const int getBindPort() const {return _bind_port;}

    virtual bool acceptSock(int clientfd, TEvent* ev) = 0;
    virtual int handleOneProto() = 0;
    virtual void onCloseFd() = 0;
    virtual int doRegisterReadEv(void* data) = 0;
    virtual int doUnregisterReadEv() = 0;
    virtual int doRegisterWriteEv(void* data) = 0;
    virtual int doUnregisterWriteEv() = 0;
    virtual int getEventFd(TEvent*event) = 0;
    virtual int getEventFlag(TEvent*event) = 0;
    virtual int getEventFilter(TEvent*event) = 0;
    virtual void* getEventUdata(TEvent*event) = 0;
    virtual int loopEvent(TEvent* events, int size);

    virtual void* deleteReadEvent(TEvent*event) = 0;
    void clearWriteBuffer();
};