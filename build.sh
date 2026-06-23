#!/bin/env bash

cmake -S . -B build-linux -G Ninja
cmake --build build-linux

cmake -S . -B build-win -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=mingw-toolchain.cmake
cmake --build build-win