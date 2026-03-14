# Include post-build functions before subdirectories that need them
include(${CMAKE_SOURCE_DIR}/cmake/post_build.cmake)

# FreeRTOS configuration
# Note: freertos_config target is defined in boards/<BOARD>/freertos_config/CMakeLists.txt
set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "FreeRTOS port" FORCE)
set(FREERTOS_HEAP 4 CACHE STRING "FreeRTOS heap implementation" FORCE)

add_subdirectory(common)
add_subdirectory(drivers)      # Framework first (provides interfaces)
add_subdirectory(chips/stm32g4)
add_subdirectory(libs)
add_subdirectory(modules)
add_subdirectory(boards)        # Board code (registers HAL implementations)
add_subdirectory(freertos)
add_subdirectory(msghub)
add_subdirectory(app)
add_subdirectory(topics)

# Remove incorrect libob.a dependency
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Programming targets
include(${CMAKE_SOURCE_DIR}/cmake/programming_targets.cmake)
add_programming_targets(${CMAKE_PROJECT_NAME})

# Debugging targets
include(${CMAKE_SOURCE_DIR}/cmake/debugging_targets.cmake)
add_debugging_targets(${CMAKE_PROJECT_NAME})

