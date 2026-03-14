add_library(module_blink STATIC
    blink.c
)

target_include_directories(module_blink PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(module_blink PUBLIC
    drivers
    freertos_kernel
    topics
)
