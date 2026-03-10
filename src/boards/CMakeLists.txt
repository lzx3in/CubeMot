# In test mode, create an INTERFACE library (header-only) to provide include paths
# In embedded mode, create a STATIC library with full implementation
if(BUILD_TESTING)
    add_library(boards INTERFACE)
    target_include_directories(boards INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}
    )
else()
    add_library(boards STATIC)
    target_include_directories(boards PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}
    )
    # Include HAL headers for board implementation
    target_include_directories(boards PRIVATE
        ${CMAKE_SOURCE_DIR}/src/hal/stm32g4
        ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/CMSIS/Device/ST/STM32G4xx/Include
        ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/CMSIS/Include
        ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/STM32G4xx_HAL_Driver/Inc
        ${CMAKE_SOURCE_DIR}/src/hal/stm32g4/led
        ${CMAKE_SOURCE_DIR}/src/drivers/framework/include
    )
    target_compile_definitions(boards PRIVATE
        USE_HAL_DRIVER
        STM32G431xx
    )
    # Only add board-specific implementation in non-test mode
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/${BOARD})
    target_link_libraries(boards PUBLIC
        hal_stm32g4_impl  # HAL implementations (PUBLIC ensures correct link order)
    )
    target_link_libraries(boards PRIVATE
        stm32g4_hal
        freertos_kernel
    )
endif()
