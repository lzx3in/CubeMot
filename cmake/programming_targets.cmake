# Programming targets for embedded targets
# Provides flash, erase, and reset functionality via OpenOCD

function(add_programming_targets TARGET_NAME)
    # Get the directory where artifacts are consolidated
    set(TARGET_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/target/${BOARD}/${CMAKE_BUILD_TYPE}")

    set(OPENOCD_CFG "${TARGET_OUTPUT_DIR}/openocd.cfg")
    set(ELF_FILE "${TARGET_OUTPUT_DIR}/${TARGET_NAME}.elf")
    set(BIN_FILE "${TARGET_OUTPUT_DIR}/${TARGET_NAME}.bin")

    # Flash using ELF file (recommended - includes address info)
    add_custom_target(flash
        COMMAND openocd -f ${OPENOCD_CFG} -c "program ${ELF_FILE} verify reset exit"
        DEPENDS ${TARGET_NAME}
        COMMENT "Flashing ${TARGET_NAME}.elf to target..."
        USES_TERMINAL
    )

    # Flash using binary file (alternative)
    add_custom_target(flash-bin
        COMMAND openocd -f ${OPENOCD_CFG} -c "program ${BIN_FILE} 0x08000000 verify reset exit"
        DEPENDS ${TARGET_NAME}
        COMMENT "Flashing ${TARGET_NAME}.bin to target..."
        USES_TERMINAL
    )

    # Erase entire chip
    # Using multiple -c args instead of semicolons to avoid CMake/shell interpretation issues
    add_custom_target(flash-erase
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "mass_erase"
            -c "exit"
        COMMENT "Erasing target..."
        USES_TERMINAL
    )

    # Reset target
    add_custom_target(reset
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "reset"
            -c "exit"
        COMMENT "Resetting target..."
        USES_TERMINAL
    )
endfunction()
