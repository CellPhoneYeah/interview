#include <iostream>
#include "spdlog/slog.h"
#include "watchdog/watchdog.h"

int main()
{
    ellnet::WatchDog wd = new ellnet::WatchDog(1);
    wd.Clean();
    SPDLOG_INFO("test cmake file");
    return 0;
}
