cmake_minimum_required(VERSION 3.15)

# detect whether pex has been added as a subproject
if(NOT DEFINED PROJECT_NAME)
    set(PEX_NOT_SUBPROJECT ON)
endif()

include(cmake_includes/DefineProjectOnce.cmake)

define_project_once(pex)

project(pex CXX)

if(PEX_NOT_SUBPROJECT)
    # This is the top-level project
    include(cmake_includes/Standalone.cmake)
    option(ENABLE_UNITY "Enable Unity builds of projects" OFF)
endif()

add_subdirectory(external)
add_subdirectory(pex)

if(PEX_NOT_SUBPROJECT)

    add_subdirectory(examples)

    if(ENABLE_TESTING)
        enable_testing()
        add_subdirectory(test)
    endif()

elseif(RECURSIVE_BUILD_TESTS AND ENABLE_TESTING)

    # A parent project has requested tests.
    add_subdirectory(test)

endif()

if(ENABLE_UNITY)
      set_target_properties(pex PROPERTIES UNITY_BUILD ON)
endif()
