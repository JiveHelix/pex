add_executable(view_demo view_demo.cpp)

target_link_libraries(
    view_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::tau
    CONAN_PKG::wxwidgets)


add_executable(check_box_demo check_box_demo.cpp)

target_link_libraries(
    check_box_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


add_executable(field_demo field_demo.cpp)

target_link_libraries(
    field_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::tau
    CONAN_PKG::wxwidgets)


add_executable(async_demo async_demo.cpp)

target_link_libraries(
    async_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::tau
    CONAN_PKG::wxwidgets)


add_executable(bitset_demo bitset_demo.cpp)

target_link_libraries(
    bitset_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


add_executable(radio_box_demo radio_box_demo.cpp)

target_link_libraries(
    radio_box_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


add_executable(combo_box_demo combo_box_demo.cpp)

target_link_libraries(
    combo_box_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


add_executable(slider_demo slider_demo.cpp)

target_link_libraries(
    slider_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


add_executable(spin_control_double_demo spin_control_double_demo.cpp)

target_link_libraries(
    spin_control_double_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


add_executable(shortcut_demo shortcut_demo.cpp)

target_link_libraries(
    shortcut_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::fields
    CONAN_PKG::wxwidgets)


add_executable(settings_demo settings_demo.cpp)

target_link_libraries(
    settings_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::fields
    CONAN_PKG::wxwidgets)


add_executable(color_demo color_demo.cpp)

target_link_libraries(
    color_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::tau
    CONAN_PKG::wxwidgets)


# add_executable(sandbox sandbox.cpp)

# target_link_libraries(
#     sandbox
#     PRIVATE
#     project_warnings
#     project_options
#     pex
#     CONAN_PKG::tau
#     CONAN_PKG::fields
#     CONAN_PKG::wxwidgets)
