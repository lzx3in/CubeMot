target_include_directories(boards PRIVATE
    ${CMAKE_SOURCE_DIR}/src/chips/stm32g4
    ${CMAKE_SOURCE_DIR}/src/chips/stm32g4/CMSIS/Device/ST/STM32G4xx/Include
    ${CMAKE_SOURCE_DIR}/src/chips/stm32g4/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/src/chips/stm32g4/STM32G4xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/src/chips/stm32g4/led
    ${CMAKE_SOURCE_DIR}/src/chips/stm32g4/button
    ${CMAKE_SOURCE_DIR}/src/drivers/framework/include
)

set(BOARD_INTERRUPT_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/interrupt_handlers/fault_handlers.c
    ${CMAKE_CURRENT_LIST_DIR}/interrupt_handlers/exception_handlers.c
    ${CMAKE_CURRENT_LIST_DIR}/interrupt_handlers/peripheral_handlers.c
    CACHE INTERNAL "Board interrupt handlers")
set(BOARD_STARTUP_SRC "${CMAKE_CURRENT_LIST_DIR}/startup/startup_stm32g431xx.s"
    CACHE INTERNAL "Board startup file")

target_sources(boards PRIVATE
    init.c
    delay.c
    hooks.c
    sysmem.c
    syscalls.c
    msp.c
    error_handlers.c
    ${BOARD_INTERRUPT_SRCS}
    ${BOARD_STARTUP_SRC}
)
