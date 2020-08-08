add_executable(filter_demo filter_demo.cpp)

target_link_libraries(
    filter_demo
    PUBLIC
    project_warnings
    project_options
    pex)

add_subdirectory(wx)
