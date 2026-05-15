#pragma once

#include "../include/cpu.h"
#include "../include/sound.h"
#include "../include/memory.h"
#include "../include/mmu.h"
#include "../include/input.h"

class BaseTest
{
public:
    MBC_Controller controller;
    Sound sound;
    Memory memory;
    CPU cpu;
    MMU mmu;
    Input input;
    PPU ppu;

    BaseTest()
        : sound(controller),
          memory(sound, 0),
          cpu(memory),
          mmu(memory),
          input(memory),
          ppu(memory)
    {
    }

    void step()
    {
        cpu.step();

        int cycles = cpu.last_instruction_cycles;

        ppu.tick(cycles);

        memory.timer.update(
            cycles,
            memory.IF,
            memory.DIV,
            memory.TIMA,
            memory.TMA,
            memory.TAC);

        sound.update(cycles);
    }

    void step_cycles(int count)
    {
        for (int i = 0; i < count; i++)
        {
            step();
        }
    }
};