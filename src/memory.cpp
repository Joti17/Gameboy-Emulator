#include <cstdint>
#include <cstring>
#include "memory.h"
#include <cstdio>

#define uint8 uint8_t
#define uint16 uint16_t


Memory::Memory()
{
    // default values on the DMG
	memset(memory, 0x00, sizeof(memory));

	memory[0xFF10] = 0x80;
	memory[0xFF11] = 0xBF;
	memory[0xFF12] = 0xF3;
	memory[0xFF14] = 0xBF;
	memory[0xFF16] = 0x3F;
	memory[0xFF19] = 0xBF;
	memory[0xFF1A] = 0x7F;
	memory[0xFF1B] = 0xFF;
	memory[0xFF1C] = 0x9F;
	memory[0xFF1E] = 0xBF;
	memory[0xFF20] = 0xFF;
	memory[0xFF23] = 0xBF;
	memory[0xFF24] = 0x77;
	memory[0xFF25] = 0xF3;
	memory[0xFF26] = 0xF1;
	memory[0xFF40] = 0x91;
	memory[0xFF47] = 0xFC;
	memory[0xFF48] = 0xFF;
	memory[0xFF49] = 0xFF;
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
    if (addr < 0x8000) {
        printf("Write to ROM not allowed: %#06X", addr);
    }
    switch(addr){
        default:
            memory[addr] = val;
    }
    
}

void Memory::write16(uint16 addr, uint16 val){
    write8(addr, val & 0xFF);
    write8(addr + 1, (val >> 8));
}

