add_library(driver_led STATIC)

target_sources(driver_led PRIVATE
    led.c
    led_topic.c
)

target_include_directories(driver_led PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(driver_led PUBLIC
    msghub
    driver_framework  # Use HAL framework interface
)

# Driver layer depends only on framework, not on HAL implementations
