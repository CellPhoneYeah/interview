aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)

message("message_queue src:${SRC}")

add_library(${MESSAGE_QUEUE_A} STATIC ${SRC})

# target_include_directories(${MESSAGE_QUEUE_A} PRIVATE ${PROJECT_SOURCE_DIR}/include/message_queue)