cmake_minimum_required(VERSION 3.5)
project(interview LANGUAGES CXX)

set(PROTOBUF_VERSION "3.23.4")

set(3RD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd)
set(3RD_PROTOBUF_PATH ${3RD_PATH}/protobuf)
message(STATUS "start download protobuf to ${3RD_PROTOBUF_PATH}")
include(FetchContent)
FetchContent_Declare(
  protobuf
  GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf.git"
  GIT_TAG        "v3.23.4"
  SOURCE_DIR     "${3RD_PROTOBUF_PATH}"
)
FetchContent_MakeAvailable(protobuf)

# 定义关键路径变量
set(PROTOBUF_ROOT "${3RD_PROTOBUF_PATH}")
set(PROTOC_EXECUTABLE "${3RD_PROTOBUF_PATH}/bin/protoc")

# option(BUILD_SERVER "Build server" ON)
# option(BUILD_CLIENT "Build client" OFF)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NET_DIR ${SRC_ROOT}/net)
set(SERVER_DIR ${SRC_ROOT})
set(CLIENT_DIR ${SRC_ROOT})
set(PROTO_DIR ${SRC_ROOT}/proto)
set(COMMON_DIR ${SRC_ROOT}/common)
set(PROTO_GEN_DIR ${PROTO_DIR}/generated)

set(NET_SOURCES ${NET_DIR}/*.cpp)
set(SERVER_SOURCES ${SERVER_DIR}/server.cpp)
set(CLIENT_SOURCES ${CLIENT_DIR}/client.cpp)

find_package(Protobuf REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PROTOBUF_INCLUDE_DIR})

file(GLOB_RECURSE PROTO_FILES "${PROTO_DIR}/*.proto")

message(STATUS "PROTO_FILES ${PROTO_FILES}")
message(STATUS "PROTO_GEN_DIR ${PROTO_GEN_DIR}")
message(STATUS "Generated source files: ${PROTO_SRCS}")
message(STATUS "Generated header files: ${PROTO_HDRS}")
file(MAKE_DIRECTORY "${PROTO_GEN_DIR}")
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${PROTO_FILES})

add_library(proto ${PROTO_HEADER} ${PROTO_SRC})
add_executable(proto_test proto_test.cpp)
target_link_libraries(proto_test proto ${PROTOBUF_LIBRARY})

# include_directories(
#     ${COMMON_DIR}
#     ${NET_DIR}
# )

# file(GLOB SERVER_FILES 
# ${SERVER_SOURCES}
# ${NET_SOURCES}
# )

# file(GLOB CLIENT_FILES 
# ${CLIENT_SOURCES}
# ${NET_SOURCES}
# )

# if(BUILD_SERVER)
#     add_executable(
#         server
#         ${SERVER_FILES}
#     )
# endif()

# if(BUILD_CLIENT)
#     add_executable(
#         client
#         ${CLIENT_FILES}
#     )
# endif()