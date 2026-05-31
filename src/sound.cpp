#include "memory.h"
#include "sound.h"
#include "mmu.h"
#include <math.h>

void Sound::write8(uint16 addr, uint8 val)
{
    if ((addr != a_NR52 && off) || false)
        return; // read only while off and replace flase with waveram check
    switch (addr)
    {
    case 0xFF26:
    {
        off = (val & 0x80) != 0;
        clearSound();
        uint8 old = NR52;
        NR52 = val;
        NR52 |= 0b1110000; // 6th-4th always 1s for safety
        NR52 |= (old & 0b1111);
        break;
    }
    case 0xFF25:
        NR51 = val;
        CH4.left = NR51 & 0x80 ? true : false;
        CH3.left = NR51 & 0x40 ? true : false;
        CH2.left = NR51 & 0x20 ? true : false;
        CH1.left = NR51 & 0x10 ? true : false;

        CH4.right = NR51 & 0x8 ? true : false;
        CH3.right = NR51 & 0x4 ? true : false;
        CH2.right = NR51 & 0x2 ? true : false;
        CH1.right = NR51 & 0x1 ? true : false;
        break;
    case 0xFF24:
    {
        // volume
        NR50 = val;
        uint8 lvol = (NR50 >> 4) & 0x4;
        uint8 rvol = (NR50 & 0x4);
        volumeL = (lvol + 1) * volumeFactor;
        volumeR = (rvol + 1) * volumeFactor;
        break;
    }
    case 0xFF10:
    {
        uint8 prev = NR10;
        NR10 = val;
        if (prev == 0)
        {
        }
        // only update the vars after sweep operation is finished
        uint8 t_pace = (val >> 4) & 0x4;
        if (t_pace == 0)
            CH1.pace = 0;
    }
    }
}

void Sound::scanNR10(){
    CH1.pace = (NR10 >> 4) & 0x4;
    CH1.direction = (NR10 >> 3) & 0x1;
    CH1.step = NR10 & 0x4;
}

void Sound::setCPU(CPU* cpu){
    this->cpu = cpu;
}

void Sound::updatePeriod(Channel& channel){
    const uint32_t SWEEP_PERIOD = 4194304/128;
    static uint8 sweep_cycles;
    sweep_cycles += cpu->last_instruction_cycles;
    if (sweep_cycles % SWEEP_PERIOD == 0){

    }
}

void Sound::clearSound()
{
    if (off)
    {
        // reset all registers
    }
}

Sound::Sound(MBC_Controller &mbc, Memory *mem) : mbc(mbc), memory(*mem), NR10(memory.memory[a_NR10]), NR50(memory.memory[a_NR50]), NR51(memory.memory[a_NR51]), NR52(memory.memory[a_NR52]) {}