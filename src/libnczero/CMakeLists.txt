set (
    SOURCES
    attacks.cpp
    bitboard.cpp
    board.cpp
    color.cpp
    move.cpp
    net.cpp
    perft.cpp
    piece.cpp
    position.cpp
    search.cpp
    square.cpp
    type.cpp
    util.cpp
    zobrist.cpp
)

set (INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

set (
    HEADERS
    ${INCLUDE_DIR}/nczero/attacks.h
    ${INCLUDE_DIR}/nczero/bitboard.h
    ${INCLUDE_DIR}/nczero/board.h
    ${INCLUDE_DIR}/nczero/color.h
    ${INCLUDE_DIR}/nczero/move.h
    ${INCLUDE_DIR}/nczero/net.h
    ${INCLUDE_DIR}/nczero/perft.h
    ${INCLUDE_DIR}/nczero/piece.h
    ${INCLUDE_DIR}/nczero/position.h
    ${INCLUDE_DIR}/nczero/search.h
    ${INCLUDE_DIR}/nczero/square.h
    ${INCLUDE_DIR}/nczero/type.h
    ${INCLUDE_DIR}/nczero/util.h
    ${INCLUDE_DIR}/nczero/zobrist.h
)

add_library (nczero ${SOURCES} ${HEADERS})
target_include_directories (nczero PRIVATE ${INCLUDE_DIR})

# Find libtorch
set (CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/torch_cmake)
find_package (Torch REQUIRED)
target_include_directories (nczero PRIVATE ${TORCH_INCLUDE_DIRS})
target_link_libraries (nczero ${TORCH_LIBRARIES})
add_compile_options (${TORCH_CXX_FLAGS})
