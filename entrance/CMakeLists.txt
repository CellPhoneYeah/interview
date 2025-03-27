# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/srcs SRC)

# message("entrance ${SRC}")

add_executable(entrance ./srcs/entrance.cpp)

target_link_libraries(entrance net)
target_link_libraries(entrance watchdog)

target_include_directories(entrance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(entrance PRIVATE ${PROJECT_SOURCE_DIR}/net/include)
target_include_directories(entrance PRIVATE ${PROJECT_SOURCE_DIR}/watchdog/include)
target_include_directories(entrance PRIVATE ${PROJECT_SOURCE_DIR}/messageQueue/include)