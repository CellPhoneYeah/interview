#include "watchdog/watchdog.h"

namespace ellnet
{
    int main()
    {
        WatchDog *wd = new WatchDog(1);
        wd->PutConn(10);
        wd->PutConn(20);
        wd->PutConn(30);
        WatchDog::PutDog(wd);
        SPDLOG_INFO("1 {}", WatchDog::ToString());
        wd->DelConn(30);
        WatchDog::DelDog(1);
        SPDLOG_INFO("2 {}", WatchDog::ToString());
        wd->GetConnections();
        SPDLOG_INFO("wd {}", "test");
        WatchDog::Clean();
        delete (wd);
        return 0;
    }
}
