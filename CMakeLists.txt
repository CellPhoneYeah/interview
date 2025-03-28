cmake_minimum_required(VERSION 3.5)
project(interview LANGUAGES CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/objs)

set(WATCHDOG_SO watchdog)
set(SPDLOG_SO spdlog)
set(ELLNET_SO ellnet)

include_directories(${PROJECT_SOURCE_DIR}/3rd/spdlog/include)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/spdlog)

link_directories(${PROJECT_SOURCE_DIR}/objs)

add_subdirectory(./3rd)
add_subdirectory(./src/watchdog)
add_subdirectory(./src/testlib)
add_subdirectory(./src/ellnet)
add_subdirectory(./src/entrance)