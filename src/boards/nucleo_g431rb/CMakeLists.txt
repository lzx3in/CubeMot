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
    led.c
    init.c
    delay.c
    hooks.c
    sysmem.c
    syscalls.c
    msp.c
)

if(DEFINED BOARD_COMPILE_DEFINITIONS)
    target_compile_definitions(boards PUBLIC ${BOARD_COMPILE_DEFINITIONS})
endif()
