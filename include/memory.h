#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>

#define uint8 uint8_t
#define uint16 uint16_t

struct Memory{
    uint8_t memory[0x10000]; // 64 Kib address space
    uint8_t* rom;
    uint8_t* vram;
    uint8_t* eram;
    uint8_t* wram;
    uint8_t* oam;
    uint8_t* io;
    uint8_t* hram;
    uint8_t  ie;
    
    Memory();           // init func


    uint8 read8(uint16 addr);
    uint16 read16(uint16 addr);

    void write8(uint16 addr, uint8 val);
    void write16(uint16 addr, uint16 val);
};

#endif