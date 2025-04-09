add_executable(entrance entrance.cpp)

target_link_libraries(entrance ${MESSAGE_QUEUE_A})
target_link_libraries(entrance ${ELLNET_SO})
target_link_libraries(entrance ${WATCHDOG_SO})