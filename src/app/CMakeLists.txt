add_executable(${CMAKE_PROJECT_NAME}
    main.c
    app_main.c
    app_init.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    boards
    modules
    freertos_kernel
    ${TOOLCHAIN_LINK_LIBRARIES}
)

consolidate_outputs_to_target_dir(${CMAKE_PROJECT_NAME})
