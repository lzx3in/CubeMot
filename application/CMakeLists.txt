add_executable(${CMAKE_PROJECT_NAME}
    main.c
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    boards
    drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
)

consolidate_outputs_to_target_dir(${CMAKE_PROJECT_NAME})
