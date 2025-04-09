#ifndef EPOLL_EVENT_HANDLER_H
#define EPOLL_EVENT_HANDLER_H
#include "event_handler.h"

#include <sys/epoll.h>

#include "ellnet/epoll_event_context.h"

namespace ellnet
{
    class EpollEventHandler : public EventHandler
    {
    public:
        virtual int Start() override;
        virtual int AddEvent(int fd, EventType et) override;
        virtual void DelEvent(int fd) override;
        virtual int WaitEvent() override;

    private:
        EventType EpollEvent2Event(int epollEvent);
        void HandleEvent(int fd, struct epoll_event &ev);

        struct epoll_event events_[64];
        int event_fd_;
        std::unordered_map<int, EpollEventContext *> contexts_;
        std::unordered_map<int, std::vector<bool>> events_registered_;
    };
}

#endif