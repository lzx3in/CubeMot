add_library(middlewares INTERFACE)

# Optional components
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/communication)
    target_include_directories(middlewares INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/communication
    )
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE middlewares)
