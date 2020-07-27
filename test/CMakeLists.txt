# Automatically enable catch2 to generate ctest targets
if(CONAN_CATCH2_ROOT_DEBUG)
  include(${CONAN_CATCH2_ROOT_DEBUG}/lib/cmake/Catch2/Catch.cmake)
else()
  include(${CONAN_CATCH2_ROOT}/lib/cmake/Catch2/Catch.cmake)
endif()

add_library(pex_catch_main STATIC catch_main.cpp)
target_link_libraries(pex_catch_main PUBLIC CONAN_PKG::catch2)
target_link_libraries(pex_catch_main PRIVATE project_options)

add_executable(
    pex_tests
    value_tests.cpp
    signal_tests.cpp
    filter_tests.cpp)

target_link_libraries(
    pex_tests
    PRIVATE
    project_warnings
    project_options
    pex_catch_main
    pex)

# Automatically discover tests that are defined in catch based test files.
catch_discover_tests(
    pex_tests
    TEST_PREFIX
    "unittests."
    EXTRA_ARGS)
