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

// #define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

int main(int argc, char **argv)
{
    SDL_SetMainReady();
    std::string bios_path = "";
    std::ifstream rom;
    bool rom_opened = false;
    bool bios_enabled = false;
    bool save_enabled = false;
    std::string save_path = "";
    std::string rom_path = "";
    uint32 turbo = 1;

    std::vector<std::string> args;
    for (int i = 0; i < argc; i++)
    {
        args.emplace_back(argv[i]);
    }

    std::string correctUsage = "./gameboy [-r|--rom] <rom.gb> [-b|--bios] <bios.bin> [-s|--save] <save.sav> [-sp|--speed] <speed>";

    if (argc <= 1)
    {
        std::cerr << "Correct Usage: " << correctUsage << std::endl;
        return -1;
    }

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if ((arg == "-r" || arg == "--rom") && i + 1 < argc)
        {
            rom_path = argv[i + 1];
            rom.open(rom_path, std::ios::binary);
            if (!rom)
            {
                std::cerr << "Invalid ROM path: " << rom_path << std::endl;
                return -1;
            }
            rom_opened = true;
            i++;
        }
        else if ((arg == "-b" || arg == "--bios") && i + 1 < argc)
        {
            bios_path = argv[i + 1];
            bios_enabled = true;
            i++;
        }
        else if ((arg == "-s" || arg == "--save") && i + 1 < argc)
        {
            save_path = argv[i + 1];
            save_enabled = true;
            i++;
        }
        else if ((arg == "-sp" || arg == "--speed") && i + 1 < argc)
        {
            turbo = static_cast<uint32>(std::stoul(argv[i + 1]));
            if (turbo >= 10)
            {
                std::cerr << "turbo won't work well with that speed. " << std::endl;
            }
        }
        else if (arg == "-b" || arg == "-s")
        {
            std::cerr << "Error: " << arg << " flag requires a path." << std::endl;
            return -1;
        }
        else if (arg == "--help")
        {
            std::cout << correctUsage << std::endl;
            return 0;
        }
        else if (arg[0] == '-')
        {
            std::cerr << "Unknown flag: " << arg << std::endl;
            std::cerr << "Usage: " << correctUsage << std::endl;
            return -1;
        }
        else if (!rom_opened)
        {
            rom.open(arg, std::ios::binary);
            if (!rom)
            {
                std::cerr << "Invalid ROM path: " << arg << std::endl;
                return -1;
            }
            rom_opened = true;
        }
    }

    if (!rom_opened)
    {
        std::cerr << "ROM file must be specified with -r flag. Usage: " << correctUsage << std::endl;
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0)
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

    if (save_path.empty())
    {
        std::filesystem::path p(rom_path);
        p.replace_extension(".sav");
        save_path = p.string();
        save_enabled = true;
    }

    Memory memory{0, controller, bios_path, save_path, save_enabled};
    if (!bios_path.empty())
        memory.biosEnabled = true;
    else
        memory.biosEnabled = false;

    Sound &sound = memory.sound;

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
    uint32 frameClocks = 0;
    const uint32 clocksPerFrame = 70224;

    uint32 lastSaveTime = SDL_GetTicks();
    uint32 interval = 60000; // 60000ms=60s
    // TODO: add turbo mode
    bool turboMode = (turbo > 1);
    uint32 turboMultiplier = turbo;

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

            sound.tick(cycles);

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
            if (e.type == SDL_QUIT){
                memory.saveGame();
                running = false;
            }
            input.HandleKey(e);

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F1)
                input.switchLayout();
        }

        uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastSaveTime >= interval)
        {
            memory.saveGame();
            lastSaveTime = currentTime;
        }
    }


    SDL_DestroyTexture(ppuTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}