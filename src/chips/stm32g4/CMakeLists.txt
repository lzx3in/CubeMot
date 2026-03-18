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

add_library(chip_gpio_stm32g4 STATIC)

target_sources(chip_gpio_stm32g4 PRIVATE
    chip_gpio.c
)

target_include_directories(chip_gpio_stm32g4 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/ST/STM32G4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32G4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(chip_gpio_stm32g4 PUBLIC
    stm32g4_hal
)

target_compile_definitions(chip_gpio_stm32g4 PUBLIC
    USE_HAL_DRIVER
    STM32G431xx
)
