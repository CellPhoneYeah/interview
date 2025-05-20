cmake_minimum_required(VERSION 3.5)
project(interview LANGUAGES CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/objs)

set(CMAKE_BUILD_TYPE Debug)

# 启用调试符号和 -rdynamic

if(UNIX)
    add_compile_options(-g -rdynamic)
    find_path(LIBUNWIND_INCLUDE_DIR libunwind.h)
    find_library(LIBUNWIND_LIBRARY unwind)
    
endif()

set(WATCHDOG_SO watchdog)
set(SPDLOG_SO spdlog)
set(ELLNET_SO ellnet)
set(MESSAGE_QUEUE_A mq_a)

include_directories(${PROJECT_SOURCE_DIR}/3rd/spdlog/include)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/spdlog)
include_directories(${PROJECT_SOURCE_DIR}/include/message_queue)
include_directories(${PROJECT_SOURCE_DIR}/include/ellnet)
include_directories(${PROJECT_SOURCE_DIR}/include/common)

link_directories(${PROJECT_SOURCE_DIR}/objs)

add_subdirectory(./3rd)
add_subdirectory(./src/watchdog)
add_subdirectory(./src/ellnet)
add_subdirectory(./src/testlib)
add_subdirectory(./src/message_queue)
add_subdirectory(./src/entrance)