#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include <cstdint>
#include <string>
#define uint16 uint16_t
#define uint8 uint8_t

struct Instruction {
    uint16 opcode;
    std::string mnemonic;
    uint8 length = 1;
    uint8 cycles = 4;
};

struct CPU {
    uint8 A, F;
    uint8 B, C;
    uint8 D, E;
    uint8 H, L;

    uint16 SP;
    uint16 PC;

    void reset();
    void step(Instruction inst);

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

Instruction decodeInstruction(uint16 opcode);
Instruction decodeCBInstruction(uint8 opcode);
Instruction decodeNormalInstruction(uint8 opcode);

void set(uint8 bit, uint8 &reg);
void setHL(uint8 bit);

void res(uint8 bit, uint8 &reg);
void resHL(uint8 bit);

void testbit(uint8 bit, uint8 &reg);

void shiftl(uint8 &reg);
void shiftlHL(uint8 bit);

void shiftr(uint8 &reg);
void shiftrHL();

void swap(uint8 &reg);
void swapHL();

extern CPU cpu;

#endif