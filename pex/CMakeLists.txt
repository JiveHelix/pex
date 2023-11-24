add_library(pex)

if (${fPIC})
    set_property(TARGET pex PROPERTY POSITION_INDEPENDENT_CODE ON)
endif ()

# Projects that include this project must #include "pex/<header-name>"
target_include_directories(pex PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(
    pex
    PUBLIC
    CONAN_PKG::jive
    CONAN_PKG::fields
    CONAN_PKG::nlohmann_json)

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
