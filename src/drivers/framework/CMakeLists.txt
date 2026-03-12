# Driver Framework Library
# Provides HAL abstraction and registration mechanisms

add_library(driver_framework STATIC)

target_sources(driver_framework PRIVATE
    hal_framework.c
    led_hal_framework.c
    button_hal_framework.c
)

target_include_directories(driver_framework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/common/include
)

# Framework should not depend on any HAL implementation
# Only basic system headers
target_link_libraries(driver_framework PUBLIC
    # No hardware dependencies!
)
