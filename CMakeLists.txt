cmake_minimum_required(VERSION 3.28)

# Project configuration
project(CubeMot C ASM)
message(STATUS "Project: ${CMAKE_PROJECT_NAME}")

option(CMAKE_EXPORT_COMPILE_COMMANDS "generate compile_commands.json for IDE integration" ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Board configuration must come before output directory settings
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/board_config.cmake)

# Generate configuration headers from KConfig at configure time
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/kconfig.cmake)

enable_language(C ASM)

# Include post-build functions before subdirectories that need them
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/post_build.cmake)

# Add project modules
add_subdirectory(boards)
add_subdirectory(application)
add_subdirectory(drivers)

# Remove incorrect libob.a dependency
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
