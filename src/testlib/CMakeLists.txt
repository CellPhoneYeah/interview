add_executable(testwatchdog testwatchdog.cpp)

target_link_libraries(testwatchdog ${WATCHDOG_SO})
target_link_libraries(testwatchdog ${SPDLOG_SO})