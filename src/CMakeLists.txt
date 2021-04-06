add_subdirectory (libnczero)
add_subdirectory (nczero)

# Build tests if debug
option (BUILD_TESTS "Build the tests" ON)

if (BUILD_TESTS)
    add_subdirectory (tests)
endif ()
