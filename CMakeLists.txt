cmake_minimum_required(VERSION 3.28)

# Project configuration
project(CubeMot)
message(STATUS "Project: ${CMAKE_PROJECT_NAME}")

option(CMAKE_EXPORT_COMPILE_COMMANDS "generate compile_commands.json for IDE integration" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Board configuration must come before output directory settings
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/board_config.cmake)

# Generate configuration headers from KConfig at configure time
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/kconfig.cmake)

# Add project modules
add_subdirectory(src)
