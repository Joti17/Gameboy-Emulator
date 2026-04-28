#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include "sound.h"
#include "ppu.h"
#include <fstream>

#define uint8 uint8_t
#define uint16 uint16_t

#define MBC1 0x01
#define MCB1R 0x02		// R means RAM
#define MCB1RB 0x03		// B means Battery

#define MBC2 0x05
#define MBC2B 0x06

#define RR 0x08			// ROM + RAM
#define RRB 0x09

#define MBC3TB 0x0F 	// T = Timer
#define MBC3TRB	0x10	
#define MBC3 0x11
#define MBC3R 0x12
#define MBC3RB 0x13

#define MBC4 0x15
#define MBC4R 0x16
#define MBC4RB 0x17

#define MBC5 0x19
#define MBC5R 0x1A
#define MBC5RB 0x1B

#define MBC5RUM 0x1C	// RUM = Rumble
#define MBC5RUMR 0x1D
#define MBC5RUMRB 0x1E

struct TimerState {
    uint16 internal_counter = 0;
    uint8 tima = 0;
    uint8 tma = 0;
    uint8 tac = 0;
    bool interupt_requested = false;
    bool enabled = true;
    uint32_t div_counter = 0;
    uint32_t timer_counter = 0;

    TimerState();

    void update(int cycles, uint8& IF, uint8& DIV_reg, uint8& TIMA_reg, uint8& TMA_reg, uint8& TAC_reg);
};



struct Memory{
    uint8_t memory[0x10000]; // 64 Kib address space
    TimerState timer;
    Sound sound;
    
    uint8 mbc;
    MBC_Controller controller;
    uint8* romData;
    size_t size;

    Memory(Sound &sound, uint8 mbc);

    uint8 &P1 = memory[0xFF00];
    uint8 &SB = memory[0xFF01];
    uint8 &SC = memory[0xFF02];
    uint8 &DIV = memory[0xFF04];
    uint8 &TIMA = memory[0xFF05];
    uint8 &TMA = memory[0xFF06];
    uint8 &TAC = memory[0xFF07];
    uint8 &IF = memory[0xFF0F];
    uint8 &NR10 = memory[0xFF10];

    uint8 &STAT = memory[0xFF41];
    uint8 &LY = memory[0xFF44];
    uint8 &LYC = memory[0xFF45];
 

    uint8 timerEnabled;
    uint16 frequency = 4096;

    uint8 read8(uint16 addr);
    uint16 read16(uint16 addr);

    void write8(uint16 addr, uint8 val);
    void write16(uint16 addr, uint16 val);

    void tickTimers(uint32_t cycles);
    void open(std::ifstream& rom, SDL_Window& window);
};


#endif