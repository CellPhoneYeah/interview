#ifndef EPOLL_EVENT_CONTEXT_H
#define EPOLL_EVENT_CONTEXT_H
#include "EventContext.h"
#include <sys/epoll.h>

class EpollEventContext: public EventContext{
public:
    EpollEventContext(int fd): EventContext(fd){}
    EpollEventContext(int fd, bool listening): EventContext(fd, listening){}
    ~EpollEventContext() override;
    int handle_event(void*event) override;
    void set_noblocking(int fd);

private:
    int handle_read_event(epoll_event*event);
    int handle_write_event(epoll_event*event);
    void process_data(char* data, int size);
    void modify_ev(int fd, int flag, bool enable);
};
#endif