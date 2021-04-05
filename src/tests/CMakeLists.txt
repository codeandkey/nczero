set (
    SOURCES
    tests.cpp
)

set (INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

include (FetchContent)

# Fetch googletest 6c5c455 March 29, 2021
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/6c5c4554ac218a8e19168edc121b1ad232015185.zip)

# Fix compiler override on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include (GoogleTest)

add_executable (nczero_test ${SOURCES})
target_include_directories (nczero_test PRIVATE ${INCLUDE_DIR})

target_link_libraries (nczero_test gtest)
target_link_libraries (nczero_test libnczero)

gtest_discover_tests (nczero_test)
