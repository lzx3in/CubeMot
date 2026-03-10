add_library(drivers INTERFACE)

# Framework layer must be added first (provides HAL interfaces)
add_subdirectory(framework)

# Individual drivers
add_subdirectory(led)
target_link_libraries(drivers INTERFACE
    driver_framework
    driver_led
)
