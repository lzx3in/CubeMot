set(CUBEMX_GENERATED_DIR ${CMAKE_CURRENT_LIST_DIR}/stm32cubemx_generated)

# STM32CubeMX defines and paths
set(STM32_DEFINES
    USE_HAL_DRIVER
    STM32G431xx
    $<$<CONFIG:Debug>:DEBUG>
)

set(STM32_INCLUDE_DIRS
    ${CUBEMX_GENERATED_DIR}/Core/Inc
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Inc
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Inc/Legacy
    ${CUBEMX_GENERATED_DIR}/Drivers/CMSIS/Device/ST/STM32G4xx/Include
    ${CUBEMX_GENERATED_DIR}/Drivers/CMSIS/Include
)

# STM32CubeMX sources
set(STM32_APPLICATION_SRCS
    ${CUBEMX_GENERATED_DIR}/Core/Src/main.c
    ${CUBEMX_GENERATED_DIR}/Core/Src/stm32g4xx_it.c
    ${CUBEMX_GENERATED_DIR}/Core/Src/stm32g4xx_hal_msp.c
    ${CUBEMX_GENERATED_DIR}/Core/Src/stm32g4xx_hal_timebase_tim.c
    ${CUBEMX_GENERATED_DIR}/Core/Src/sysmem.c
    ${CUBEMX_GENERATED_DIR}/Core/Src/syscalls.c
    ${CUBEMX_GENERATED_DIR}/startup_stm32g431xx.s
)

set(STM32_HAL_SRCS
    ${CUBEMX_GENERATED_DIR}/Core/Src/system_stm32g4xx.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim_ex.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ex.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ramfunc.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_exti.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma_ex.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c
    ${CUBEMX_GENERATED_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
)

set(BOARD_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/led.c
)

# STM32 HAL interface library
add_library(stm32_hal INTERFACE)
target_include_directories(stm32_hal INTERFACE ${STM32_INCLUDE_DIRS})
target_compile_definitions(stm32_hal INTERFACE ${STM32_DEFINES})

# STM32 HAL drivers library
add_library(stm32_hal_drivers OBJECT)
target_sources(stm32_hal_drivers PRIVATE ${STM32_HAL_SRCS})
target_link_libraries(stm32_hal_drivers PUBLIC stm32_hal)

# Configure the boards library target
target_sources(boards PRIVATE
    ${BOARD_SRCS}
    ${STM32_APPLICATION_SRCS}
)

target_include_directories(boards PUBLIC
    ${CMAKE_SOURCE_DIR}
)

if(DEFINED BOARD_COMPILE_DEFINITIONS)
    target_compile_definitions(boards PUBLIC ${BOARD_COMPILE_DEFINITIONS})
endif()

# Link HAL libraries to boards
target_link_libraries(boards PUBLIC
    stm32_hal_drivers
    stm32_hal
)
