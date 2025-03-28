set(SPDLOG_BUILD_SHARED true)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/objs)

include_directories(${SPDLOG_SO} ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include)

add_subdirectory(${SPDLOG_SO})
