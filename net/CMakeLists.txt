aux_source_directory(./srcs SRC)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs)

add_library(net ${SRC})

target_include_directories(net PRIVATE ./include)