add_library(drivers INTERFACE)

# Framework layer must be added first (provides HAL interfaces)
add_subdirectory(framework)

# Individual drivers
add_subdirectory(led)
add_subdirectory(button)
target_link_libraries(drivers INTERFACE
    driver_framework
    driver_led
    driver_button
)
