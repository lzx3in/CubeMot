# Board configuration
# Use -DBOARD=<board_name> to select board (default: nucleo_g431rb)

if(NOT DEFINED BOARD)
    set(BOARD "nucleo_g431rb" CACHE STRING "Target board name" FORCE)
    message(STATUS "BOARD not specified, using default: ${BOARD}")
else()
    message(STATUS "Selected board: ${BOARD}")
endif()

# Normalize board name (replace hyphens with underscores for CMake target names)
string(REPLACE "-" "_" BOARD_TARGET_NAME ${BOARD})

# Validate board directory exists
set(BOARD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD})
if(NOT EXISTS ${BOARD_DIR})
    message(FATAL_ERROR "Board '${BOARD}' not found at ${BOARD_DIR}\n"
                        "Available boards: $(ls ${CMAKE_CURRENT_SOURCE_DIR}/boards/)")
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
