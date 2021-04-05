add_subdirectory (libnczero)
add_subdirectory (nczero)

# Build tests if debug
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_subdirectory (tests)
endif ()
