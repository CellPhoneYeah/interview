#include "EpollEventHandler.h"
#include <iostream>
#include <unistd.h>


int EpollEventHandler::start()
{
    event_fd = epoll_create1(0);
    return event_fd;
}

int EpollEventHandler::add_event(int fd, EventType et, EventCallback ecb)
{
    struct epoll_event ev;
    ev.events = 0;
    if(et & EventRead)
    {
        ev.events |= EPOLLIN;
    }
    if(et & EventWrite)
    {
        ev.events |= EPOLLOUT;
    }
    if(epoll_ctl(event_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        epoll_ctl(event_fd, EPOLL_CTL_MOD, fd, &ev);
    }
    if(events_cb.find(fd) == events_cb.end()){
        events_cb[fd] = std::unordered_map<EventType, EventCallback>();
        events_cb[fd][et] = ecb;
    }else{
        events_cb[fd][et] = ecb;
    }
    return 0;
}

void EpollEventHandler::del_event(int fd)
{
    epoll_ctl(event_fd, EPOLL_CTL_DEL, fd, nullptr);
    events_cb.erase(fd);
}

int EpollEventHandler::wait_event()
{
    while(1){
        int eventn = epoll_wait(event_fd, events, WAIT_EVENT_NUM, 0);
        if(eventn < 0){
            std::cout << "epoll wait err:" << errno << std::endl;
        }
        if(eventn == 0){
            std::cout << "epoll wait no event to handle \n";
            sleep(1);
        }
        for (int i = 0; i < eventn; i++)
        {
            epoll_event ev = events[i];
            auto it = events_cb.find(ev.data.fd);
            if(it == nullptr){
                std::cout << "epoll cannot find event cb list for ev " << ev.data.fd << std::endl;
                continue;
            }
            EventType et = EpollEvent2Event(EventRead);
            if(it->second[et] == nullptr){
                std::cout << "epoll cannot find event cb for ev " << ev.data.fd << " event type " << et << std::endl;
                continue;
            }
            it->second[et](ev.data.fd, et);
        }
    }
    close(event_fd);
    std::cout << "epoll stop wait " << event_fd << std::endl;
    return 0;
}

EventType EpollEventHandler::EpollEvent2Event(int epollEvent)
{
    switch (epollEvent)
    {
    case EPOLLIN:
        return EventRead;
    case EPOLLOUT:
        return EventWrite;
    case EPOLLERR:
        return EventError;
    
    default:
        return EventError;
    }
}
