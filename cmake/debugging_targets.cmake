# Debugging targets for embedded targets
# GDB server, run control, and target inspection

function(add_debugging_targets TARGET_NAME)
    # Get the directory where artifacts are consolidated
    set(TARGET_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/target/${BOARD}/${CMAKE_BUILD_TYPE}")

    set(OPENOCD_CFG "${TARGET_OUTPUT_DIR}/openocd.cfg")
    set(ELF_FILE "${TARGET_OUTPUT_DIR}/${TARGET_NAME}.elf")

    # GDB path (from ARM toolchain)
    set(GDB "${TOOLCHAIN_PREFIX}gdb")

    #=========================================================================
    # Configuration (can be overridden via -D or environment)
    #=========================================================================

    # Default memory read/write parameters
    if(NOT DEFINED MEM_ADDR)
        set(MEM_ADDR "0x08000000" CACHE STRING "Memory address for read/write operations")
    endif()
    if(NOT DEFINED MEM_LEN)
        set(MEM_LEN "256" CACHE STRING "Memory length for read operations (bytes)")
    endif()

    #=========================================================================
    # Target Reset
    #=========================================================================

    # Reset target and let it run
    add_custom_target(target-reset
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "reset"
            -c "exit"
        COMMENT "Resetting target..."
        USES_TERMINAL
    )

    # Reset and halt (for debugging entry point)
    add_custom_target(target-reset-halt
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "reset halt"
            -c "exit"
        COMMENT "Resetting and halting target..."
        USES_TERMINAL
    )

    #=========================================================================
    # Run Control (Halt/Resume)
    #=========================================================================

    # Halt target execution
    add_custom_target(target-halt
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "exit"
        COMMENT "Halting target..."
        USES_TERMINAL
    )

    # Resume target execution
    add_custom_target(target-resume
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "resume"
            -c "exit"
        COMMENT "Resuming target..."
        USES_TERMINAL
    )

    #=========================================================================
    # GDB Server
    #=========================================================================

    # Start GDB server (blocks until Ctrl-C)
    add_custom_target(gdb-server
        COMMAND openocd -f ${OPENOCD_CFG}
        COMMENT "Starting GDB server on port 3333..."
        USES_TERMINAL
    )

    # Start GDB server with target halted at reset vector
    add_custom_target(gdb-server-halt
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "reset halt"
        COMMENT "Starting GDB server (target halted)..."
        USES_TERMINAL
    )

    #=========================================================================
    # GDB Client
    #=========================================================================

    # Start GDB client and connect to running GDB server
    add_custom_target(gdb-client
        COMMAND ${GDB}
            -ex "target remote :3333"
            -ex "file ${ELF_FILE}"
            -ex "monitor reset halt"
            ${ELF_FILE}
        DEPENDS ${TARGET_NAME}
        COMMENT "Starting GDB client (connecting to :3333)..."
        USES_TERMINAL
    )

    # Start GDB client for post-mortem analysis (no target connection)
    add_custom_target(gdb-debug-file
        COMMAND ${GDB} ${ELF_FILE}
        DEPENDS ${TARGET_NAME}
        COMMENT "Starting GDB for ELF inspection..."
        USES_TERMINAL
    )

    #=========================================================================
    # Memory Read/Write (Parameterized)
    #=========================================================================

    # Read memory at specified address (default: 0x08000000, 256 bytes)
    # Usage: cmake --build build --target mem-read -DMEM_ADDR=0x20000000 -DMEM_LEN=128
    add_custom_target(mem-read
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "mdb ${MEM_ADDR} ${MEM_LEN}"
            -c "exit"
        COMMENT "Reading ${MEM_LEN} bytes from ${MEM_ADDR}..."
        USES_TERMINAL
    )

    # Read memory as 32-bit words (hex dump)
    # Usage: cmake --build build --target mem-read32 -DMEM_ADDR=0x20000000 -DMEM_LEN=32
    add_custom_target(mem-read32
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "mdw ${MEM_ADDR} ${MEM_LEN}"
            -c "exit"
        COMMENT "Reading ${MEM_LEN} 32-bit words from ${MEM_ADDR}..."
        USES_TERMINAL
    )

    # Write memory (byte)
    # Usage: cmake --build build --target mem-write -DMEM_ADDR=0x20000000 -DMEM_VALUE=0x42
    add_custom_target(mem-write
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "mwb ${MEM_ADDR} ${MEM_VALUE}"
            -c "exit"
        COMMENT "Writing 0x${MEM_VALUE} to ${MEM_ADDR}..."
        USES_TERMINAL
    )

    # Write memory (32-bit word)
    # Usage: cmake --build build --target mem-write32 -DMEM_ADDR=0x20000000 -DMEM_VALUE=0x12345678
    add_custom_target(mem-write32
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "mww ${MEM_ADDR} ${MEM_VALUE}"
            -c "exit"
        COMMENT "Writing 0x${MEM_VALUE} to ${MEM_ADDR} (32-bit)..."
        USES_TERMINAL
    )

    #=========================================================================
    # Target Inspection
    #=========================================================================

    # Display target information (registers, flash info)
    add_custom_target(target-info
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "target_info"
            -c "exit"
        COMMENT "Reading target information..."
        USES_TERMINAL
    )

    # Display core registers
    add_custom_target(target-regs
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "reg"
            -c "exit"
        COMMENT "Reading core registers..."
        USES_TERMINAL
    )

    # Check if Flash is empty (all 0xFF)
    add_custom_target(flash-check-empty
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "flash info 0"
            -c "mdb 0x08000000 32"
            -c "exit"
        COMMENT "Checking Flash status..."
        USES_TERMINAL
    )

    # Peek memory at specified address (parameterized, default: Flash start)
    # Usage: cmake --build build --target target-peek -DMEM_ADDR=0x08000000 -DMEM_LEN=256
    add_custom_target(target-peek
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
            -c "mdb ${MEM_ADDR} ${MEM_LEN}"
            -c "exit"
        COMMENT "Reading ${MEM_LEN} bytes from ${MEM_ADDR}..."
        USES_TERMINAL
    )

    #=========================================================================
    # Interactive
    #=========================================================================

    # Open Telnet console (OpenOCD CLI)
    add_custom_target(openocd-console
        COMMAND openocd -f ${OPENOCD_CFG}
            -c "init"
            -c "halt"
        COMMENT "OpenOCD console ready. Connect with: telnet localhost 4444"
        USES_TERMINAL
    )

endfunction()
