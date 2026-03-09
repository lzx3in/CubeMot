# Common - Shared definitions and utilities for all layers
# This includes error codes, common types, and utility functions

add_library(common INTERFACE)

target_include_directories(common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
