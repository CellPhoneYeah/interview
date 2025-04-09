#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <time.h>

#include <functional>
#include <unordered_map>

#include "ellnet/event_context.h"

namespace ellnet
{
#define WAIT_EVENT_NUM 64

    enum EventType
    {
        EVENT_READ,
        EVENT_WRITE,
        EVENT_ERROR
    };

    class EventHandler
    {
    public:
        virtual int Start() = 0;
        virtual int AddEvent(int fd, EventType et) = 0;
        virtual void DelEvent(int fd) = 0;
        virtual int WaitEvent() = 0;

    
    };
}

#endif