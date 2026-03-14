# Chips Directory
# Chip-specific implementations and abstractions

# STM32G4 chip-specific implementations
add_subdirectory(stm32g4)

# Aggregate STATIC library for all chip-level code
add_library(chips STATIC)

target_sources(chips PRIVATE
    chips.c
)

target_include_directories(chips PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio/include
)

target_link_libraries(chips PUBLIC
    stm32g4_hal
    chip_gpio_stm32g4
)
