# Board configuration
# BOARD is read from Kconfig BOARD_NAME symbol
# Run 'make menuconfig' to select board

# Python interpreter: prefer venv, fallback to system Python3
if(EXISTS ${CMAKE_SOURCE_DIR}/.venv/bin/python)
    set(PYTHON_EXECUTABLE ${CMAKE_SOURCE_DIR}/.venv/bin/python)
else()
    find_package(Python3 REQUIRED)
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
endif()

# Kconfig paths (duplicated from kconfig.cmake for independence)
set(KCONFIG_ROOT ${CMAKE_SOURCE_DIR}/Kconfig)
set(KCONFIG_USRCONFIG ${CMAKE_SOURCE_DIR}/.config)
set(KCONFIG_DEFCONFIG ${CMAKE_SOURCE_DIR}/defconfig)

# Query BOARD_NAME from Kconfig using gen_config.py
set(ENV{SRCTREE} ${CMAKE_SOURCE_DIR})
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_config.py
            ${KCONFIG_ROOT}
            --usrconfig ${KCONFIG_USRCONFIG}
            --defconfig ${KCONFIG_DEFCONFIG}
            --symbol BOARD_NAME
    OUTPUT_VARIABLE BOARD
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE BOARD_QUERY_RESULT
    ERROR_VARIABLE BOARD_QUERY_ERROR
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

if(NOT BOARD_QUERY_RESULT EQUAL 0 OR NOT BOARD)
    set(BOARD "nucleo_g431rb")
    message(STATUS "BOARD_NAME not found in Kconfig, using default: ${BOARD}")
    if(BOARD_QUERY_ERROR)
        message(STATUS "  (Query error: ${BOARD_QUERY_ERROR})")
    endif()
else()
    message(STATUS "Board from Kconfig: ${BOARD}")
endif()

# Normalize board name (replace hyphens with underscores for CMake target names)
string(REPLACE "-" "_" BOARD_TARGET_NAME ${BOARD})

# Validate board directory exists
set(BOARD_DIR ${CMAKE_SOURCE_DIR}/src/boards/${BOARD})
if(NOT EXISTS ${BOARD_DIR})
    message(FATAL_ERROR "Board '${BOARD}' not found at ${BOARD_DIR}\n"
                        "Available boards: $(ls ${CMAKE_SOURCE_DIR}/src/boards/)")
endif()

# Include board-specific configuration
set(BOARD_CONFIG_FILE ${BOARD_DIR}/board_config.cmake)
if(EXISTS ${BOARD_CONFIG_FILE})
    include(${BOARD_CONFIG_FILE})
else()
    message(FATAL_ERROR "Board configuration file not found: ${BOARD_CONFIG_FILE}")
endif()

# Debug build definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND BOARD_COMPILE_DEFINITIONS DEBUG)
endif()
