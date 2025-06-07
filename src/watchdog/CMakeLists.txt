aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)

add_library(${WATCHDOG_A} STATIC ${SRC})

target_link_libraries(${WATCHDOG_A} spdlog)