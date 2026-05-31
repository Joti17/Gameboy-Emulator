#pragma once
#include "memory.h"
#include "mmu.h"        // I think is needed in other file with reference to sound
#include "cpu.h"

// class for inheratance
class Channel{
    public:
    double clocksPerMs = 4194.304; 
    bool left = false;
    bool right = false;
    uint8 pace = 0;
    uint8 direction = 0;        // 0; addition 1: subtraction
    uint8 step = 0;
    // new period = last_period +- last_period/ 2^step 
    private:
};

struct Sound{
    MBC_Controller& mbc;
    Memory& memory;
    CPU* cpu;



    const double volumeFactor = 0.125f;
    double volumeL = volumeFactor;
    double volumeR = volumeFactor;
    void write8(uint16 addr, uint8 val);

    bool off;
    void clearSound();
    // sound regsiters addresses + referecnces
    const uint16 a_NR52 = 0xFF26;
    uint8& NR52;
    const uint16 a_NR51 = 0xFF25;
    uint8& NR51;
    Channel CH1;
    Channel CH2;
    Channel CH3;
    Channel CH4;

    const uint16 a_NR50 = 0xFF24;
    uint8& NR50;

    const uint16 a_NR10 = 0xFF10;
    uint8& NR10;
    void scanNR10();
    void updatePeriod(Channel& channel);

    Sound(MBC_Controller& mbc, Memory* mem);
    void setCPU(CPU* cpu);
};