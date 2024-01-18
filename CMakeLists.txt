cmake_minimum_required(VERSION 3.15)

project(pex CXX)

include(${CMAKE_CURRENT_LIST_DIR}/cmake_includes/setup_project.cmake)
setup_project()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts")
endif ()

add_subdirectory(pex)


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
