cmake_minimum_required (VERSION 3.0)
project (nczero)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

option (DEBUG "Enable debug build")

if (BUILD_TESTS)
    set(DEBUG ON)
endif ()

# Set build output
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Make sure compiler is gcc if debug
if (DEBUG)
    if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        message(FATAL_ERROR "Debug builds only supported on GCC!")
    endif ()

    message (STATUS "Building in DEBUG mode")

    # Add debug macro
    add_compile_definitions (NCZ_DEBUG)

    # Add debug flags
    set (GCC_DEBUG_FLAGS "-g -Wall -Werror -O0")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_DEBUG_FLAGS}")
else ()
    message (STATUS "Building in RELEASE mode")

    # Add release flags
    set (GCC_RELEASE_FLAGS "-g -Wall -Werror -O0")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_DEBUG_FLAGS}")
    endif ()
endif ()

if (BUILD_TESTS)
    # Add coverage flags
    set (GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
    set (GCC_COVERAGE_LINK_FLAGS "-lgcov")

    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

    # Enable tests build
    enable_testing()
endif()

# Find libtorch
find_package (Torch REQUIRED)
add_compile_options (${TORCH_CXX_FLAGS})

add_subdirectory (src)
