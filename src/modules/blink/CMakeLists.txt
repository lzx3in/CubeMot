add_library(cubemot_blink blink.c)

target_include_directories(cubemot_blink PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
