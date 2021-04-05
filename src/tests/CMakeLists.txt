set (
    SOURCES
    tests.cpp
)

set (INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_executable(nczero_test ${SOURCES})
target_include_directories (nczero_test PRIVATE ${INCLUDE_DIR})

target_link_libraries (nczero_test gtest)
