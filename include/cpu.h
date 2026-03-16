#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include <cstdint>
#define uint16 uint16_t
#define uint8 uint8_t

struct CPU {
    uint8 A, F;
    uint8 B, C;
    uint8 D, E;
    uint8 H, L;

    uint16 SP;
    uint16 PC;

    void reset();
    void step();

    uint16 AF();
    uint16 BC();
    uint16 DE();
    uint16 HL();

    void setAF(uint16 val);
    void setBC(uint16 val);
    void setDE(uint16 val);
    void setHL(uint16 val);

    void addAF(uint16 val);
    void addBC(uint16 val);
    void addDE(uint16 val);
    void addHL(uint16 val);

    void subAF(uint16 val);
    void subBC(uint16 val);
    void subDE(uint16 val);
    void subHL(uint16 val);
    
};

extern CPU cpu;

#endif