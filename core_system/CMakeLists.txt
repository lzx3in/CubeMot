add_library(core_system INTERFACE)

# Optional components
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/osal)
    target_include_directories(core_system INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/osal
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rtos)
    target_include_directories(core_system INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/rtos
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/services)
    target_include_directories(core_system INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/services
    )
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE core_system)
