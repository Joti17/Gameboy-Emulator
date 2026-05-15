#include "../include/cpu.h"
#include "../include/sound.h"
#include "../include/memory.h"
#include "../include/mmu.h"
#include "../include/input.h"

#include <iostream>

class TestEmulator
{
public:
    MBC_Controller controller;
    Sound sound;
    Memory memory;
    CPU cpu;
    MMU mmu;
    Input input;
    PPU ppu;

    TestEmulator()
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
};

int main()
{
    TestEmulator gb;

    std::cout << "CPU initialized\n";

    return 0;
}