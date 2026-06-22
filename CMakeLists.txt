cmake_minimum_required(VERSION 3.16)

project(gameboy_emulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

add_executable(gameboy ${SRC_FILES})

target_include_directories(gameboy PRIVATE
    include
    src
)


if (MINGW)
    include_directories(/usr/x86_64-w64-mingw32/include/SDL2)
    link_directories(/usr/x86_64-w64-mingw32/lib)

    target_link_libraries(gameboy PRIVATE SDL2 SDL2main)

    target_compile_definitions(gameboy PRIVATE SDL_MAIN_HANDLED)

else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)

    target_include_directories(gameboy PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(gameboy PRIVATE ${SDL2_LIBRARIES})

    target_compile_options(gameboy PRIVATE ${SDL2_CFLAGS_OTHER})
endif()

target_compile_definitions(gameboy PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
)