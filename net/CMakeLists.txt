aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/srcs SRC)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs)

add_library(net SHARED ${SRC})

target_include_directories(net PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(net PRIVATE ${PROJECT_SOURCE_DIR}/include/spdlog)
target_include_directories(net PRIVATE ${PROJECT_SOURCE_DIR}/3rd/spdlog/include)