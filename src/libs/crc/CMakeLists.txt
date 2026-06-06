add_library(lib_crc crc.c)

target_include_directories(lib_crc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
