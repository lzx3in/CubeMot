add_library(drivers OBJECT)

target_sources(drivers PRIVATE
    led/led.c
)

target_include_directories(drivers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/led
)

target_include_directories(drivers PRIVATE 
    ${CMAKE_SOURCE_DIR}
)
