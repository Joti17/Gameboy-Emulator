#!/bin/env bash

cd build
cmake ..
make
cd ..
./build/gameboy -r roms/LOZ.gb