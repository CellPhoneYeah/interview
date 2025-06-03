add_executable(signal_handler_test signal_handler_test.cpp)
message("make signal handler test finished=====!")

set(CMAKE_BUILD_TYPE Debug)  # 确保为 Debug 构建

target_link_libraries(signal_handler_test ${SIGNAL_HANDLER_A})
target_compile_options(signal_handler_test PRIVATE
    -g          # 生成调试信息
    -O0         # 不优化
    -Wall       # 启用所有警告
    -Wextra     # 启用额外的警告
    -rdynamic   # 在动态符号表中添加所有符号
    -fno-omit-frame-pointer # 保留帧指针
)
message("make signal handler test finished=====!")