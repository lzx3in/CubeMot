# Common - Shared definitions and utilities for all layers
# This includes error codes, common types, and utility functions

add_library(common STATIC
    common_time.c
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(common PUBLIC
    freertos_kernel
)
