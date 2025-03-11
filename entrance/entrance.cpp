#include "slog.h"
#include "EpollNet.h"
#include "watchdog.h"
#include "WatchDogConnectHandler.h"

int main(){
    watchdog *wd = new watchdog(1);
    watchdog::putDog(wd);
    EpollNet *eNet = EpollNet::getInstance();
    eNet->setConnectHandler(new WatchDogConnectHandler());
    eNet->listenOn("127.0.0.1", 8088);
    return 0;
}