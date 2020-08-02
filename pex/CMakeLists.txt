add_library(pex INTERFACE)

# Projects that include this project must #include "pex/<header-name>"
target_include_directories(pex INTERFACE ${PROJECT_SOURCE_DIR})

target_link_libraries(pex INTERFACE jive)

target_sources(
    pex
    INTERFACE
    initialize.h
    notify.h
    signal.h
    value.h)
