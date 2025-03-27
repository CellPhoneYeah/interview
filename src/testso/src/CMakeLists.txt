aux_source_directory(./ SRC)

add_library(${MYSONAME} SHARED ${SRC})

message("src.." ${ROOT_INCLUDE_PATH})

target_include_directories(${MYSONAME} PRIVATE ${ROOT_INCLUDE_PATH})