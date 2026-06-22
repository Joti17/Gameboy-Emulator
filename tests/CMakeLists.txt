cmake_minimum_required(VERSION 3.16)
project(GameBoyTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

add_executable(execute_test
    execute.cpp
    ../src/cpu.cpp
    ../src/memory.cpp
    ../src/mmu.cpp
    ../src/sound.cpp
    ../src/input.cpp
    ../src/ppu.cpp
)

target_include_directories(execute_test PRIVATE ../include)

target_link_libraries(execute_test PRIVATE SDL2::SDL2)