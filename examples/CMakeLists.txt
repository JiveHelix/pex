add_executable(filter_demo filter_demo.cpp)

target_link_libraries(
    filter_demo
    PUBLIC
    project_warnings
    project_options
    pex)


add_executable(expand_demo expand_demo.cpp)

target_link_libraries(
    expand_demo
    PUBLIC
    project_warnings
    project_options
    pex)


add_executable(group_demo group_demo.cpp)

target_link_libraries(
    group_demo
    PUBLIC
    project_warnings
    project_options
    pex)
