add_library(pex_catch_main STATIC catch_main.cpp)
target_link_libraries(pex_catch_main PUBLIC CONAN_PKG::catch2)
target_link_libraries(pex_catch_main PRIVATE project_options)

add_executable(
    pex_tests
    aggregate_tests.cpp
    assign_tests.cpp
    endpoint_tests.cpp
    filter_tests.cpp
    group_tests.cpp
    list_tests.cpp
    poly_list_tests.cpp
    range_tests.cpp
    select_tests.cpp
    signal_tests.cpp
    terminus_tests.cpp
    traits_tests.cpp
    value_tests.cpp)

target_link_libraries(
    pex_tests
    PRIVATE
    project_warnings
    project_options
    pex_catch_main
    pex
    CONAN_PKG::nlohmann_json)
