#ifndef EPOLL_EVENT_CONTEXT_H
#define EPOLL_EVENT_CONTEXT_H
#include "EventContext.h"

#include <sys/epoll.h>

class EpollEventContext: public EventContext{
public:
    EpollEventContext(int fd);
    EpollEventContext(int fd, bool listening);
    ~EpollEventContext() override;
    int handle_event(void*event) override;
    void set_noblocking(int fd);
    void pushMsgQ(const char* msg, int size);
    void setPipe(){pipe_flag = true;}
    bool isPipe() {return pipe_flag;}
private:
    int handle_read_event(epoll_event*event);
    int handle_write_event(epoll_event*event);
    void process_data(char* data, int size);
    void modify_ev(int fd, int flag, bool enable);
    bool pipe_flag = false;
};
#endif