add_executable(testwatchdog testwatchdog.cpp)

target_include_directories(testwatchdog PRIVATE ${PROJECT_SOURCE_DIR}/include/spdlog)
target_include_directories(testwatchdog PRIVATE ${PROJECT_SOURCE_DIR})
message("testlib proj src dir:${PROJECT_SOURCE_DIR}")

target_link_libraries(testwatchdog ${WATCHDOG_SO})