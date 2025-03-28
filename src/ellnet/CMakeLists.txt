aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/objs)

add_library(${ELLNET_SO} SHARED ${SRC})

target_include_directories(${ELLNET_SO} PRIVATE ${PROJECT_SOURCE_DIR}/include/ellnet)