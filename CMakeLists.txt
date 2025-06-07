cmake_minimum_required(VERSION 3.5)
project(interview LANGUAGES CXX)

# 设置 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置 Objective-C++ 使用 C++20
set(CMAKE_OBJCXX_STANDARD 20)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)

# 启用异常处理
# add_compile_options(-fexceptions)
# add_link_options(-fexceptions)
set(CMAKE_EXE_LINKER_FLAGS "-static")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/objs)

set(CMAKE_BUILD_TYPE Debug)

# 启用调试符号和 -rdynamic

# if(LINUX)
    message(STATUS "Configuring for UNIX-like system")
    add_compile_options(
        -g 
        -rdynamic
        -O0
        -Wall
    )
    # find_path(LIBUNWIND_INCLUDE_DIR libunwind.h)
    # find_library(LIBUNWIND_LIBRARY unwind)
# endif()

set(WATCHDOG_A watchdog)
set(SPDLOG_A spdlog)
set(ELLNET_A ellnet)
set(SIGNAL_HANDLER_A signal_handler_a)
set(MESSAGE_QUEUE_A mq_a)

include_directories(${PROJECT_SOURCE_DIR}/3rd/spdlog/include)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/spdlog)
include_directories(${PROJECT_SOURCE_DIR}/include/message_queue)
include_directories(${PROJECT_SOURCE_DIR}/include/ellnet)
include_directories(${PROJECT_SOURCE_DIR}/include/common)

link_directories(${PROJECT_SOURCE_DIR}/objs)

add_subdirectory(./3rd)
# add_subdirectory(./src/common)
add_subdirectory(./src/watchdog)
add_subdirectory(./src/ellnet)
add_subdirectory(./src/testlib)
add_subdirectory(./src/message_queue)
add_subdirectory(./src/entrance)
# add_subdirectory(./src/tests)