#include <iostream>
#include "slog.h"
#include "watchdog.h"

int main (){
    watchdog wd = new watchdog(1);
    wd.clean();
    SPDLOG_INFO("test cmake file");
    return 0;
}