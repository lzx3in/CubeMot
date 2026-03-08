add_library(led_driver STATIC)

target_sources(led_driver PRIVATE
    led.c
)

target_include_directories(led_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(led_driver PUBLIC
    msghub
    boards
)
