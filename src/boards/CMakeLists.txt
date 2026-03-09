# In test mode, create an INTERFACE library (header-only) to provide include paths
# In embedded mode, create a STATIC library with full implementation
if(BUILD_TESTING)
    add_library(boards INTERFACE)
    target_include_directories(boards INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}
    )
else()
    add_library(boards STATIC)
    target_include_directories(boards PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}
    )
    # Only add board-specific implementation in non-test mode
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/${BOARD})
endif()

target_link_libraries(boards PRIVATE
    stm32g4_hal
)
