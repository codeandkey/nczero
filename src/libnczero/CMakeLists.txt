set (
    SOURCES
    chess/attacks.cpp
    chess/bitboard.cpp
    chess/board.cpp
    chess/color.cpp
    chess/move.cpp
    chess/perft.cpp
    chess/piece.cpp
    chess/position.cpp
    chess/square.cpp
    chess/type.cpp
    chess/zobrist.cpp
    log.cpp
    net.cpp
    node.cpp
    pool.cpp
    timer.cpp
    worker.cpp
)

set (INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

set (
    HEADERS
    ${INCLUDE_DIR}/nczero/chess/attacks.h
    ${INCLUDE_DIR}/nczero/chess/bitboard.h
    ${INCLUDE_DIR}/nczero/chess/board.h
    ${INCLUDE_DIR}/nczero/chess/color.h
    ${INCLUDE_DIR}/nczero/chess/move.h
    ${INCLUDE_DIR}/nczero/chess/perft.h
    ${INCLUDE_DIR}/nczero/chess/piece.h
    ${INCLUDE_DIR}/nczero/chess/position.h
    ${INCLUDE_DIR}/nczero/chess/square.h
    ${INCLUDE_DIR}/nczero/chess/type.h
    ${INCLUDE_DIR}/nczero/chess/zobrist.h
    ${INCLUDE_DIR}/nczero/log.h
    ${INCLUDE_DIR}/nczero/net.h
    ${INCLUDE_DIR}/nczero/node.h
    ${INCLUDE_DIR}/nczero/pool.h
    ${INCLUDE_DIR}/nczero/timer.h
    ${INCLUDE_DIR}/nczero/worker.h
)

add_library (libnczero ${SOURCES} ${HEADERS})
target_include_directories (libnczero PRIVATE ${INCLUDE_DIR})

target_include_directories (libnczero PRIVATE ${TORCH_INCLUDE_DIRS})
target_link_libraries (libnczero ${TORCH_LIBRARIES})

