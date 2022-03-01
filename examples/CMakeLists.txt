add_executable(filter_demo filter_demo.cpp)

target_link_libraries(
    filter_demo
    PUBLIC
    project_warnings
    project_options
    CONAN_PKG::tau
    pex)

add_subdirectory(wx)
