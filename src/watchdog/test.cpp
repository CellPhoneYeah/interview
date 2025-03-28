#include "watchdog/watchdog.h"

int main(){
    watchdog *wd = new watchdog(1);
    wd->putConn(10);
    wd->putConn(20);
    wd->putConn(30);
    watchdog::putDog(wd);
    SPDLOG_INFO("1 {}", watchdog::toString());
    wd->delConn(30);
    watchdog::delDog(1);
    SPDLOG_INFO("2 {}", watchdog::toString());
    wd->getConnections();
    SPDLOG_INFO("wd {}", "test");
    watchdog::clean();
    delete(wd);
    return 0;
}