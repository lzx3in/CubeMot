add_library(modules_blink STATIC
    blink.c
)

target_include_directories(modules_blink PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(modules_blink PUBLIC
    drivers
    freertos_kernel
)
