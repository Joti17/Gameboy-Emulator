#include <iostream>
#include <cstdint>
#include <cstring>

#define uint8 uint8_t
#define uint16 uint16_t

uint8_t memory[0x10000]; // 64 Kib address space

// Common areas
uint8_t* rom = memory;
uint8_t* vram = memory + 0x8000; // 0x8000-0x9FFF
uint8_t* eram = memory + 0xA000; // 0xA000-0xBFFF (cartridge RAM)
uint8_t* wram = memory + 0xC000; // 0xC000-0xDFFF (work RAM)
uint8_t* oam  = memory + 0xFE00; // 0xFE00-0xFE9F (sprites)
uint8_t* io   = memory + 0xFF00; // 0xFF00-0xFF7F (I/O registers)
uint8_t* hram = memory + 0xFF80; // 0xFF80-0xFFFE (high RAM)
uint8_t  ie    = 0;              // 0xFFFF (interrupt enable)

uint8 read8(uint16 addr){
    // input handling
    return memory[addr];
}

void write8(uint16 addr, uint8 val){
    // Writes to ROM not accepted
    if (addr < 0x8000) return;
    memory[addr] = val;
}

uint16 read16(uint16 addr){
    // little Endian, because the gameboy was designed that way
    return read8(addr) | (read8(addr + 1) << 8);
}

void write16(uint16 addr, uint16 val){
    write8(addr, val & 0xFF);
    write8(addr + 1, (val >> 8));
}