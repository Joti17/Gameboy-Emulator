#include <iostream>
#include <cstdint>
#include <cstring>
#include "memory.h"

#define uint8 uint8_t
#define uint16 uint16_t

Memory::Memory(){
    rom  = memory;
    vram = memory + 0x8000; // 0x8000-0x9FFF
    eram = memory + 0xA000; // 0xA000-0xBFFF (cartridge RAM)
    wram = memory + 0xC000; // 0xC000-0xDFFF (work RAM)
    oam  = memory + 0xFE00; // 0xFE00-0xFE9F (sprites)
    io   = memory + 0xFF00; // 0xFF00-0xFF7F (I/O registers)
    hram = memory + 0xFF80; // 0xFF80-0xFFFE (high RAM)
    ie   = 0;              // 0xFFFF (interrupt enable)
}

uint8 Memory::read8(uint16 addr){
    return memory[addr];
}
uint16 Memory::read16(uint16 addr){
    // little Endian, because the gameboy was designed that way
    return read8(addr) | (read8(addr + 1) << 8);
}

void Memory::write8(uint16 addr, uint8 val){
    // Writes to ROM not accepted
    if (addr < 0x8000) return;
    memory[addr] = val;
}
void Memory::write16(uint16 addr, uint16 val){
    write8(addr, val & 0xFF);
    write8(addr + 1, (val >> 8));
}