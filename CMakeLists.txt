cmake_minimum_required(VERSION 3.28)

# Set default toolchain before project() as CMake loads toolchain file during project() command
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc_arm_none_eabi_toolchain.cmake" 
        CACHE STRING "Toolchain file" FORCE)
    message(STATUS "Using default toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Project configuration
project(CubeMot)
message(STATUS "Project: ${CMAKE_PROJECT_NAME}")

option(CMAKE_EXPORT_COMPILE_COMMANDS "generate compile_commands.json for IDE integration" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Generate configuration headers from KConfig at configure time
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/kconfig.cmake)

# Board configuration must come before output directory settings
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/board_config.cmake)

# Add project modules
add_subdirectory(src)
