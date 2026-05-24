cmake_minimum_required(VERSION 3.16)

project(gameboy_emulator)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
include_directories(src)
include_directories(${SDL2_INCLUDE_DIRS})

file(GLOB SRC_FILES src/*.cpp)

add_executable(gameboy ${SRC_FILES})

target_compile_options(gameboy PRIVATE -U_GLIBCXX_ASSERTIONS)
target_compile_definitions(gameboy PRIVATE NDEBUG)

target_link_libraries(gameboy ${SDL2_LIBRARIES})