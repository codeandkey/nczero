set (
    SOURCES
    main.cpp
)

set (INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_executable (nczero ${SOURCES})
target_include_directories (nczero PRIVATE ${INCLUDE_DIR})

target_link_libraries (nczero libnczero)
