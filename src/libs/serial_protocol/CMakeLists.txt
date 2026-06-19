add_library(lib_serial_protocol serial_protocol.c)

target_include_directories(lib_serial_protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_serial_protocol PUBLIC lib_crc)
