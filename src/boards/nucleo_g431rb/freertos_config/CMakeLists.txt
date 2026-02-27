# FreeRTOS Configuration for nucleo_g431rb
# This defines the freertos_config target required by FreeRTOS Kernel CMake

add_library(freertos_config INTERFACE)

target_include_directories(freertos_config SYSTEM
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_definitions(freertos_config
    INTERFACE
        projCOVERAGE_TEST=0
)

# Board-specific FreeRTOS settings can be added here if needed
# For example: board-specific compile definitions or options
