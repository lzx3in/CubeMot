# Include FreeRTOS configuration for this board
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/freertos_config)

set(CUBEMX_GENERATED_DIR ${CMAKE_CURRENT_LIST_DIR}/stm32cubemx_generated)

# STM32CubeMX sources
set(STM32_APPLICATION_SRCS
    ${CUBEMX_GENERATED_DIR}/Core/Src/main.c
    ${CUBEMX_GENERATED_DIR}/Core/Src/stm32g4xx_hal_msp.c
    ${CUBEMX_GENERATED_DIR}/Core/Src/stm32g4xx_hal_timebase_tim.c
    ${CUBEMX_GENERATED_DIR}/Core/Src/sysmem.c
    ${CUBEMX_GENERATED_DIR}/Core/Src/syscalls.c
)

set(BOARD_INTERRUPT_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/interrupt_handlers/fault_handlers.c
    ${CMAKE_CURRENT_LIST_DIR}/interrupt_handlers/exception_handlers.c
    ${CMAKE_CURRENT_LIST_DIR}/interrupt_handlers/peripheral_handlers.c
    CACHE INTERNAL "Board interrupt handlers")
set(BOARD_STARTUP_SRC "${CMAKE_CURRENT_LIST_DIR}/startup/startup_stm32g431xx.s"
    CACHE INTERNAL "Board startup file")

set(STM32_HAL_SRCS
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim_ex.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ex.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ramfunc.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_exti.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma_ex.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c
    ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
)

set(BOARD_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/led.c
    ${CMAKE_CURRENT_LIST_DIR}/init.c
    ${CMAKE_CURRENT_LIST_DIR}/delay.c
    ${CMAKE_CURRENT_LIST_DIR}/hooks.c
)

# STM32 HAL drivers library
add_library(stm32_hal_drivers STATIC)
target_sources(stm32_hal_drivers PRIVATE ${STM32_HAL_SRCS})
target_link_libraries(stm32_hal_drivers PUBLIC stm32g4_hal)

# Configure the boards library target
target_sources(boards PRIVATE
    ${BOARD_SRCS}
    ${STM32_APPLICATION_SRCS}
)

if(DEFINED BOARD_COMPILE_DEFINITIONS)
    target_compile_definitions(boards PUBLIC ${BOARD_COMPILE_DEFINITIONS})
endif()

target_link_libraries(boards PRIVATE
    stm32_hal_drivers
    freertos_kernel
)
