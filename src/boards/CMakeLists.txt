add_library(boards OBJECT)
target_include_directories(boards PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/${BOARD})
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/${BOARD}/freertos_config)

target_link_libraries(boards PRIVATE
    stm32g4_hal
    hal_stm32g4_impl
    freertos_kernel
    driver_button
    msghub
)
