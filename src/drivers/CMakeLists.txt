add_library(drivers INTERFACE)

add_subdirectory(led)
target_link_libraries(drivers INTERFACE
    driver_led
)

add_subdirectory(button)
target_link_libraries(drivers INTERFACE
    driver_button
)
