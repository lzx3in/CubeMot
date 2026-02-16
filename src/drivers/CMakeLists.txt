add_library(drivers OBJECT)

target_sources(drivers PRIVATE
    led/led.c
)

target_include_directories(drivers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(drivers PRIVATE
    boards
)
