add_library(pex INTERFACE)

# Projects that include this project must #include "pex/<header-name>"
target_include_directories(pex INTERFACE ${PROJECT_SOURCE_DIR})

target_link_libraries(pex INTERFACE jive wxshim)

target_sources(
    pex
    INTERFACE
    initialize.h
    signal.h
    value.h
    detail/argument.h
    detail/callable_style.h
    detail/filters.h
    detail/implements_connect.h
    detail/notify.h
    detail/signal_detail.h
    detail/value_detail.h
    detail/value_notify.h)
