cmake_minimum_required (VERSION 3.0)
project (nczero)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# Set build output
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Set build type
if (CMAKE_CONFIGURATION_TYPES)
    set (CMAKE_CONFIGURATION_TYPES Debug Release)
elseif (NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "Missing build type. Run cmake with one of the following flags:\n    -DCMAKE_BUILD_TYPE=Release\n    -DCMAKE_BUILD_TYPE=Debug")
endif ()

set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Add debug macro
    add_compile_definitions (NCZ_DEBUG)

    # Enable tests build
    enable_testing()

    # Add coverage, debug flags
    set (GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
    set (GCC_COVERAGE_LINK_FLAGS "-lgcov")

    set (GCC_DEBUG_FLAGS "-g -Wall -Werror")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
        set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_DEBUG_FLAGS}")
    else ()
        message(FATAL_ERROR "Debug builds only supported on gcc.")
    endif ()
else ()
    # Add release macro
    add_compile_definitions (NCZ_DEBUG)
endif ()

# Find libtorch
find_package (Torch REQUIRED)
add_compile_options (${TORCH_CXX_FLAGS})

add_subdirectory (src)
