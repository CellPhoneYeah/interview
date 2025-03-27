aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/srcs SRC)

# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./libs)

add_library(${WATCHDOG_SO} SHARED ${SRC})

message(${SRC} ":watchdog/project source dir:" ${PROJECT_SOURCE_DIR} "cmake src dir:" ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(${WATCHDOG_SO} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(${WATCHDOG_SO} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${WATCHDOG_SO} spdlog)