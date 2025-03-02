#ifndef EVENT_CONTEXT_H
#define EVENT_CONTEXT_H
#include <queue>
#include <vector>
#define MAX_READ_BUFFER_SIZE 1024

enum HANDLE_EVENT_CODE{
    HANDLE_EVENT_CODE_OK,
    HANDLE_EVENT_CODE_CONNECT_RESET,
    HANDLE_EVENT_CODE_ACCEPT_ET_WAIT,
    HANDLE_EVENT_CODE_ACCEPT_FAILED,
    HANDLE_EVENT_CODE_ACCEPT_SIGNAL_BREAK,
    HANDLE_EVENT_CODE_ACCEPT_REMOTE_CLOSED,
    HANDLE_EVENT_CODE_REMOTE_CONNECT_CLOSED,
    HANDLE_EVENT_CODE_UNKNOW_ERR,
    HANDLE_EVENT_CODE_SEND_QUEUE_EMPTY,
};

class EventContext{
protected:
    int ownfd = 0;
    int socket_type = 0;
    bool listening = false;
    public:
    std::queue<std::vector<char> > sendQ;
    void readBytes(int byte_len);
    std::vector<char> readBuffer;
    int offsetPos = 0;
    EventContext(int fd);
    EventContext(int fd, bool isListen);
    virtual ~EventContext() = 0;
    virtual int handle_event(void* event) = 0;
    int getFd(){return ownfd;}
    bool isListening(){return listening;}
    void clearSendQ();
};
#endif