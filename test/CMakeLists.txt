add_library(pex_catch_main STATIC catch_main.cpp)
target_link_libraries(pex_catch_main PUBLIC CONAN_PKG::catch2)
target_link_libraries(pex_catch_main PRIVATE project_options)

add_executable(
    pex_tests
    value_tests.cpp
    signal_tests.cpp
    filter_tests.cpp
    expand_tests.cpp
    range_tests.cpp
    assign_tests.cpp
    terminus_tests.cpp
    group_tests.cpp
    select_tests.cpp)

target_link_libraries(
    pex_tests
    PRIVATE
    project_warnings
    project_options
    pex_catch_main
    pex)
