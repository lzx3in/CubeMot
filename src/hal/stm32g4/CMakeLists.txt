# STM32G4 HAL Implementation
# This has two parts:
# 1. stm32g4_hal - Vendor HAL library (ST's code)
# 2. hal_stm32g4 - Our HAL implementation using the framework

# ============================================================================
# Part 1: Vendor HAL Library (ST's code)
# ============================================================================
add_library(stm32g4_hal STATIC)

target_sources(stm32g4_hal PRIVATE
    system_stm32g4xx.c
    stm32g4xx_hal_timebase_tim.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim_ex.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ex.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ramfunc.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_exti.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma_ex.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c
    STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
)

target_compile_definitions(stm32g4_hal PUBLIC
    USE_HAL_DRIVER
    STM32G431xx
    $<$<CONFIG:Debug>:DEBUG>
)

target_include_directories(stm32g4_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/ST/STM32G4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32G4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32G4xx_HAL_Driver/Inc/Legacy
)

# ============================================================================
# Part 2: Our HAL Implementation (using framework)
# ============================================================================
add_library(hal_stm32g4_impl STATIC)

target_sources(hal_stm32g4_impl PRIVATE
    led/led_hal_stm32g4.c
)

target_include_directories(hal_stm32g4_impl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/ST/STM32G4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32G4xx_HAL_Driver/Inc
)

target_include_directories(hal_stm32g4_impl PRIVATE
    ${CMAKE_SOURCE_DIR}/src/drivers/framework/include
)

target_link_libraries(hal_stm32g4_impl PUBLIC
    driver_framework
    stm32g4_hal
)

target_compile_definitions(hal_stm32g4_impl PUBLIC
    USE_HAL_DRIVER
    STM32G431xx
)
