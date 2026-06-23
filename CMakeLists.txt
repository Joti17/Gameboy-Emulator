cmake_minimum_required(VERSION 3.16)

project(gameboy_emulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
add_executable(gameboy ${SRC_FILES})

target_include_directories(gameboy PRIVATE include src)

# ---- SDL2 ----
find_package(SDL2 REQUIRED)

# modern SDL2 configs sometimes expose different vars
if (TARGET SDL2::SDL2)
    target_link_libraries(gameboy PRIVATE SDL2::SDL2)
else()
    target_include_directories(gameboy PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(gameboy PRIVATE ${SDL2_LIBRARIES})
    target_compile_options(gameboy PRIVATE ${SDL2_CFLAGS_OTHER})
endif()

# ---- SDL main handling (IMPORTANT) ----
target_compile_definitions(gameboy PRIVATE SDL_MAIN_HANDLED)

# ---- Windows extra libs (ONLY for MinGW) ----
if (MINGW)
    target_link_libraries(gameboy PRIVATE
        setupapi
        imm32
        version
        winmm
    )

    # optional but recommended
    target_link_options(gameboy PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()

# Release flag
target_compile_definitions(gameboy PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
)