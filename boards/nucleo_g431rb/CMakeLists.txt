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

add_library(nucleo_g431rb OBJECT)
target_sources(nucleo_g431rb PRIVATE ${BOARD_SRCS})

# STM32 application sources
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${STM32_APPLICATION_SRCS})

# STM32 HAL interface library
add_library(stm32_hal INTERFACE)
target_include_directories(stm32_hal INTERFACE ${STM32_INCLUDE_DIRS})
target_compile_definitions(stm32_hal INTERFACE ${STM32_DEFINES})

# STM32 HAL drivers library
add_library(stm32_hal_drivers OBJECT)
target_sources(stm32_hal_drivers PRIVATE ${STM32_HAL_SRCS})
target_link_libraries(stm32_hal_drivers PUBLIC stm32_hal)

target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE)

# Link HAL libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    stm32_hal_drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
)

# Link board library
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE nucleo_g431rb)

# Board configuration includes and definitions
target_include_directories(nucleo_g431rb PRIVATE ${CMAKE_SOURCE_DIR})
if(DEFINED BOARD_INCLUDE_DIRS)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${BOARD_INCLUDE_DIRS})
    target_include_directories(nucleo_g431rb PRIVATE ${BOARD_INCLUDE_DIRS})
endif()

if(DEFINED BOARD_COMPILE_DEFINITIONS)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ${BOARD_COMPILE_DEFINITIONS})
    target_compile_definitions(nucleo_g431rb PRIVATE ${BOARD_COMPILE_DEFINITIONS})
endif()

# Link with HAL
target_link_libraries(nucleo_g431rb PUBLIC stm32_hal)
