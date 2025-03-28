#include <iostream>
#include "spdlog/slog.h"
#include "watchdog/watchdog.h"

int main (){
    watchdog wd = new watchdog(1);
    wd.clean();
    SPDLOG_INFO("test cmake file");
    return 0;
}