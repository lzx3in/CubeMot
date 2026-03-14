add_library(driver_button STATIC)

target_sources(driver_button PRIVATE
    button.c
)

target_include_directories(driver_button PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/src/chips/gpio/include
)

target_link_libraries(driver_button PRIVATE
    common
)
