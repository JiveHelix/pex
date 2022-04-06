cmake_minimum_required(VERSION 3.15)

project(pex CXX)

include(${CMAKE_CURRENT_LIST_DIR}/cmake_includes/setup_project.cmake)
setup_project()

add_subdirectory(pex)


if (APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(
        CMAKE_OSX_DEPLOYMENT_TARGET 10.12
        CACHE STRING "macOS Deployment Target")
endif ()


if (${PROJECT_IS_TOP_LEVEL})

    option(
        ENABLE_PEX_LOG
        "Enable the verbose debug log"
        OFF)

    option(
        BUILD_EXAMPLES
        "Build the example targets"
        ${ENABLES_DEFAULT})
endif ()


if (${ENABLE_PEX_LOG})
    add_definitions(-DENABLE_PEX_LOG)
else ()
    add_definitions(-DDO_NOT_ENABLE_PEX_LOG)
endif ()


if (${PROJECT_IS_TOP_LEVEL})

    if (ENABLE_TESTING)
        add_subdirectory(test)
    endif()

    if (${BUILD_EXAMPLES})
        add_subdirectory(examples)
    endif ()

elseif (${RECURSIVE_BUILD_TESTS} AND ${ENABLE_TESTING})
    add_subdirectory(test)
endif ()
