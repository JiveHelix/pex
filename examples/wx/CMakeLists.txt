add_executable(view_demo view_demo.cpp)

target_link_libraries(
    view_demo
    PRIVATE
    project_warnings
    project_options
    pex
    wx::core)


add_executable(check_box_demo check_box_demo.cpp)

target_link_libraries(
    check_box_demo
    PRIVATE
    project_warnings
    project_options
    pex
    wx::core)


add_executable(field_demo field_demo.cpp)

target_link_libraries(
    field_demo
    PRIVATE
    project_warnings
    project_options
    pex
    wx::core)


add_executable(bitset_demo bitset_demo.cpp)

target_link_libraries(
    bitset_demo
    PRIVATE
    project_warnings
    project_options
    pex
    wx::core)


add_executable(bitset_demo_with_filter bitset_demo_with_filter.cpp)

target_link_libraries(
    bitset_demo_with_filter
    PRIVATE
    project_warnings
    project_options
    pex
    wx::core)


add_executable(radio_box_demo radio_box_demo.cpp)

target_link_libraries(
    radio_box_demo
    PRIVATE
    project_warnings
    project_options
    pex
    wx::core)


add_executable(combo_box_demo combo_box_demo.cpp)

target_link_libraries(
    combo_box_demo
    PRIVATE
    project_warnings
    project_options
    pex
    wx::core)


add_executable(slider_demo slider_demo.cpp)

target_link_libraries(
    slider_demo
    PRIVATE
    project_warnings
    project_options
    pex
    wx::core)


add_executable(spin_control_double_demo spin_control_double_demo.cpp)

target_link_libraries(
    spin_control_double_demo
    PRIVATE
    project_warnings
    project_options
    pex
    wx::core)
