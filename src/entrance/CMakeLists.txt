# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/srcs SRC)

# message("entrance ${SRC}")

add_executable(entrance entrance.cpp)

target_link_libraries(entrance ${ELLNET_SO})
target_link_libraries(entrance ${WATCHDOG_SO})