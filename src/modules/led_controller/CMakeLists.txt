add_library(module_led_controller STATIC
    led_controller.c
)

target_include_directories(module_led_controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(module_led_controller PRIVATE
    common
    driver_led
    msghub
    freertos_kernel
    topics
)
