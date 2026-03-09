add_executable(${CMAKE_PROJECT_NAME}
    main.c
    ${BOARD_INTERRUPT_SRCS}
    ${BOARD_STARTUP_SRC}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    stm32g4_hal
    boards
    drivers
    freertos_kernel
    ${TOOLCHAIN_LINK_LIBRARIES}
)

# Post-build: consolidate outputs to target/
consolidate_outputs_to_target_dir(${CMAKE_PROJECT_NAME})

# Programming targets: flash, flash-bin, flash-erase, flash-erase-sectors,
#                      flash-verify, flash-compare, flash-read, flash-backup,
#                      show-size, show-size-detailed, show-symbols
#   (Parameterized: -DFLASH_SIZE=... -DFLASH_ERASE_START=... -DFLASH_ERASE_END=...)
include(${CMAKE_SOURCE_DIR}/cmake/programming_targets.cmake)
add_programming_targets(${CMAKE_PROJECT_NAME})

# Debugging targets: target-reset, target-reset-halt, target-halt, target-resume,
#                    gdb-server, gdb-server-halt, gdb-client, gdb-debug-file,
#                    mem-read, mem-read32, mem-write, mem-write32, target-info,
#                    target-regs, flash-check-empty, target-peek, openocd-console
#   (Parameterized: -DMEM_ADDR=... -DMEM_LEN=... -DMEM_VALUE=...)
include(${CMAKE_SOURCE_DIR}/cmake/debugging_targets.cmake)
add_debugging_targets(${CMAKE_PROJECT_NAME})
