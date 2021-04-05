# NcZero

[![Build Status](https://travis-ci.com/codeandkey/nczero.svg?branch=master)](https://travis-ci.com/codeandkey/nczero) [![Coverage Status](https://coveralls.io/repos/github/codeandkey/nczero/badge.svg?branch=master&kill_cache=1)](https://coveralls.io/github/codeandkey/nczero?branch=master)

A self-learning chess engine.

## architecture

Many techniques in this engine are inspired by the revolutionary [AlphaZero](https://arxiv.org/pdf/1712.01815.pdf) as well as the extensive [Chess Programming Wiki](https://www.chessprogramming.org).

- Parallel Monte Carlo Tree Search
- Bitboard move generation
- Incremental input layer updates

## dependencies

- CMake 3.0+
- GCC 8+ on \*NIX, or MSVC 15.7+ on Windows
- [LibTorch](https://pytorch.org/get-started/locally/) 1.8.1 with CMake module
- [GoogleTest](https://github.com/google/googletest) for debug/test builds

## building

To build nczero execute `./compile` in the project directory.

After nczero is built you can execute it with `build/bin/nczero`.

> NOTE: Building the test suite requires GoogleTest to be available on the host.
