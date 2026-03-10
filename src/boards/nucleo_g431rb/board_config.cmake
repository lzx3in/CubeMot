set(LINKER_SCRIPT_PATH ${BOARD_DIR}/linker/STM32G431XX_FLASH.ld)
if(COMMAND set_linker_script)
    set_linker_script(${LINKER_SCRIPT_PATH})
endif()
