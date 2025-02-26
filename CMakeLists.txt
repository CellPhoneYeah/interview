cmake_minimum_required(VERSION 3.5)
project(interview LANGUAGES CXX)

set(PROTOBUF_VERSION "3.23.4")

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

file(GLOB_RECURSE PROTO_FILES
        "${PROTO_DIR}/*.proto"
        "${PROTO_DIR}/**/*.proto"
)

file(MAKE_DIRECTORY "${PROTO_GEN_DIR}")
# 添加可执行文件
add_executable(proto_test
        proto_test.cpp
        ${PROTO_FILES}
)

protobuf_generate(
        TARGET proto_test
        LANGUAGE cpp
        PROTOC_OUT_DIR "${PROTO_GEN_DIR}"
        PROTOC_OPTIONS
        "--proto_path=${PROTO_DIR}"
        "--proto_path=${PROTO_GEN_DIR}"
        GENERATE_EXTENSIONS .pb.h .pb.cc
        FILES ${PROTO_FILES}
)

# 包含生成目录（处理嵌套目录结构）
target_include_directories(proto_test PRIVATE
        "${PROTO_GEN_DIR}"
)
message(VERBOSE "include directories ${PROTO_GEN_DIR}")

target_link_libraries(proto_test PRIVATE
        protobuf::libprotobuf
)

message(VERBOSE "Proto files: ${PROTO_FILES}")
get_target_property(GEN_SOURCES proto_test PROTOC_SOURCES)
message(VERBOSE "Generated sources: ${GEN_SOURCES}")
# 显示生成的文件路径（调试用）
message(STATUS "Proto headers: ${PROTO_HDRS}")
message(STATUS "Proto sources: ${PROTO_SRCS}")

include_directories(
        ${COMMON_DIR}
        ${NET_DIR}
)

file(GLOB SERVER_FILES
        ${SERVER_SOURCES}
        ${NET_SOURCES}
)

file(GLOB CLIENT_FILES
        ${CLIENT_SOURCES}
        ${NET_SOURCES}
)

add_executable(
        server
        ${SERVER_FILES}
)

add_executable(
        client
        ${CLIENT_FILES}
        player/player.cpp
        player/player.h
)