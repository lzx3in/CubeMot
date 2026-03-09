add_library(stm32g4_hal STATIC)

target_compile_definitions(stm32g4_hal PUBLIC
    USE_HAL_DRIVER
    STM32G431xx
    $<$<CONFIG:Debug>:DEBUG>
)

target_sources(stm32g4_hal PRIVATE
    system_stm32g4xx.c
)

target_include_directories(stm32g4_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/ST/STM32G4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32G4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32G4xx_HAL_Driver/Inc/Legacy
)

target_link_libraries(stm32g4_hal PUBLIC

)
