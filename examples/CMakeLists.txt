add_executable(filter_demo filter_demo.cpp)

target_link_libraries(
    filter_demo
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


add_executable(range_demo range_demo.cpp)

target_link_libraries(
    range_demo
    PUBLIC
    project_warnings
    project_options
    pex)


add_executable(serialization serialization.cpp)

target_link_libraries(
    serialization
    PUBLIC
    project_warnings
    project_options
    pex
    CONAN_PKG::nlohmann_json)


add_executable(simple simple.cpp)

target_link_libraries(
    simple
    PUBLIC
    project_warnings
    project_options
    pex)
