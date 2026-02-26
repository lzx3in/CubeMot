# Programming targets for embedded devices
# Flash programming, verification, erase, and static analysis

function(add_programming_targets TARGET_NAME)
    # Get the directory where artifacts are consolidated
    set(TARGET_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/target/${BOARD}/${CMAKE_BUILD_TYPE}")

    set(OPENOCD_CFG "${TARGET_OUTPUT_DIR}/openocd.cfg")
    set(ELF_FILE "${TARGET_OUTPUT_DIR}/${TARGET_NAME}.elf")
    set(BIN_FILE "${TARGET_OUTPUT_DIR}/${TARGET_NAME}.bin")

    #=========================================================================
    # Configuration (can be overridden via -D or environment)
    #=========================================================================

    # Flash size in bytes (default: 128KB for G431)
    if(NOT DEFINED FLASH_SIZE)
        set(FLASH_SIZE "131072" CACHE STRING "Flash size in bytes")
    endif()
    if(NOT DEFINED FLASH_ERASE_START)
        set(FLASH_ERASE_START "0" CACHE STRING "First sector to erase")
    endif()
    if(NOT DEFINED FLASH_ERASE_END)
        set(FLASH_ERASE_END "31" CACHE STRING "Last sector to erase")
    endif()

    #=========================================================================
    # Flash Programming
    #=========================================================================

    # Flash using ELF file (recommended - includes address info)
    add_custom_target(flash
        COMMAND openocd -f ${OPENOCD_CFG} -c "program ${ELF_FILE} verify reset exit"
        DEPENDS ${TARGET_NAME}
        COMMENT "Flashing ${TARGET_NAME}.elf to target..."
        USES_TERMINAL
    )

    # Flash using binary file (alternative - requires explicit address)
    add_custom_target(flash-bin
        COMMAND openocd -f ${OPENOCD_CFG} -c "program ${BIN_FILE} 0x08000000 verify reset exit"
        DEPENDS ${TARGET_NAME}
        COMMENT "Flashing ${TARGET_NAME}.bin to target..."
        USES_TERMINAL
    )

    #=========================================================================
    # Flash Verification
    #=========================================================================

    # Verify Flash content matches ELF
    add_custom_target(flash-verify
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "verify_image ${ELF_FILE}"
            -c "exit"
        DEPENDS ${TARGET_NAME}
        COMMENT "Verifying Flash content..."
        USES_TERMINAL
    )

    # Compare Flash content with ELF (detailed diff)
    add_custom_target(flash-compare
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "dump_image ${TARGET_OUTPUT_DIR}/flash_compare.bin 0x08000000 ${FLASH_SIZE}"
            -c "exit"
        COMMAND ${CMAKE_COMMAND} -E echo "Comparing Flash with ${ELF_FILE}..."
        COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_FILE} ${TARGET_OUTPUT_DIR}/elf_compare.bin
        COMMAND diff -q ${TARGET_OUTPUT_DIR}/flash_compare.bin ${TARGET_OUTPUT_DIR}/elf_compare.bin && ${CMAKE_COMMAND} -E echo "Files are identical" || ${CMAKE_COMMAND} -E echo "Files differ"
        DEPENDS ${TARGET_NAME}
        COMMENT "Comparing Flash content with ELF..."
        USES_TERMINAL
    )

    #=========================================================================
    # Flash Erase
    #=========================================================================

    # Erase entire chip
    add_custom_target(flash-erase
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "mass_erase"
            -c "exit"
        COMMENT "Erasing entire Flash..."
        USES_TERMINAL
    )

    # Erase specified sector range (parameterized)
    # Usage: cmake --build build --target flash-erase-sectors -DFLASH_ERASE_START=0 -DFLASH_ERASE_END=15
    add_custom_target(flash-erase-sectors
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "flash erase_sector 0 ${FLASH_ERASE_START} ${FLASH_ERASE_END}"
            -c "exit"
        COMMENT "Erasing Flash sectors ${FLASH_ERASE_START}-${FLASH_ERASE_END}..."
        USES_TERMINAL
    )

    #=========================================================================
    # Flash Read / Backup
    #=========================================================================

    # Read entire Flash to binary file (parameterized size)
    # Usage: cmake --build build --target flash-read -DFLASH_SIZE=262144
    add_custom_target(flash-read
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "flash read_bank 0 ${TARGET_OUTPUT_DIR}/flash_dump.bin 0 ${FLASH_SIZE}"
            -c "exit"
        COMMENT "Reading ${FLASH_SIZE} bytes Flash to flash_dump.bin..."
        USES_TERMINAL
    )

    # Backup Flash before programming (safety target)
    add_custom_target(flash-backup
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "flash read_bank 0 ${TARGET_OUTPUT_DIR}/flash_backup_$$(date +%Y%m%d_%H%M%S).bin 0 ${FLASH_SIZE}"
            -c "exit"
        COMMENT "Creating Flash backup (${FLASH_SIZE} bytes)..."
        USES_TERMINAL
    )

    #=========================================================================
    # Static Analysis (no hardware required)
    #=========================================================================

    # Display ELF size summary
    add_custom_target(show-size
        COMMAND ${CMAKE_SIZE} -B ${ELF_FILE}
        DEPENDS ${TARGET_NAME}
        COMMENT "Memory usage for ${TARGET_NAME}:"
    )

    # Display detailed ELF size
    add_custom_target(show-size-detailed
        COMMAND ${CMAKE_SIZE} -A ${ELF_FILE}
        DEPENDS ${TARGET_NAME}
        COMMENT "Detailed memory usage:"
    )

    # Display symbol table (sorted by size)
    add_custom_target(show-symbols
        COMMAND ${CMAKE_NM} --print-size --size-sort --reverse-sort ${ELF_FILE} | head -50
        DEPENDS ${TARGET_NAME}
        COMMENT "Top 50 symbols by size:"
    )

endfunction()
