#include "epoll_event_handler.h"

#include <unistd.h>
#include <sys/epoll.h>

#include <iostream>

#include "event_handler.h"
#include "ellnet/epoll_event_context.h"

namespace ellnet
{
    int EpollEventHandler::Start()
    {
        event_fd_ = epoll_create1(0);
        return event_fd_;
    }

    int EpollEventHandler::AddEvent(int fd, EventType et)
    {
        struct epoll_event ev;
        ev.events = 0;
        if (et & EVENT_READ)
        {
            ev.events |= EPOLLIN;
        }
        if (et & EVENT_WRITE)
        {
            ev.events |= EPOLLOUT;
        }
        if (epoll_ctl(event_fd_, EPOLL_CTL_ADD, fd, &ev) == -1)
        {
            epoll_ctl(event_fd_, EPOLL_CTL_MOD, fd, &ev);
        }
        if (events_registered_.find(fd) == events_registered_.end())
        {
            events_registered_[fd] = std::vector<bool>({false, false, false});
            events_registered_[fd][et] = true;
        }
        else
        {
            events_registered_[fd][et] = true;
        }
        return 0;
    }

    void EpollEventHandler::DelEvent(int fd)
    {
        epoll_ctl(event_fd_, EPOLL_CTL_DEL, fd, nullptr);
        events_registered_.erase(fd);
    }

    int EpollEventHandler::WaitEvent()
    {
        while (1)
        {
            int eventn = epoll_wait(event_fd_, events_, WAIT_EVENT_NUM, 0);
            if (eventn < 0)
            {
                std::cout << "epoll wait err:" << errno << std::endl;
            }
            if (eventn == 0)
            {
                std::cout << "epoll wait no event to handle \n";
                sleep(1);
            }
            for (int i = 0; i < eventn; i++)
            {
                epoll_event ev = events_[i];
                int currentFd = ev.data.fd;
                std::unordered_map<int, std::vector<bool>>::const_iterator registered = events_registered_.find(currentFd);
                if (registered == events_registered_.end())
                {
                    std::cout << "epoll cannot find event listening list for ev " << currentFd << std::endl;
                    continue;
                }
                EventType et = EpollEvent2Event(ev.events);
                if (!registered->second[et])
                {
                    std::cout << "epoll cannot find event listening for ev " << currentFd << " event type " << et << std::endl;
                    continue;
                }
                HandleEvent(currentFd, ev);
            }
        }
        close(event_fd_);
        std::cout << "epoll stop wait " << event_fd_ << std::endl;
        return 0;
    }

    EventType EpollEventHandler::EpollEvent2Event(int epollEvent)
    {
        if (epollEvent & EPOLLIN)
        {
            return EVENT_READ;
        }
        if (epollEvent & EPOLLOUT)
        {
            return EVENT_WRITE;
        }
        return EVENT_ERROR;
    }

    void EpollEventHandler::HandleEvent(int fd, epoll_event &ev)
    {
        EpollEventContext *eec = (EpollEventContext *)ev.data.ptr;
        eec->HandleEvent(&ev);
    }

}
