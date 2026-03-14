add_library(module_button_detector STATIC
    button_detector.c
)

target_include_directories(module_button_detector PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(module_button_detector PRIVATE
    common
    msghub
    freertos_kernel
    topics
    driver_button
)
