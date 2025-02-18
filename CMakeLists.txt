cmake_minimum_required (VERSION 3.0)
project (nczero)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

option (DEBUG "Enable debug build" OFF)
option (BUILD_TESTS "Build tests" OFF)

if (BUILD_TESTS)
    set(DEBUG ON)
    message (STATUS "Will build tests")
endif ()

# Set build output
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Write compile mode
if (DEBUG)
    message (STATUS "Building in DEBUG mode")
else ()
    message (STATUS "Building in RELEASE mode")
endif ()

# Make sure compiler is gcc if debug
if (DEBUG)
    if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        message(FATAL_ERROR "Debug builds only supported on GCC!")
    endif ()

    # Add debug macro
    add_compile_definitions (NCZ_DEBUG)

    # Add debug flags
    set (GCC_DEBUG_FLAGS "-g -Wall -Werror -Wno-format-security -O0")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_DEBUG_FLAGS}")
else ()
    # Add release flags
    set (GCC_RELEASE_FLAGS "-g -Wall -Werror -Wno-format-security -O0")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_DEBUG_FLAGS}")
    endif ()
endif ()

if (BUILD_TESTS)
    # Add coverage flags
    set (GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
    set (GCC_COVERAGE_LINK_FLAGS "-fprofile-arcs -ftest-coverage -lgcov")

    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

    # Enable tests build
    enable_testing()
endif()

# Find libtorch
find_package (Torch REQUIRED)
add_compile_options (${TORCH_CXX_FLAGS})

add_subdirectory (src)
