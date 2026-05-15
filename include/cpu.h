#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include <cstdint>
#include <string>

#define uint8 uint8_t
#define uint16 uint16_t
#define int8 int8_t
#define int16 int16_t

struct Instruction {
    Instruction(uint16 op, std::string mne, uint8 len = 1, uint8 cycle = 4);
    uint16 opcode;
    std::string mnemonic;
    uint8 length;
    uint8 cycles;
};

struct CPU {
    Memory& memory;
    CPU(Memory& mem);

    uint8 A, F;
    uint8 B, C;
    uint8 D, E;
    uint8 H, L;

    uint32_t clock_speed; // 4.194304 MHz
    uint32_t clocks_this_sec;
    uint8 last_instruction_cycles;

    uint8* rom = nullptr;
    size_t romSize;

    bool halted;
    bool stopped;

    bool interupt_pending;
    bool IME_pending; // for accuracy
    bool IME;

    uint16 SP;
    uint16 PC;

    void reset();
    void step();

    uint16 AF();
    uint16 BC();
    uint16 DE();
    uint16 HL();

    uint8 d8();
    uint16 d16();
    uint16 a8();
    uint16 a16();
    int8 r8();

    void execute(uint16 opcode);


    uint8 RET(uint8 opcode);
    uint8 conRET(bool condition);
    void RETI();

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

    // Flag operations
    void resetZ();
    void resetN();
    void resetH();
    void resetC();

    void setZ();
    void setN();
    void setH();
    void setC();

    bool getZ();
    bool getN();
    bool getH();
    bool getC();

    // Bit operations
    void set(uint8 bit, uint8 &reg);
    void setiHL(uint8 bit);

    void res(uint8 bit, uint8 &reg);
    void resHL(uint8 bit);

    void testbit(uint8 bit, uint8 reg);

    void shiftl(uint8 &reg);
    void shiftlHL(uint8 bit);

    void shiftr(uint8 &reg);
    void shiftrHL();

    void swap(uint8 &reg);
    void swapHL();

    void slaHL();
    void srlHL();
    void sla(uint8& reg);
    void sra(uint8 &reg);
    void sraHL();
    void srl(uint8 &reg);

    void rr(uint8 &reg);
    void rrHL();

    void rl(uint8 &reg);
    void rlHL();

    void rrc(uint8 &reg);
    void rrcHL();

    void rlc(uint8 &reg);
    void rlcHL();

    void rst(uint8 n);

    void cp(uint8 reg);
    void cpHL();

    void updateZ(uint8 reg);
    
    void EI();

    void ld(uint8 opcode);

    void OR(uint8 byte);
    void XOR(uint8 byte);
    void AND(uint8 byte);

    void PUSH(uint16 d_reg);
    void POP(uint8 &regH, uint8 &regL);

    void DI();

    uint8 conJP(bool condition, uint16 addr);
    uint8 JP(uint8 opcode);

    void addSP(uint8 val);
    void ADD(uint8 val);
    void ADC(uint8 val);
    void INC(uint8 &reg);
    void INCHL();
    void INCSP();
    void INC16(uint8 &high, uint8 &low);

    uint8 conCALL(bool condition, uint16 addr);
    uint8 CALL(uint8 opcode);
    uint8 conJR(bool condition, int8 offset);
    uint8 JR(uint8 opcode);

    void SBC(uint8 val);
    void SUB(uint8 val);
    void DEC(uint8 &reg);
    void DECHL();
    void DECSP();
    void DEC16(uint8 &high, uint8 &low);
    
    void CCF();

    void HALT();

    void CPL();

    void SCF();

    void DAA();

    void RRA();
    void RRCA();

    void RLA();
    void RLCA();

    void STOP();
    void checkInterrupts();
};

Instruction decodeInstruction(uint16 opcode);
Instruction decodeCBInstruction(uint8 opcode);
Instruction decodeNormalInstruction(uint8 opcode);

#endif