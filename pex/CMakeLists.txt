add_library(pex INTERFACE)

# Projects that include this project must #include "pex/<header-name>"
target_include_directories(pex INTERFACE ${PROJECT_SOURCE_DIR})

target_link_libraries(pex INTERFACE CONAN_PKG::jive CONAN_PKG::fields)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/pex
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
