#ifndef MMU_H
#define MMU_H
#include <cstdint>
#include "memory.h"
#include <SDL2/SDL.h>


#define uint8 uint8_t
#define uint16 uint16_t

struct MMU{
    Memory& memory;
    MMU(Memory &mem);
    uint8 readIO(uint16 addr);
    void writeIO(uint16 addr, uint8 val);
};

#endif