#include <iostream>
#include "memory.h"
#include "cpu.h"
#include "mmu.h"
#include "input.h"
#include "sound.h"
#include <fstream>
#include <vector>
#include <string>

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

    std::cout << "Hello World!" << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0)
    {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        exit(-1);
    }
    SDL_Event e;
    SDL_Window *window = SDL_CreateWindow("Gameboy-Emualtor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 4, 144 * 4, SDL_WINDOW_SHOWN);
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 4096;
    want.callback = [](void *userdata, Uint8 *stream, int len)
    {
        Sound *s = (Sound *)userdata;
        s->fill_buffer((float *)stream, len / sizeof(float));
    };
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    SDL_PauseAudioDevice(dev, 0);


    if (!window)
    {
        fprintf(stderr, "SDL CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    MBC_Controller controller;
    Sound sound{controller};
    want.userdata = &sound;
    Memory memory{sound, 0};
    CPU cpu{memory};
    MMU mmu{memory};
    Input input{memory};

    PPU ppu{memory};
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

    memory.open(rom, *window);

    __uint128_t frame_count = 0;
    bool running = true;
    while (running)
    {
        cpu.step();

        int cycles = cpu.last_instruction_cycles;
        ppu.tick(cycles);
        memory.timer.update(cycles, memory.IF, memory.DIV, memory.TIMA, memory.TMA, memory.TAC);
        sound.update(cycles);

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = false;
            }
            input.HandleKey(e);
        }
    }
    return 0;
}