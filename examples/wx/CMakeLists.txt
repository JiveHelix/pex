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


add_executable(field_demo field_demo.cpp)

target_link_libraries(
    field_demo
    PUBLIC
    project_warnings
    project_options
    pex
    wx::net wx::core wx::base)


add_executable(bitset_demo bitset_demo.cpp)

target_link_libraries(
    bitset_demo
    PUBLIC
    project_warnings
    project_options
    pex
    wx::net wx::core wx::base)


add_executable(bitset_demo_with_filter bitset_demo_with_filter.cpp)

target_link_libraries(
    bitset_demo_with_filter
    PUBLIC
    project_warnings
    project_options
    pex
    wx::net wx::core wx::base)


add_executable(radio_box_demo radio_box_demo.cpp)

target_link_libraries(
    radio_box_demo
    PUBLIC
    project_warnings
    project_options
    pex
    wx::net wx::core wx::base)
