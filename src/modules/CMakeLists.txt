add_subdirectory(led_controller)
add_subdirectory(button_detector)
add_subdirectory(blink)

add_library(modules INTERFACE)
target_link_libraries(modules INTERFACE
    module_led_controller
    module_button_detector
    module_blink
)
