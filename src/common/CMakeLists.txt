aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/signal SRC)
set(CMAKE_BUILD_TYPE Debug)  # 确保为 Debug 构建
if(NOT SRC)
    message(FATAL_ERROR "No source files found in directory: ${CMAKE_CURRENT_SOURCE_DIR}/signal")
endif()

find_library(DW_LIB dw)

message("Found libdw: ${DW_LIB}")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/objs)
message("target static a ${SIGNAL_HANDLER_A} sources=======: ${SRC} ouput: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

add_library(${SIGNAL_HANDLER_A} STATIC ${SRC})

target_include_directories(${SIGNAL_HANDLER_A} PUBLIC 
    ${PROJECT_SOURCE_DIR}/include/common/signal
)

# 链接系统库
target_link_libraries(${SIGNAL_HANDLER_A} PRIVATE 
    dl
    ${DW_LIB}
)

# 添加编译选项
target_compile_options(${SIGNAL_HANDLER_A} PRIVATE
    -Wall
    -Wextra
    -rdynamic
    -g
    -O0
)
