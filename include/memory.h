#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>

#define uint8 uint8_t
#define uint16 uint16_t

// Read an 8 bit value from memory
uint8 read8(uint16 addr);

// writes 8 bit value to memory
void write8(uint16 addr, uint8 val);

// read 2 8 bit values from memory
uint16 read16(uint16 addr);

// writes 2 8 bit values from memory
void write16(uint16 addr, uint16 val);

#endif