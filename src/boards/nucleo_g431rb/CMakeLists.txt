# Include FreeRTOS configuration for this board
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/freertos_config)

set(CUBEMX_GENERATED_DIR ${CMAKE_CURRENT_LIST_DIR}/stm32cubemx_generated)

set(BOARD_INTERRUPT_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/interrupt_handlers/fault_handlers.c
    ${CMAKE_CURRENT_LIST_DIR}/interrupt_handlers/exception_handlers.c
    ${CMAKE_CURRENT_LIST_DIR}/interrupt_handlers/peripheral_handlers.c
    CACHE INTERNAL "Board interrupt handlers")
set(BOARD_STARTUP_SRC "${CMAKE_CURRENT_LIST_DIR}/startup/startup_stm32g431xx.s"
    CACHE INTERNAL "Board startup file")

# Configure the boards library target
target_sources(boards PRIVATE
    init.c
    delay.c
    hooks.c
    sysmem.c
    syscalls.c
    msp.c
)

add_executable(${CMAKE_PROJECT_NAME}
    main.c
    ${BOARD_INTERRUPT_SRCS}
    ${BOARD_STARTUP_SRC}
    error_handlers.c
)

# Linking order:
# - stm32g4_hal: Vendor HAL (ST's code)
# - hal_stm32g4_impl: Our HAL implementation using framework
# - boards: Board code (calls stm32g4_led_hal_init)
# Framework ensures correct initialization order through registration
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    stm32g4_hal
    freertos_kernel
    app
    msghub
    modules
    hal_stm32g4_impl  # HAL implementations (must be before boards)
    boards
    ${TOOLCHAIN_LINK_LIBRARIES}
)

# Add HAL include path for board code
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/led
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    ${BOARD_COMPILE_DEFINITIONS}
)

consolidate_outputs_to_target_dir(${CMAKE_PROJECT_NAME})

# Programming targets
include(${CMAKE_SOURCE_DIR}/cmake/programming_targets.cmake)
add_programming_targets(${CMAKE_PROJECT_NAME})

# Debugging targets
include(${CMAKE_SOURCE_DIR}/cmake/debugging_targets.cmake)
add_debugging_targets(${CMAKE_PROJECT_NAME})
