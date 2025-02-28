#include "NetServerFactory.h"
#include "EpollEventHandler.h"
#include "EventContext.h"

EventHandler *NetServerFactory::createEpollEventHandler()
{
    EpollEventHandler *eeh = new EpollEventHandler();
    int epollfd = eeh->start();
    eeh->add_event(epollfd, EventRead, EventHandler::AcceptCallback);
    return nullptr;
}


static void handleAcceptCallback(int fd, EventType et, EventContext &ec){
    return;
}

void NetServerFactory::handleAcceptEvent(int fd, EventType et)
{
}
