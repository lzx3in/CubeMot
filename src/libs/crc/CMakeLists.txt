add_library(lib_crc STATIC
    crc.c
)

target_include_directories(lib_crc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_crc PUBLIC
    m
)
