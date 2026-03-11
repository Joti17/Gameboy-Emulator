#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include <cstdint>

struct CPU {
    uint8_t A, F;
    uint8_t B, C;
    uint8_t D, E;
    uint8_t H, L;

    uint16_t SP;
    uint16_t PC;

    void reset();
    void step();
    uint16_t AF();
    uint16_t BC();
    uint16_t DE();
    uint16_t HL();
};

#endif