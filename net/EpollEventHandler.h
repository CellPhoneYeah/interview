#include "EventHandler.h"
#include <sys/epoll.h>
class EventContext;

class EpollEventHandler: public EventHandler{
    public:
    int start() override;
    int add_event(int fd, EventType et, EventCallback ec) override;
    void del_event(int fd) override;
    int wait_event() override;
    private:
    int event_fd;
    struct epoll_event events[64];
    EventType EpollEvent2Event(int epollEvent);
    private:
    std::unordered_map<int, EventContext> contexts;
};