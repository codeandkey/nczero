# NcZero

[![Build Status](https://travis-ci.com/codeandkey/nczero.svg?branch=master)](https://travis-ci.com/codeandkey/nczero) [![Coverage Status](https://coveralls.io/repos/github/codeandkey/nczero/badge.svg?branch=master&kill_cache=1)](https://coveralls.io/github/codeandkey/nczero?branch=master)

Chess engine powered with reinforcement learning.

## architecture

Many techniques used in this engine are inspired by the revolutionary [AlphaZero](https://arxiv.org/pdf/1712.01815.pdf) program as well as the extensive [Chess Programming Wiki](https://www.chessprogramming.org).

- Parallel Monte Carlo Tree Search
- Bitboard move generation
- Incremental input layer updates

## dependencies

- CMake 3.14+
- [LibTorch](https://pytorch.org/get-started/locally/) 1.8.1+
- *(optional)* [GoogleTest](https://github.com/google/googletest) for tests

GCC 8+ is required for debug and test builds.

## building

To build nczero execute `./compile` in the project directory.

After nczero is built you can execute it with `build/bin/nczero`.
