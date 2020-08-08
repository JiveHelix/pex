add_executable(view_demo view_demo.cpp)

target_link_libraries(
    view_demo
    PUBLIC
    project_warnings
    project_options
    pex
    wx::net wx::core wx::base)


add_executable(check_box_demo check_box_demo.cpp)

target_link_libraries(
    check_box_demo
    PUBLIC
    project_warnings
    project_options
    pex
    wx::net wx::core wx::base)
