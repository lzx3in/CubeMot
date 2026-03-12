add_library(driver_button STATIC)

target_sources(driver_button PRIVATE
    button.c
    button_topic.c
)

target_include_directories(driver_button PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(driver_button PUBLIC
    msghub
    driver_framework  # Use HAL framework interface
)

# Driver layer depends only on framework, not on HAL implementations
