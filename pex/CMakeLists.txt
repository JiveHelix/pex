add_library(pex)

if (${fPIC})
    set_property(TARGET pex PROPERTY POSITION_INDEPENDENT_CODE ON)
endif ()

find_package(Fmt REQUIRED)
find_package(Jive REQUIRED)
find_package(Fields REQUIRED)

# Projects that include this project must #include "pex/<header-name>"
target_include_directories(pex PUBLIC ${PROJECT_SOURCE_DIR})

target_compile_options(pex PUBLIC $<$<CXX_COMPILER_ID:GNU>:-fconcepts>)

target_link_libraries(
    pex
    PUBLIC
    project_options
    project_warnings
    jive::jive
    fields::fields
    fmt::fmt)

target_sources(
    pex
    PRIVATE
    model_value.cpp
    control_value.cpp
    detail/log.cpp)

install(TARGETS pex DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/pex
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
