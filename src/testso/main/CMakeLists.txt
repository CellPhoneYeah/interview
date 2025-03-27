add_executable(testso main.cpp)

target_link_libraries(testso ${MYSONAME})

message("main.." ${ROOT_INCLUDE_PATH})

target_include_directories(testso PRIVATE ${ROOT_INCLUDE_PATH})