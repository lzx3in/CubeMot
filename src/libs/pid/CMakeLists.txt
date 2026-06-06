add_library(pid pid.c)

target_include_directories(pid PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
