cmake_minimum_required(VERSION 3.5)
project(interview LANGUAGES CXX)

message("start cmake")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs)

set(WATCHDOG_SO watchdog)
message("output path" ${EXECUTABLE_OUTPUT_PATH})
message("output lib" ${CMAKE_LIBRARY_PATH})

include_directories(${PROJECT_SOURCE_DIR}/include/spdlog)

link_directories(${PROJECT_SOURCE_DIR}/libs)

add_subdirectory(./3rd)
add_subdirectory(./watchdog)
add_subdirectory(./testlib)
add_subdirectory(./net)
add_subdirectory(./entrance)