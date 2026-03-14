add_library(module_blink STATIC
    blink.c
)

target_include_directories(module_blink PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(module_blink PRIVATE
    common
    msghub
    freertos_kernel
    topics
)
