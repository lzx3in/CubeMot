add_executable(${CMAKE_PROJECT_NAME}
    main.c
    ${BOARD_INTERRUPT_SRCS}
    ${BOARD_STARTUP_SRC}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    boards
    drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
)

# Post-build: consolidate outputs to target/
consolidate_outputs_to_target_dir(${CMAKE_PROJECT_NAME})

# Programming targets: flash, erase, reset
include(${CMAKE_SOURCE_DIR}/cmake/programming.cmake)
add_programming_targets(${CMAKE_PROJECT_NAME})
