#include "EpollEventHandler.h"
#include <iostream>
#include <unistd.h>
#include "EpollEventContext.h"


int EpollEventHandler::start()
{
    event_fd = epoll_create1(0);
    return event_fd;
}

int EpollEventHandler::add_event(int fd, EventType et)
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
    if(events_registered.find(fd) == events_registered.end()){
        events_registered[fd] = std::vector<bool>({false, false, false});
        events_registered[fd][et] = true;
    }else{
        events_registered[fd][et] = true;
    }
    return 0;
}

void EpollEventHandler::del_event(int fd)
{
    epoll_ctl(event_fd, EPOLL_CTL_DEL, fd, nullptr);
    events_registered.erase(fd);
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
            int currentFd = ev.data.fd;
            std::unordered_map<int, std::vector<bool>>::const_iterator registered = events_registered.find(currentFd);
            if(registered == events_registered.end()){
                std::cout << "epoll cannot find event listening list for ev " << currentFd << std::endl;
                continue;
            }
            EventType et = EpollEvent2Event(ev.events);
            if(!registered->second[et]){
                std::cout << "epoll cannot find event listening for ev " << currentFd << " event type " << et << std::endl;
                continue;
            }
            handle_event(currentFd, ev);
        }
    }
    close(event_fd);
    std::cout << "epoll stop wait " << event_fd << std::endl;
    return 0;
}

EventType EpollEventHandler::EpollEvent2Event(int epollEvent)
{
    if(epollEvent & EPOLLIN){
        return EventRead;
    }
    if(epollEvent & EPOLLOUT){
        return EventWrite;
    }
    return EventError;
}

void EpollEventHandler::handle_event(int fd, epoll_event &ev)
{
    EpollEventContext* eec = (EpollEventContext*)ev.data.ptr;
    eec->handle_event(&ev);
}
