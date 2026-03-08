add_library(crc STATIC
    crc.c
)

target_include_directories(crc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(crc PUBLIC
    m
)
