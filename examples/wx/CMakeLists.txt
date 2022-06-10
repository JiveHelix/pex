if (MSVC)
    set(windowed WIN32)
elseif (APPLE)
    set(windowed MACOSX_BUNDLE)
endif ()

add_executable(view_demo ${windowed} view_demo.cpp)

target_link_libraries(
    view_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::tau
    CONAN_PKG::wxwidgets)


add_executable(check_box_demo ${windowed} check_box_demo.cpp)

target_link_libraries(
    check_box_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


add_executable(field_demo ${windowed} field_demo.cpp)

target_link_libraries(
    field_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::tau
    CONAN_PKG::wxwidgets)


add_executable(async_demo ${windowed} async_demo.cpp)

target_link_libraries(
    async_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::tau
    CONAN_PKG::wxwidgets)


add_executable(bitset_demo ${windowed} bitset_demo.cpp)

target_link_libraries(
    bitset_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


add_executable(radio_box_demo ${windowed} radio_box_demo.cpp)

target_link_libraries(
    radio_box_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


add_executable(combo_box_demo ${windowed} combo_box_demo.cpp)

target_link_libraries(
    combo_box_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


add_executable(slider_demo ${windowed} slider_demo.cpp)

target_link_libraries(
    slider_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::wxwidgets)


# Left as a console app for demonstration.
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


add_executable(settings_demo ${windowed} settings_demo.cpp)

target_link_libraries(
    settings_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::fields
    CONAN_PKG::wxwidgets)


add_executable(color_demo ${windowed} color_demo.cpp)

target_link_libraries(
    color_demo
    PRIVATE
    project_warnings
    project_options
    pex
    CONAN_PKG::tau
    CONAN_PKG::wxwidgets)
