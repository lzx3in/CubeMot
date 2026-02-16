set(LINKER_SCRIPT_PATH ${BOARD_DIR}/stm32cubemx_generated/STM32G431XX_FLASH.ld)
if(COMMAND set_linker_script)
    set_linker_script(${LINKER_SCRIPT_PATH})
endif()

set(BOARD_COMPILE_DEFINITIONS
    USE_HAL_DRIVER
    STM32G431xx
)
