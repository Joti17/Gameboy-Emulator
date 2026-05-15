#!/bin/env bash

cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
cd ..
gdb --args ./build/gameboy -r roms/LOZ.gb