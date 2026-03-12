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

add_executable(${CMAKE_PROJECT_NAME}
    main.c
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    app
    boards
)

consolidate_outputs_to_target_dir(${CMAKE_PROJECT_NAME})
