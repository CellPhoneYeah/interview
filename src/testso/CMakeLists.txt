cmake_minimum_required(VERSION 3.2)

project(testso)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs)

set(MYSONAME add)

set(ROOT_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/include)

link_directories(${PROJECT_SOURCE_DIR}/libs)

add_subdirectory(src)

add_subdirectory(main)