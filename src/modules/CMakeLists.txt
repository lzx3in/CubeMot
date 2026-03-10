# Modules - Business logic layer
# Each module is a self-contained business feature

add_subdirectory(blink)

add_library(modules INTERFACE)
target_link_libraries(modules INTERFACE
    module_blink
)
