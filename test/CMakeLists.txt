find_package(Nlohmann_json REQUIRED)

add_catch2_test(
    NAME pex_tests
    SOURCES
        aggregate_tests.cpp
        assign_tests.cpp
        endpoint_tests.cpp
        filter_tests.cpp
        group_tests.cpp
        list_tests.cpp
        ordered_list_tests.cpp
        poly_list_tests.cpp
        range_tests.cpp
        select_tests.cpp
        signal_tests.cpp
        swap_tests.cpp
        terminus_tests.cpp
        traits_tests.cpp
        value_tests.cpp
    LINK
        pex
        nlohmann_json::nlohmann_json)
