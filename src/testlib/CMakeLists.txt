
add_executable(test_watchdog test_watchdog.cpp)

target_link_libraries(test_watchdog ${WATCHDOG_A})
target_link_libraries(test_watchdog ${SPDLOG_A})

add_executable(entrancetest entrancetest.cc)

target_link_libraries(entrancetest ${ELLNET_A})
target_link_libraries(entrancetest ${SPDLOG_A})
target_link_libraries(entrancetest ${MESSAGE_QUEUE_A})
# if(UNIX)
#     if (LIBUNWIND_INCLUDE_DIR AND LIBUNWIND_LIBRARY)
#         message(STATUS "Found libunwind: ${LIBUNWIND_LIBRARY}")
#         target_include_directories(entrancetest PRIVATE ${LIBUNWIND_INCLUDE_DIR})
#         target_link_libraries(entrancetest ${LIBUNWIND_LIBRARY})
#     else()
#         message(FATAL_ERROR "libunwind not found!")
#     endif()
# endif()