#include <iostream>
#include "memory.h"
#include "cpu.h"
#include "mmu.h"
#include "input.h"
#include "sound.h"
#include <fstream>
#include <vector>
#include <string>
#include "logger.h"
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char **argv)
{
    std::ifstream rom;
    bool rom_opened;

    std::vector<std::string> args;
    for (int i = 0; i < argc; i++)
    {
        args.emplace_back(argv[i]);
    }

    std::string correctUsage = "./gameboy [-r|--help] <rom.gb>";

    if (argc <= 1)
    {
        std::cerr << "Correct Usage: " << correctUsage << std::endl;
        return -1;
    }

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-r" && i + 1 < argc)
        {
            std::string rom_path = argv[i + 1];
            rom.open(rom_path, std::ios::binary);
            if (!rom)
            {
                std::cerr << "Invalid path " << rom_path << std::endl;
                return -1;
            }
            rom_opened = true;
            i++;
        }
        else if (arg != "--help")
        {
            if (!rom_opened)
            {
                std::cerr << "Missing ROM path. Usage: " << correctUsage << std::endl;
                return -1;
            }
        }
    }

    if (!rom_opened && argc > 1 && std::string(argv[1]) != "--help")
    {
        std::cerr << "ROM file must be specified with -r flag. Usage: " << correctUsage << std::endl;
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0)
    {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        exit(-1);
    }
    SDL_Event e;
    SDL_Window *window = SDL_CreateWindow("Game Boy Emulator",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          160 * 3, 144 * 3,
                                          SDL_WINDOW_SHOWN);

    if (!window)
    {
        fprintf(stderr, "SDL CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }
    
    MBC_Controller controller;

    Memory memory{0, controller};
    Sound sound = memory.sound;


    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer accelerated failed: " << SDL_GetError() << std::endl;
        std::cerr << "Falling back to software renderer." << std::endl;
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Texture *ppuTexture = SDL_CreateTexture(renderer,
                                                SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                160, 144);
    if (!ppuTexture)
    {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    memory.open(rom, *window);

    CPU cpu{memory};
    sound.setCPU(&cpu);
    MMU mmu{memory};
    Input input{memory};

    PPU ppu{memory};

    uint64_t frame_count = 0;
    bool running = true;
    bool insideVBlank = false;
    uint32 frameClocks = 0;
    const uint32 clocksPerFrame = 70224;

    while (running)
    {
        while (frameClocks < clocksPerFrame)
        {
            cpu.step();

            int cycles = cpu.last_instruction_cycles;

            for (int i = 0; i < cycles; ++i)
            {
                ppu.tick(1);
                memory.timer.update(1, memory.IF, memory.DIV, memory.TIMA, memory.TMA, memory.TAC);
            }

            frameClocks += cycles;
        }

        SDL_RenderClear(renderer);
        ppu.drawToScreen(renderer, ppuTexture);
        SDL_RenderPresent(renderer);

        frame_count++;

        frameClocks -= clocksPerFrame;

        if (frameClocks < clocksPerFrame / 2)
            SDL_Delay(1);

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;
            input.HandleKey(e);

            // F1 to switch layout
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F1)
                input.switchLayout();
        }
    }
    SDL_DestroyTexture(ppuTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}