#include <functional>
#include <unordered_map>
#include <time.h>
#include "EventContext.h"

#define WAIT_EVENT_NUM 64

enum EventType{
    EventRead,
    EventWrite,
    EventError
};

using EventCallback = std::function<void(int fd, EventType et, EventContext &eContext)>;

class EventHandler{
    public:
    virtual int start() = 0;
    virtual int add_event(int fd, EventType et, EventCallback ec) = 0;
    virtual void del_event(int fd) = 0;
    virtual int wait_event() = 0;

    protected:
    std::unordered_map<int, std::unordered_map<EventType, EventCallback>> events_cb;
};