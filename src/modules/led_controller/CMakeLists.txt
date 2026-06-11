add_library(cubemot_led_controller led_controller.c)

target_include_directories(cubemot_led_controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
