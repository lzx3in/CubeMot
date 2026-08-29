# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(NOT BUILD_TESTING)
    find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
endif()

project(CubeMot)

# ── msghub location (parameterized; default matches the workspace layout) ────
# Override with -DMSGHUB_DIR=<path> when msghub lives elsewhere (e.g. a west
# module checkout, a vendored copy, or a CI workspace).
set(MSGHUB_DIR "${CMAKE_SOURCE_DIR}/../shared/msghub" CACHE PATH "Path to the msghub repository")
if(NOT EXISTS "${MSGHUB_DIR}/include/msghub/msghub.h")
    message(FATAL_ERROR "msghub not found at ${MSGHUB_DIR}; pass -DMSGHUB_DIR=<path> to point at the msghub repository")
endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
else()
    add_subdirectory(src)
endif()