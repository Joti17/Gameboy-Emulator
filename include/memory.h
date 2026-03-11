#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>

// Read an 8 bit value from memory
uint8_t read8(uint16_t addr);

// writes 8 bit value to memory
void write8(uint16_t addr, uint8_t val);

// read 2 8 bit values from memory
uint16_t read16(uint16_t addr);

// writes 2 8 bit values from memory
void write16(uint16_t addr, uint16_t val);

#endif