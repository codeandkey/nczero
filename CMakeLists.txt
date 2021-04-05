cmake_minimum_required (VERSION 3.0)
project (nczero)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# Set build type
if (CMAKE_CONFIGURATION_TYPES)
    set (CMAKE_CONFIGURATION_TYPES Debug Release)
elseif (NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "Missing build type. Run cmake with one of the following flags:\n    -DCMAKE_BUILD_TYPE=Release\n    -DCMAKE_BUILD_TYPE=Debug")
endif ()

set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug)

# Get googletest if debug
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Compiling in debug mode. Fetching googletest..")
    include (FetchContent)

    # Fetch googletest 6c5c455 March 29, 2021
    FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/6c5c4554ac218a8e19168edc121b1ad232015185.zip)

    # Fix compiler override on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Add debug macro
    add_compile_definitions (NCZ_DEBUG)
else ()
    # Add release macro
    add_compile_definitions (NCZ_DEBUG)
endif ()

# Find libtorch
find_package (Torch REQUIRED)
add_compile_options (${TORCH_CXX_FLAGS})

add_subdirectory (src)
