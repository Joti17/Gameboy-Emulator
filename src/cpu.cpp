#include <cstdint>
#include "memory.h"
#include <string>
#include "cpu.h"
#include <cstdio>
#include <iomanip>
#include <iostream>
#include "logger.h"

// tmp
#include <thread>
#include <chrono>

#define uint8 uint8_t
#define uint16 uint16_t
#define int8 int8_t
#define int16 int16_t

// global PC for logging from other modules
uint16_t g_cpu_pc = 0;
/*
#define true 1
#define false 0
#define bool int8_t
*/

uint8 nor(uint8 a, uint8 b)
{
    return ~(a | b);
}

uint16 nor(uint16 a, uint16 b)
{
    return ~(a | b);
}


CPU::CPU(Memory &mem)
    : memory(mem),
      clock_speed(4194304),
      clocks_this_sec(0),
      A(0x01), F(0xB0), B(0x00), C(0x13), D(0x00), E(0xD8), H(0x01), L(0x4D),
            SP(0xFFFE), PC(0x0000), IME(false), interupt_pending(false), halted(false), stopped(false),
      last_loop_pc(0xFFFF), same_pc_count(0)
{
        if (memory.biosEnabled)
                PC = 0x0000;
        else
                PC = 0x0100;
}

// setZ conditionally
void CPU::updateZ(uint8 reg)
{
    if (reg == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
}

void CPU::reset()
{
    /*
    A(0x01), F(0xB0), B(0x00), C(0x13), D(0x00), E(0xD8), H(0x01), L(0x4D),
      SP(0xFFFE), PC(0x100)
    */
    this->A = 0x01;
    this->F = 0xB0;

    this->B = 0x00;
    this->C = 0x13;

    this->D = 0x00;
    this->E = 0xD8;

    this->H = 0x01;
    this->L = 0x4D;

    if (memory.biosEnabled)
        PC = 0x0000;
    else
        PC = 0x100;
    SP = 0xFFFE; // grows downwards

    this->IME = false;
    this->interupt_pending = false;

    this->halted = false;
    this->stopped = false;
}

void CPU::checkInterrupts()
{
    uint8 IF = memory.read8(0xFF0F);
    uint8 IE = memory.read8(0xFFFF);
    uint8 fired = IF & IE;

    if (fired)
    {
        if (halted)  halted = false;
        if (stopped) stopped = false;
    }

    if (!IME || !fired)
        return;

    IME = false;

    for (int i = 0; i < 5; ++i)
    {
        if (fired & (1 << i))
        {
            memory.write8(0xFF0F, IF & ~(1 << i));

            PUSH(PC);

            switch (i)
            {
            case 0: PC = 0x40; break; // VBlank
            case 1: PC = 0x48; break; // LCD STAT
            case 2: PC = 0x50; break; // Timer
            case 3: PC = 0x58; break; // Serial
            case 4: PC = 0x60; break; // Joypad
            }

            clocks_this_sec += 20;
            return;
        }
    }
}

void CPU::step() {
    uint8_t cycles_spent = 0;

    if (this->halted) {
        cycles_spent = 4; 
        this->clocks_this_sec += cycles_spent;
        this->last_instruction_cycles = cycles_spent;

        uint8_t ie = memory.read8(0xFFFF);
        uint8_t interrupt_flags = memory.read8(0xFF0F);
        if ((ie & interrupt_flags & 0x1F) != 0) {
            this->halted = false;
        }
        checkInterrupts();
        return;
    }

    if (this->IME && (memory.read8(0xFFFF) & memory.read8(0xFF0F) & 0x1F) != 0) {
        checkInterrupts(); 
        return; 
    }

    uint16_t current_pc = this->PC;
    g_cpu_pc = current_pc;

    uint8_t first_byte = memory.read8(current_pc);
    uint16_t full_opcode = first_byte;
    
    if (first_byte == 0xCB) {
        uint8_t second_byte = memory.read8(current_pc + 1);
        full_opcode = (static_cast<uint16_t>(first_byte) << 8) | second_byte;
    }

    Instruction inst = decodeInstruction(full_opcode); 

    this->last_instruction_cycles = inst.cycles;

    uint16_t oldPC = this->PC;
    execute(full_opcode); 

    if (this->PC == oldPC) {
        if (this->halt_bug_triggered) {
            this->halt_bug_triggered = false; 
        } else {
            this->PC += inst.length;
        }
    }

    this->clocks_this_sec += this->last_instruction_cycles;

    if (this->IME_pending) {
        this->IME = true;
        this->IME_pending = false;
    }
    if (this->enabling_ime) {
        this->IME_pending = true;
        this->enabling_ime = false;
    }

    checkInterrupts();
}

// returns cycles
void CPU::execute(uint16 opcode)
{
    if ((opcode & 0xFF00) == 0xCB00)
    {
        uint8* reg = nullptr;
        uint8 cb = opcode & 0xFF;
        uint8 reg_idx = cb & 0x07;

        switch(reg_idx){
            // right + left funcs
            case 0:
               reg = &B;
               break;
            case 1:
                reg = &C;
                break;
            case 2:
                reg = &D;
                break;
            case 3:
                reg = &E;
                break;
            case 4:
                reg = &H;
                break;
            case 5:
                reg = &L;
                break;
            case 6:
                // special HL func
                break;
            case 7:
                reg = &A;
                break;
        }
        switch(cb){
            case 0x06:
                CPU::rlcHL();
                return;
            case 0x16:
                CPU::rlHL();
                return;
            case 0x26:
                CPU::slaHL();
                return;
            case 0x36:
                CPU::swapHL();
                return;
            case 0x46:
                CPU::testbit(0, memory.read8(HL()));
                return;
            case 0x56:
                CPU::testbit(2, memory.read8(HL()));
                return;
            case 0x66:
                CPU::testbit(4, memory.read8(HL()));
                return;
            case 0x76:
                CPU::testbit(6, memory.read8(HL()));
                return;
            case 0x86:
                CPU::resHL(0);
                return;
            case 0x96:
                CPU::resHL(2);
                return;
            case 0xA6:
                CPU::resHL(4);
                return;
            case 0xB6:
                CPU::resHL(6);
                return;
            case 0xC6:
                CPU::setHL(0);
                return;
            case 0xD6:
                CPU::setHL(2);
                return;
            case 0xE6:
                CPU::setHL(4);
                return;
            case 0xF6:
                CPU::setHL(6);
                return;
            case 0x0E:
                CPU::rrcHL();
                return;
            case 0x1E:
                CPU::rrHL();
                return;
            case 0x2E:
                CPU::sraHL();
                return;
            case 0x3E:
                CPU::srlHL();
                return;
            case 0x4E:
                CPU::testbit(1, memory.read8(HL()));
                return;
            case 0x5E:
                CPU::testbit(3, memory.read8(HL()));
                return;
            case 0x6E:
                CPU::testbit(5, memory.read8(HL()));
                return;
            case 0x7E:
                CPU::testbit(7, memory.read8(HL()));
                return;
            case 0x8E:
                CPU::resHL(1);
                return;
            case 0x9E:
                CPU::resHL(3);
                return;
            case 0xAE:
                CPU::resHL(5);
                return;
            case 0xBE:
                CPU::resHL(7);
                return;
            case 0xCE:
                CPU::setHL(static_cast<uint8>(1));
                return;
            case 0xDE:
                CPU::setHL(static_cast<uint8>(3));
                return;
            case 0xEE:
                CPU::setHL(static_cast<uint8>(5));
                return;
            case 0xFE:
                CPU::setHL(static_cast<uint8>(7));
                return;
        }

        if (reg == nullptr){
            g_logger.log("Reg is nullptr opcode: 0x{:02X}", opcode);
            return;
        }

        uint8 type = (cb >> 6) & 0x03;
        uint8 bit = (cb >> 3) & 0x07;
        switch(type){
            case 0: 
                switch(bit){
                    case 0: rlc(*reg); break;
                    case 1: rrc(*reg); break;
                    case 2: rl(*reg);  break;
                    case 3: rr(*reg);  break;
                    case 4: sla(*reg); break;
                    case 5: sra(*reg); break;
                    case 6: swap(*reg); break;
                    case 7: srl(*reg); break;
                }
                break;
            case 1: testbit(bit, *reg); break;
            case 2: res(bit, *reg); break;
            case 3: set(bit, *reg); break;
        }
        return;
    }
    else
    {
        // normal Instructions
        switch (opcode)
        {
        case 0x00:
            return;
        case 0x01:
        case 0x02:
            ld(opcode);
            return;
        case 0x03:
            INC16(B, C);
            return;
        case 0x04:
            INC(B);
            return;
        case 0x05:
            DEC(B);
            return;
        case 0x06:
            ld(opcode);
            return;
        case 0x07:
            RLCA();
            return;
        case 0x08:
            ld(opcode);
            return;
        case 0x09:
            addHL(BC());
            return;
        case 0x0A:
            ld(opcode);
            return;
        case 0x0B:
            DEC16(B, C);
            return;
        case 0x0C:
            INC(C);
            return;
        case 0x0D:
            DEC(C);
            return;
        case 0x0E:
            ld(opcode);
            return;
        case 0x0F:
            RRCA();
            return;
        case 0x10:
            STOP();
            return;
        case 0x11:
            ld(opcode);
            return;
        case 0x12:
            ld(opcode);
            return;
        case 0x13:
            INC16(D, E);
            return;
        case 0x14:
            INC(D);
            return;
        case 0x15:
            DEC(D);
            return;
        case 0x16:
            ld(opcode);
            return;
        case 0x17:
            RLA();
            return;
        case 0x18:
            JR(static_cast<uint8>(opcode));
            return;
        case 0x19:
            addHL(DE());
            return;
        case 0x1A:
            ld(opcode);
            return;
        case 0x1B:
            DEC16(D, E);
            return;
        case 0x1C:
            INC(E);
            return;
        case 0x1D:
            DEC(E);
            return;
        case 0x1E:
            ld(opcode);
            return;
        case 0x1F:
            RRA();
            return;
        case 0x20:
            {
                uint8 cycles = JR(opcode) - 8;
                clocks_this_sec += cycles;
                return;
            }
        case 0x21:
            ld(opcode);
            return;
        case 0x22:
            ld(opcode);
            return;
        case 0x23:
            INC16(H, L);
            return;
        case 0x24:
            INC(H);
            return;
        case 0x25:
            DEC(H);
            return;
        case 0x26:
            ld(opcode);
            return;
        case 0x27:
            DAA();
            return;
        case 0x28:
            {
                uint8 cycles = JR(opcode) - 8;
                clocks_this_sec += cycles;
                return;
            }
        case 0x29:
            addHL(HL());
            return;
        case 0x2A:
            ld(opcode);
            return;
        case 0x2B:
            DEC16(H, L);
            return;
        case 0x2C:
            INC(L);
            return;
        case 0x2D:
            DEC(L);
            return;
        case 0x2E:
            ld(opcode);
            return;
        case 0x2F:
            CPL();
            return;
        case 0x30:
            {
                uint8 cycles = JR(opcode) - 8;
                clocks_this_sec += cycles;
                return;
            }
        case 0x31:
        case 0x32:
            ld(opcode);
            return;
        case 0x33:
            INCSP();
            return;
        case 0x34:
            INCHL();
            return;
        case 0x35:
            DECHL();
            return;
        case 0x36:
            ld(opcode);
            return;
        case 0x37:
            SCF();
            return;
        case 0x38:
            {
                uint8 cycles = JR(opcode) - 8;
                clocks_this_sec += cycles;
                return;
            }
        case 0x39:
            addHL(SP);
            return;
        case 0x3A:
            ld(opcode);
            return;
        case 0x3B:
            DECSP();
            return;
        case 0x3C:
            INC(A);
            return;
        case 0x3D:
            DEC(A);
            return;
        case 0x3E:
            ld(opcode);
            return;
        case 0x3F:
            CCF();
            return;
        case 0x40 ... 0x7F:
        {
            if (opcode == 0x76)
            {
                HALT();
                return;
            }

            uint8 src = opcode & 0x07;
            uint8 dst = (opcode >> 3) & 0x07;

            uint8 value = 0;
            switch (src) {
                case 0: value = B; break;
                case 1: value = C; break;
                case 2: value = D; break;
                case 3: value = E; break;
                case 4: value = H; break;
                case 5: value = L; break;
                case 6: value = memory.read8(HL()); break;
                case 7: value = A; break;
            }

            switch (dst) {
                case 0: B = value; break;
                case 1: C = value; break;
                case 2: D = value; break;
                case 3: E = value; break;
                case 4: H = value; break;
                case 5: L = value; break;
                case 6: memory.write8(HL(), value); break;
                case 7: A = value; break;
            }
            break;
        }
        case 0x80:
            ADD(B);
            return;
        case 0x81:
            ADD(C);
            return;
        case 0x82:
            ADD(D);
            return;
        case 0x83:
            ADD(E);
            return;
        case 0x84:
            ADD(H);
            return;
        case 0x85:
            ADD(L);
            return;
        case 0x86:
            ADD(memory.read8(HL()));
            return;
        case 0x87:
            ADD(A);
            return;
        case 0x88:
            ADC(B);
            return;
        case 0x89:
            ADC(C);
            return;
        case 0x8A:
            ADC(D);
            return;
        case 0x8B:
            ADC(E);
            return;
        case 0x8C:
            ADC(H);
            return;
        case 0x8D:
            ADC(L);
            return;
        case 0x8E:
            ADC(memory.read8(HL()));
            return;
        case 0x8F:
            ADC(A);
            return;
        case 0x90:
            SUB(B);
            return;
        case 0x91:
            SUB(C);
            return;
        case 0x92:
            SUB(D);
            return;
        case 0x93:
            SUB(E);
            return;
        case 0x94: 
            SUB(H);
            return;
        case 0x95:
            SUB(L);
            return;
        case 0x96:
            SUB(memory.read8(HL()));
            return;
        case 0x97:
            SUB(A);
            return;
        case 0x98:
            SBC(B);
            return;
        case 0x99:
            SBC(C);
            return;
        case 0x9A:
            SBC(D);
            return;
        case 0x9B:
            SBC(E);
            return;
        case 0x9C:
            SBC(H);
            return;
        case 0x9D:
            SBC(L);
            return;
        case 0x9E:
            SBC(memory.read8(HL()));
            return;
        case 0x9F:
            SBC(A);
            return;
        case 0xA0:
            AND(B);
            return;
        case 0xA1:
            AND(C);
            return;
        case 0xA2:
            AND(D);
            return;
        case 0xA3:
            AND(E);
            return;
        case 0xA4:
            AND(H);
            return;
        case 0xA5:
            AND(L);
            return;
        case 0xA6:
            AND(memory.read8(HL()));
            return;
        case 0xA7:
            AND(A);
            return;
        case 0xA8:
            XOR(B);
            return;
        case 0xA9:
            XOR(C);
            return;
        case 0xAA:
            XOR(D);
            return;
        case 0xAB:
            XOR(E);
            return;
        case 0xAC:
            XOR(H);
            return;
        case 0xAD:
            XOR(L);
            return;
        case 0xAE:
            XOR(memory.read8(HL()));
            return;
        case 0xAF:
            XOR(A);
            return;
        case 0xB0:
            OR(B);
            return;
        case 0xB1:
            OR(C);
            return;
        case 0xB2:
            OR(D);
            return;
        case 0xB3:
            OR(E);
            return;
        case 0xB4:
            OR(H);
            return;
        case 0xB5:
            OR(L);
            return;
        case 0xB6:
            OR(memory.read8(HL()));
            return;
        case 0xB7:
            OR(A);
            return;
        case 0xB8:
            cp(B);
            return;
        case 0xB9:
            cp(C);
            return;
        case 0xBA:
            cp(D);
            return;
        case 0xBB:
            cp(E);
            return;
        case 0xBC:
            cp(H);
            return;
        case 0xBD:
            cp(L);
            return;
        case 0xBE:
            cpHL();
            return;
        case 0xBF:
            cp(A);
            return;
        case 0xC0:
            {
                uint8 cycles = RET(0xC0) - 8;
                clocks_this_sec += cycles;
                return;
            }
        case 0xC1:
            POP(B, C);
            return;
        case 0xC2:
            {
                uint8 cycles = JP(opcode) - 12;
                clocks_this_sec += cycles;
                return;
            }
        case 0xC3:
            JP(opcode);
            return;
        case 0xC4:
            {
                uint8 cycles = CALL(opcode) - 12;
                clocks_this_sec += cycles;
                return;
            }
        case 0xC5:
            PUSH(BC());
            return;
        case 0xC6:
            ADD(d8());
            return;
        case 0xC7:
            rst(0);
            return;
        case 0xC8:
            {
                uint8 cycles = RET(opcode) - 8;
                clocks_this_sec += cycles;
                return;
            }
        case 0xC9:
            RET(opcode);
            return;
        case 0xCA:
            {
                uint8 cycles = JP(opcode) - 12;
                clocks_this_sec += cycles;
                return;
            }
        // CB prefix
        case 0xCC:
            {
                uint8 cycles = CALL(opcode) - 12;
                clocks_this_sec += cycles;
                return;
            }
        case 0xCD:
            conCALL(true, d16());
            return;
        case 0xCE:
            ADC(d8());
            return;
        case 0xCF:
            rst(1);
            return;
        case 0xD0:
            {
                uint8 cycles = RET(opcode) - 8;
                clocks_this_sec += cycles;
                return;
            }
        case 0xD1:
            POP(D, E);
            return;
        case 0xD2:
            {
                uint8 cycles = JP(opcode) - 12;
                clocks_this_sec += cycles;
                return;
            }
        case 0xD4:
            {
                uint8 cycles = CALL(opcode) - 12;
                clocks_this_sec += cycles;
                return;
            }
        case 0xD5:
            PUSH(DE());
            return;
        case 0xD6:
            SUB(d8());
            return;
        case 0xD7:
            rst(2);
            return;
        case 0xD8:
            {
                uint8 cycles = RET(opcode) - 8;
                clocks_this_sec += cycles;
                return;
            }
        case 0xD9:
            RETI();
            return;
        case 0xDA:
            {
                uint8 cycles = JP(opcode) - 12;
                clocks_this_sec += cycles;
                return;
            }
        case 0xDC:
            {
                uint8 cycles = CALL(opcode) - 12;
                clocks_this_sec += cycles;
                return;
            }
        case 0xDE:
            SBC(d8());
            return;
        case 0xDF:
            rst(3);
            return;
        case 0xE0: 
            ld(opcode);
            return;
        case 0xE1:
            POP(H, L);
            return;
        case 0xE2:
            ld(opcode);
            return;
        case 0xE5:
            PUSH(HL());
            return;
        case 0xE6:
            AND(d8());
            return;
        case 0xE7:
            rst(4);
            return;
        case 0xE8:
            addSP((int8)d8());
            return;
        case 0xE9:
            JP(opcode);
            return;
        case 0xEA:
            ld(opcode);
            return;
        case 0xEE:
            XOR(d8());
            return;
        case 0xEF:
            rst(5);
            return;
        case 0xF0:
            ld(opcode);
            return;
        case 0xF1:
            POP(A, F);
            return;
        case 0xF2:
            ld(opcode);
            return;
        case 0xF3:
            DI();
            return;
        case 0xF5:
            PUSH(AF());
            return;
        case 0xF6:
            OR(d8());
            return;
        case 0xF7:
            rst(6);
            return;
        case 0xF8:
            ld(opcode);
            return;
        case 0xF9:
            ld(opcode);
            return;
        case 0xFA:
            ld(opcode);
            return;
        case 0xFB:
            EI();
            return;
        case 0xFE:
            cp(d8());
            return;
        case 0xFF:
            rst(7);
            return;
        default:
            std::cout << "Invalid opcode: " << std::hex << std::uppercase << std::showbase << std::setw(6) << std::setfill('0') << opcode << "\n";
            g_logger.log("Invalid opcode: 0x{:06X}", opcode);
            return;
        }
        
    }
}


uint16 CPU::AF() { return (A << 8) | F; }
uint16 CPU::BC() { return (B << 8) | C; }
uint16 CPU::DE() { return (D << 8) | E; }
uint16 CPU::HL() { return (H << 8) | L; }
void CPU::setAF(uint16 val)
{
    A = val >> 8;
    F = val & 0xF0;             // lower 4 bits of F are always 0
}

void CPU::setBC(uint16 val)
{
    B = val >> 8;
    C = val & 0xFF;
}

void CPU::setDE(uint16 val)
{
    D = val >> 8;
    E = val & 0xFF;
}

void CPU::setHL(uint16 val)
{
    H = val >> 8;
    L = val & 0xFF;
}

void CPU::addAF(uint16 val)
{
    uint16 result = AF() + val;
    setAF(result);
}

void CPU::addBC(uint16 val)
{
    uint16 result = BC() + val;
    setBC(result);
}

void CPU::addDE(uint16 val)
{
    uint16 result = DE() + val;
    setDE(result);
}

void CPU::addHL(uint16 val)
{
    uint16 hl = HL();
    uint32 result = hl + val;

    resetN();

    if (((hl & 0x0FFF) + (val & 0x0FFF)) > 0x0FFF)
        setH();
    else
        resetH();

    if (result > 0xFFFF)
        setC();
    else
        resetC();

    setHL(static_cast<uint16>(result));
}

void CPU::subAF(uint16 val)
{
    uint16 result = AF() - val;
    setAF(result);
}

void CPU::subBC(uint16 val)
{
    uint16 result = BC() - val;
    setBC(result);
}

void CPU::subDE(uint16 val)
{
    uint16 result = DE() - val;
    setDE(result);
}

void CPU::subHL(uint16 val)
{
    uint16 result = HL() - val;
    setHL(result);
}

void CPU::resetZ() { F &= ~0x80; } // Clear Z flag (bit 7)
void CPU::resetN() { F &= ~0x40; } // Clear N flag (bit 6)
void CPU::resetH() { F &= ~0x20; } // Clear H flag (bit 5)
void CPU::resetC() { F &= ~0x10; } // Clear C flag (bit 4)

void CPU::setZ() { F |= 0x80; } // Set Z flag (bit 7)
void CPU::setN() { F |= 0x40; } // Set N flag (bit 6)
void CPU::setH() { F |= 0x20; } // Set H flag (bit 5)
void CPU::setC() { F |= 0x10; } // Set C flag (bit 4)

bool CPU::getZ() { return F & 0x80; } // Zero flag
bool CPU::getN() { return F & 0x40; } // Subtract flag
bool CPU::getH() { return F & 0x20; } // Half-carry flag
bool CPU::getC() { return F & 0x10; } // Carry flag

uint8 CPU::d8()
{
    return memory.read8(PC + 1);
}

uint16 CPU::d16()
{
    return memory.read16(PC + 1);
}

uint16 CPU::a8()
{
    return 0xFF00 | memory.read8(PC + 1);
}

uint16 CPU::a16()
{
    return memory.read16(PC + 1);
}

int8 CPU::r8()
{
    // location of jump -128 to +127
    // note: +0 thats why only to +127
    return (int8)memory.read8(PC + 1);
}

#pragma region CB

// SET B, X
// X = X | (1 << B)
void CPU::set(uint8 bit, uint8 &reg)
{
    reg |= (1 << bit);
}

void CPU::setiHL(uint8 bit)
{
    // set instruction for HL
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);
    val |= (1 << bit);
    memory.write8(addr, val);
}

void CPU::res(uint8 bit, uint8 &reg)
{
    reg &= ~(1 << bit);
}
void CPU::resHL(uint8 bit)
{
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);
    val &= ~(1 << bit);
    memory.write8(addr, val);
}
void CPU::testbit(uint8 bit, uint8 reg)
{
    this->setH();   // H flag always set
    this->resetN(); // N flag always cleared
    if (!(reg & (1 << bit)))
        this->setZ(); // Set Z if bit is 0
    else
        this->resetZ(); // Clear Z if bit is 1
}

void CPU::shiftl(uint8 &reg)
{
    uint8 MSB = (reg >> 7);
    if (MSB)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
    reg = (reg << 1);
    if (reg == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    this->resetN();
    this->resetH();
}
void CPU::shiftlHL(uint8 bit)
{
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);
    uint8 MSB = (val >> 7);
    if (MSB)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
    val = (val << 1);
    if (val == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    this->resetN();
    this->resetH();
    memory.write8(addr, val);
}
void CPU::shiftr(uint8 &reg)
{
    uint8 LSB = (reg & 1);
    if (LSB)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
    reg = (reg >> 1);
    if (reg == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    this->resetN();
    this->resetH();
}
void CPU::shiftrHL()
{
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);
    uint8 LSB = (val & 1);
    if (LSB)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
    val = (val >> 1);
    if (val == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    this->resetN();
    this->resetH();
    memory.write8(addr, val);
}

void CPU::swap(uint8 &reg)
{
    this->resetC();
    this->resetH();
    this->resetN();

    uint8 lower = reg & 0x0F;
    uint8 upper = (reg >> 4) & 0x0F;
    reg = (lower << 4) | upper;

    if (reg == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
}

void CPU::swapHL()
{
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);
    this->resetC();
    this->resetH();
    this->resetN();

    uint8 lower = val & 0x0F;
    uint8 upper = (val >> 4) & 0x0F;
    val = (lower << 4) | upper;

    if (val == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    memory.write8(addr, val);
}


uint8 CPU::RET(uint8 opcode) {
    switch(opcode) {
        case 0xC0: return conRET(!getZ());
        case 0xC8: return conRET(getZ()); 
        case 0xD0: return conRET(!getC()); 
        case 0xD8: return conRET(getC());  
        case 0xC9: return conRET(true);    
    }
    return 0;
}

void CPU::RETI(){
    PC = memory.read16(SP);
    SP += 2;

    IME = true;
}

uint8 CPU::conRET(bool condition) {
    if (condition) {
        uint16 addr = memory.read16(SP);
        SP += 2;
        PC = addr;
        return 20; 
    }
    return 8;
}



void CPU::sra(uint8 &reg)
{
    // Shift right arithmetic
    // shift right for signed int
    // 1100 0000 => 1010 0000
    uint8 msb = reg & 0x80;
    uint8 lsb = reg & 0x01;
    reg = (reg >> 1) | msb;
    // Flags
    if (reg == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    if (lsb == 0)
    {
        this->resetC();
    }
    else
    {
        this->setC();
    }
    this->resetN();
    this->resetH();
}

void CPU::sraHL()
{
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);
    uint8 msb = val & 0x80;
    uint8 lsb = val & 0x01;
    val = (val >> 1) | msb;
    // Flags
    if (val == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    if (lsb == 0)
    {
        this->resetC();
    }
    else
    {
        this->setC();
    }
    this->resetN();
    this->resetH();
    memory.write8(addr, val);
}

void CPU::rr(uint8 &reg)
{
    // rotate right, MSB = C
    uint8 carry = (this->F & 0x10) ? 1 : 0; // C flag
    uint8 new_carry = reg & 1;

    reg = (reg >> 1);
    reg = (reg | (carry << 7));

    this->resetN();
    this->resetH();

    if (reg == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    if (new_carry)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
}

void CPU::rrHL()
{
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);

    uint8 carry = (this->F & 0x10) ? 1 : 0; // C flag
    uint8 new_carry = val & 1;

    val = (val >> 1);
    val = (val | (carry << 7));

    this->resetN();
    this->resetH();

    if (val == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    if (new_carry)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
    memory.write8(addr, val);
}

void CPU::rl(uint8 &reg)
{
    uint8 carry = (this->F & 0x10) ? 1 : 0; // C flag
    uint8 new_carry = (reg >> 7) & 1;

    reg = (reg << 1);
    reg = (reg | carry);

    this->resetN();
    this->resetH();

    if (reg == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    if (new_carry)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
}

void CPU::rlHL()
{
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);

    uint8 carry = (this->F & 0x10) ? 1 : 0; // C flag
    uint8 new_carry = (val >> 7) & 1;

    val = (val << 1);
    val = (val | carry);

    this->resetN();
    this->resetH();

    if (val == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    if (new_carry)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
    memory.write8(addr, val);
}

void CPU::sla(uint8& reg) {
    bool carryOut = (reg & 0x80) != 0;

    reg <<= 1;

    if (reg == 0) this->setZ(); else this->resetZ();
    
    this->resetN();
    this->resetH();
    
    if (carryOut) this->setC(); else this->resetC();
}

void CPU::srlHL() {
    uint8_t val = memory.read8(HL());

    bool carryOut = (val & 0x01) != 0;

    val >>= 1;

    if (val == 0) setZ(); else resetZ();
    
    resetN();
    resetH();
    
    if (carryOut) setC(); else resetC();

    memory.write8(HL(), val);
}

void CPU::srl(uint8 &reg)
{
    uint8 lsb = reg & 1;
    reg >>= 1;

    lsb ? setC() : resetC();
    resetN();
    resetH();
    updateZ(reg);
}

void CPU::slaHL() {
    uint8_t val = memory.read8(HL());

    bool carryOut = (val & 0x80) != 0;

    val <<= 1;

    if (val == 0) setZ(); else resetZ();
    
    resetN(); 
    resetH(); 
    
    if (carryOut) setC(); else resetC();

    memory.write8(HL(), val);
}

void CPU::rrc(uint8 &reg)
{
    // Z 0 0 C
    // 0100 0001 -> 1010 0000
    this->resetN();
    this->resetH();

    uint8 LSB = reg & 1;
    if (LSB)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
    reg = (reg >> 1) | (LSB << 7);

    if (reg == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
}

void CPU::rrcHL()
{
    // Z 0 0 C
    // 0100 0001 -> 1010 0000
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);

    this->resetN();
    this->resetH();

    uint8 LSB = val & 1;
    if (LSB)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
    val = (val >> 1) | (LSB << 7);

    if (val == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    memory.write8(addr, val);
}

void CPU::rlc(uint8 &reg)
{
    // 1000 0000 -> 0000 0001
    this->resetN();
    this->resetH();

    uint8 MSB = (reg >> 7) & 1;
    reg = (reg << 1) | MSB;
    if (MSB)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }

    if (reg == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
}

void CPU::rlcHL()
{
    // 1000 0000 -> 0000 0001
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);

    this->resetN();
    this->resetH();

    uint8 MSB = (val >> 7) & 1;
    val = (val << 1) | MSB;
    if (MSB)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
    if (val == 0)
    {
        this->setZ();
    }
    else
    {
        this->resetZ();
    }
    memory.write8(addr, val);
}
#pragma endregion

#pragma region Normal

void CPU::rst(uint8 n)
{
    this->SP -= 2;
    memory.write16(this->SP, this->PC+1);
    this->PC = n*8;
}

void CPU::cp(uint8 reg)
{
    // Z 1 H C
    // A - reg
    uint8 res = this->A - reg;

    this->updateZ(res);
    this->setN();
    if ((this->A & 0x0F) < (reg & 0x0F))
    {
        this->setH();
    }
    else
    {
        this->resetH();
    }
    if (this->A < reg)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
}

void CPU::cpHL()
{
    // Z 1 H C
    // A - (HL)
    uint16 addr = this->HL();
    uint8 val = memory.read8(addr);

    uint8 res = this->A - val;

    this->updateZ(res);
    this->setN();
    if ((this->A & 0x0F) < (val & 0x0F))
    {
        this->setH();
    }
    else
    {
        this->resetH();
    }
    if (this->A < val)
    {
        this->setC();
    }
    else
    {
        this->resetC();
    }
}

void CPU::EI()
{
    this->IME_pending = true;
}

void CPU::ld(uint8 opcode)
{
    switch (opcode)
    {
    case 0x01: setBC(d16()); break;
    case 0x02: memory.write8(BC(), A); break;
    case 0x06: B = d8(); 
        std::cerr << "CPU: LD B,d8 executed at PC=0x" << std::hex << std::uppercase << PC << std::dec 
                  << " value=0x" << std::hex << (int)B << std::dec << "\n";
        break;
    case 0x08: memory.write16(a16(), SP); break;
    case 0x0A: A = memory.read8(BC()); break;
    case 0x0E: C = d8(); break;
    case 0x11: setDE(d16()); break;
    case 0x12: memory.write8(DE(), A); break;
    case 0x16: D = d8(); break;
    case 0x1A: A = memory.read8(DE()); break;
    case 0x1E: E = d8(); break;
    case 0x21: setHL(d16()); break;
    case 0x22: memory.write8(HL(), A); addHL(1); break;
    case 0x26: H = d8(); break;
    case 0x2A: A = memory.read8(HL()); addHL(1); break;
    case 0x2E: L = d8(); break;
    case 0x31: SP = d16(); break;
    case 0x32: memory.write8(HL(), A); subHL(1); break;
    case 0x36: memory.write8(HL(), d8()); break;
    case 0x3A: A = memory.read8(HL()); subHL(1); break;
    case 0x3E: A = d8(); break;

    // LDH / special
    case 0xE0: memory.write8(a8(), A); break;
    case 0xE2: memory.write8(0xFF00 | C, A); break;
    case 0xEA: memory.write8(a16(), A); break;
    case 0xF0: A = memory.read8(a8()); break;
    case 0xF2: A = memory.read8(0xFF00 | C); break;
    case 0xF8: {
        int8 offset = (int8)memory.read8(PC + 1);
        resetZ(); resetN();
        if (((SP & 0x0F) + (offset & 0x0F)) > 0x0F) setH(); else resetH();
        if (((SP & 0xFF) + (offset & 0xFF)) > 0xFF) setC(); else resetC();
        setHL(SP + offset);
        break;
    }
    case 0xF9: SP = HL(); break;
    case 0xFA: A = memory.read8(a16()); break;

    case 0x40 ... 0x7F:
    {
        if (opcode == 0x76) {
            HALT();
            return;
        }
        uint8 src = opcode & 0x07;
        uint8 dst = (opcode >> 3) & 0x07;

        uint8 value = 0;
        switch (src) {
            case 0: value = B; break;
            case 1: value = C; break;
            case 2: value = D; break;
            case 3: value = E; break;
            case 4: value = H; break;
            case 5: value = L; break;
            case 6: value = memory.read8(HL()); break;
            case 7: value = A; break;
        }

        switch (dst) {
            case 0: B = value; break;
            case 1: C = value; break;
            case 2: D = value; break;
            case 3: E = value; break;
            case 4: H = value; break;
            case 5: L = value; break;
            case 6: memory.write8(HL(), value); break;
            case 7: A = value; break;
        }
        break;
    }

    default:
        g_logger.log("Invalid LD opcode: 0x{:02X} at PC=0x{:04X}", opcode, PC);
        std::cout << "Invalid LD opcode: 0x" << std::hex << (int)opcode << std::dec << "\n";
        break;
    }
}

void CPU::OR(uint8 byte)
{
    A |= byte;
    updateZ(A);
    resetN();
    resetH();
    resetC();
}

void CPU::PUSH(uint16 d_reg)
{
    SP -= 2;
    memory.write16(SP, d_reg);
}

void CPU::DI()
{
    IME = false;
}

void CPU::POP(uint8 &regH, uint8 &regL)
{
    regL = memory.read8(SP);
    SP++;
    regH = memory.read8(SP);
    SP++;
}

void CPU::XOR(uint8 byte)
{
    A ^= byte;
    updateZ(A);
    resetC();
    resetH();
    resetN();
}

uint8 CPU::conJP(bool condition, uint16 addr)
{
    // Returns clocks
    if (condition)
    {
        PC = addr;
        return 16;
    }
    return 12;
}

uint8 CPU::JP(uint8 opcode)
{
    switch (opcode)
    {
    case 0xC3:
        return conJP(true, d16()); // JP nn
    case 0xC2:
        return conJP(!(F & 0x80), d16()); // JP NZ,nn
    case 0xCA:
        return conJP(F & 0x80, d16()); // JP Z,nn
    case 0xD2:
        return conJP(!(F & 0x10), d16()); // JP NC,nn
    case 0xDA:
        return conJP(F & 0x10, d16()); // JP C,nn
    case 0xE9:
        PC = HL();
        return 4; // JP (HL)
    }
    return 1;
}

void CPU::addSP(int8 val)
{
    uint16 oldSP = SP;
    int16_t signed_offset = static_cast<int16_t>(val);
    SP = static_cast<uint16_t>(static_cast<int32_t>(oldSP) + signed_offset);

    // Flags: Z = 0, N = 0
    F = 0;

    uint8_t uoffset = static_cast<uint8_t>(val);
    if (((oldSP & 0x0F) + (uoffset & 0x0F)) > 0x0F)
        setH();
    else
        resetH();

    if (((oldSP & 0xFF) + uoffset) > 0xFF)
        setC();
    else
        resetC();
}

void CPU::AND(uint8 byte)
{
    A &= byte;
    updateZ(A);
    resetN();
    setH();
    resetC();
}

uint8 CPU::conCALL(bool condition, uint16 addr)
{
    // return value means CPU clocks
    if (condition)
    {
        SP -= 2;
        memory.write16(SP, PC + 3);
        PC = addr;
        return 24;
    }
    return 12;
}

uint8 CPU::CALL(uint8 opcode)
{
    // wrapper for CPU::CALL
    switch (opcode)
    {
    case 0xCD:
        return conCALL(true, d16()); // CALL nn
    case 0xC4:
        return conCALL(!(F & 0x80), d16()); // CALL NZ,nn
    case 0xCC:
        return conCALL(F & 0x80, d16()); // CALL Z,nn
    case 0xD4:
        return conCALL(!(F & 0x10), d16()); // CALL NC,nn
    case 0xDC:
        return conCALL(F & 0x10, d16()); // CALL C,nn
    default:
        std::cerr << "Something went wrong in Line: " << __LINE__ << " CALL func wrapper: " << opcode << std::endl;
        return 4;
    }
}

void CPU::SBC(uint8 val)
{
    // val or reg
    uint8 a = A;
    uint16 result = A - val - (uint8)getC();
    A = result & 0xFF;
    updateZ(A);
    setN();
    ((a & 0xF) < ((val & 0xF) + getC())) ? setH() : resetH();
    (a < (uint16)val + getC()) ? setC() : resetC();
}

void CPU::SUB(uint8 val)
{
    uint8 a = A;
    A = (A - val) & 0xFF;
    updateZ(A);
    setN();
    ((a & 0xF) < (val & 0xF)) ? setH() : resetH();
    (a < val) ? setC() : resetC();
}





void CPU::ADD(uint8 val)
{
    uint8 a = A;
    uint16 result = A + val;
    A = result & 0xFF;
    updateZ(A);
    resetN();
    ((a & 0xF) + (val & 0xF) > 0xF) ? setH() : resetH();
    (result > 0xFF) ? setC() : resetC();
}

void CPU::ADC(uint8 val)
{
    uint8 a = A;
    uint16 result = A + val + (uint8)getC();
    A = result & 0xFF;
    updateZ(A);
    resetN();
    ((a & 0xF) + (val & 0xF) + getC() > 0xF) ? setH() : resetH();
    (result > 0xFF) ? setC() : resetC();
}

void CPU::CCF()
{
    resetN();
    resetH();
    getC() ? resetC() : setC();
}

void CPU::INC(uint8 &reg)
{
    uint8 a = reg;
    reg++;
    updateZ(reg);
    resetN();
    ((a & 0xF) + 1 > 0xF) ? setH() : resetH();
}

void CPU::DEC(uint8 &reg)
{
    uint8 a = reg;
    reg--;
    updateZ(reg);
    setN();
    ((a & 0xF) == 0) ? setH() : resetH();
    // Debug: trace decrements for key registers during boot memory-clear loops
    if (&reg == &B || &reg == &C || &reg == &D || &reg == &E || &reg == &H || &reg == &L || &reg == &A)
    {
        g_logger.log("CPU: DEC reg at PC=0x{:04X} new=0x{:02X} Z={} H={} N={} C={}", PC, static_cast<int>(reg), getZ(), getH(), getN(), getC());
    }
}

void CPU::INCHL()
{
    uint16 addr = HL();
    uint8 val = memory.read8(addr);
    uint8 a = val;
    val++;
    memory.write8(addr, val);

    updateZ(val);
    resetN();
    ((a & 0xF) + 1 > 0xF) ? setH() : resetH();
}

void CPU::INC16(uint8 &high, uint8 &low)
{
    // INC DOUBLE REG
    uint16 val = ((uint16)high << 8) | low;
    val++;
    high = (val >> 8) & 0xFF;
    low = val & 0xFF;
}

void CPU::INCSP()
{
    SP++;
}

void CPU::DECHL()
{
    uint16 addr = HL();
    uint8 val = memory.read8(addr);
    uint8 a = val;
    val--;
    memory.write8(addr, val);

    updateZ(val);
    setN();
    ((a & 0xF) == 0) ? setH() : resetH();
}

void CPU::DEC16(uint8 &high, uint8 &low)
{
    // DEC DOUBLE REG
    uint16 val = ((uint16)high << 8) | low;
    val--;
    high = (val >> 8) & 0xFF;
    low = val & 0xFF;
}

void CPU::DECSP()
{
    SP--;
}
void CPU::HALT()
{
    halted = true;
}

void CPU::SCF()
{
    setC();
    resetN();
    resetH();
}

void CPU::CPL()
{
    A = ~A;
    setN();
    setH();
}

uint8 CPU::conJR(bool condition, int8 offset)
{
    if (condition)
    {
        PC += static_cast<int16_t>(offset) + 2;
        this->last_instruction_cycles += 4;
        this->clocks_this_sec += 4;
        return 12;
    }
    return 8;
}

uint8 CPU::JR(uint8 opcode)
{
    switch (opcode)
    {
    case 0x18:
        return conJR(true, r8()); // JR r8
    case 0x20:
        return conJR(!getZ(), r8()); // JR NZ,r8
    case 0x28:
        return conJR(getZ(), r8()); // JR Z,r8
    case 0x30:
        return conJR(!getC(), r8()); // JR NC,r8
    case 0x38:
        return conJR(getC(), r8()); // JR C,r8
    }
    std::cerr << "Something went wrong in CPU::JR()\n";
    return 8;
}

void CPU::DAA() {
    uint8 correction = 0;

    if (getH() || (!getN() && (A & 0x0F) > 9)) {
        correction |= 0x06;
    }
    if (getC() || (!getN() && A > 0x99)) {
        correction |= 0x60;
        setC();
    } else {
        resetC();
    }

    if (getN()) {
        A -= correction;
    } else {
        A += correction;
    }

    updateZ(A);
    resetH();
}

void CPU::RRCA()
{
    bool bit0 = A & 1;
    A = (A >> 1) | (bit0 << 7);
    resetZ();
    resetN();
    resetH();
    bit0 ? setC() : resetC();
}

void CPU::RRA()
{
    bool bit0 = A & 1;
    A = (A >> 1) | (getC() << 7);
    resetZ();
    resetN();
    resetH();
    bit0 ? setC() : resetC();
}

void CPU::RLCA()
{
    uint8 bit7 = (A >> 7) & 1;
    A = (A << 1) | bit7;
    resetZ(); resetN(); resetH();
    bit7 ? setC() : resetC();
}

void CPU::RLA()
{
    bool bit7 = A & 0x80;
    A = (A << 1) | getC();
    resetZ();
    resetN();
    resetH();
    bit7 ? setC() : resetC();
}

void CPU::STOP()
{
    stopped = true;
}

// lol

#pragma endregion

#pragma region Instructions

Instruction::Instruction(uint16 op, std::string mne, uint8 len, uint8 cycle)
    : opcode(op), mnemonic(mne), length(len), cycles(cycle) {}

Instruction decodeInstruction(uint16 opcode)
{
    
    if ((opcode & 0xFF00) == 0xCB00)
    {
        uint8 cb_opcode = opcode & 0xFF;
        return decodeCBInstruction(cb_opcode);
    }
    else
    {
        uint8 normal_opcode = opcode & 0xFF;
        return decodeNormalInstruction(normal_opcode);
    }
}

Instruction decodeNormalInstruction(uint8 opcode)
{
    switch (opcode)
    {
    case 0x00:
        return Instruction{opcode, "NOP"};
    case 0x01:
        return Instruction{opcode, "LD BC, d16", 3, 12};
    case 0x02:
        return Instruction{opcode, "LD (BC), A", 1, 8};
    case 0x03:
        return Instruction{opcode, "INC BC", 1, 8};
    case 0x04:
        return Instruction{opcode, "INC B"};
    case 0x05:
        return Instruction{opcode, "DEC B"};
    case 0x06:
        return Instruction{opcode, "LD B, d8", 2, 8};
    case 0x07:
        return Instruction{opcode, "RLCA"};
    case 0x08:
        return Instruction{opcode, "LD (a16), SP", 3, 20};
    case 0x09:
        return Instruction{opcode, "ADD HL, BC", 1, 8};
    case 0x0A:
        return Instruction{opcode, "LD A, (BC)", 1, 8};
    case 0x0B:
        return Instruction{opcode, "DEC BC", 1, 8};
    case 0x0C:
        return Instruction{opcode, "INC C"};
    case 0x0D:
        return Instruction{opcode, "DEC C"};
    case 0x0E:
        return Instruction{opcode, "LD C, d8", 2, 8};
    case 0x0F:
        return Instruction{opcode, "RRCA"};

    case 0x10:
        return Instruction{opcode, "STOP", 2, 4};
    case 0x11:
        return Instruction{opcode, "LD DE, d16", 3, 12};
    case 0x12:
        return Instruction{opcode, "LD (DE), A", 1, 8};
    case 0x13:
        return Instruction{opcode, "INC DE", 1, 8};
    case 0x14:
        return Instruction{opcode, "INC D"};
    case 0x15:
        return Instruction{opcode, "DEC D"};
    case 0x16:
        return Instruction{opcode, "LD D, d8", 2, 8};
    case 0x17:
        return Instruction{opcode, "RLA"};
    case 0x18:
        return Instruction{opcode, "JR r8", 2, 12};
    case 0x19:
        return Instruction{opcode, "ADD HL, DE", 1, 8};
    case 0x1A:
        return Instruction{opcode, "LD A, (DE)", 1, 8};
    case 0x1B:
        return Instruction{opcode, "DEC DE", 1, 8};
    case 0x1C:
        return Instruction{opcode, "INC E"};
    case 0x1D:
        return Instruction{opcode, "DEC E"};
    case 0x1E:
        return Instruction{opcode, "LD E, d8", 2, 8};
    case 0x1F:
        return Instruction{opcode, "RRA"};

    case 0x20:
        return Instruction{opcode, "JR NZ, r8", 2, 12};
    case 0x21:
        return Instruction{opcode, "LD HL, d16", 3, 12};
    case 0x22:
        return Instruction{opcode, "LD (HL+), A", 1, 8};
    case 0x23:
        return Instruction{opcode, "INC HL", 1, 8};
    case 0x24:
        return Instruction{opcode, "INC H"};
    case 0x25:
        return Instruction{opcode, "DEC H"};
    case 0x26:
        return Instruction{opcode, "LD H, d8", 2, 8};
    case 0x27:
        return Instruction{opcode, "DAA"};
    case 0x28:
        return Instruction{opcode, "JR Z, r8", 2, 12};
    case 0x29:
        return Instruction{opcode, "ADD HL, HL", 1, 8};
    case 0x2A:
        return Instruction{opcode, "LD A, (HL+)", 1, 8};
    case 0x2B:
        return Instruction{opcode, "DEC HL", 1, 8};
    case 0x2C:
        return Instruction{opcode, "INC L"};
    case 0x2D:
        return Instruction{opcode, "DEC L"};
    case 0x2E:
        return Instruction{opcode, "LD L, d8", 2, 8};
    case 0x2F:
        return Instruction{opcode, "CPL"};

    case 0x30:
        return Instruction{opcode, "JR NC, r8", 2, 12};
    case 0x31:
        return Instruction{opcode, "LD SP, d16", 3, 12};
    case 0x32:
        return Instruction{opcode, "LD (HL-), A", 1, 8};
    case 0x33:
        return Instruction{opcode, "INC SP", 1, 8};
    case 0x34:
        return Instruction{opcode, "INC (HL)", 1, 12};
    case 0x35:
        return Instruction{opcode, "DEC (HL)", 1, 12};
    case 0x36:
        return Instruction{opcode, "LD (HL), d8", 2, 12};
    case 0x37:
        return Instruction{opcode, "SCF"};
    case 0x38:
        return Instruction{opcode, "JR C, r8", 2, 12};
    case 0x39:
        return Instruction{opcode, "ADD HL, SP", 1, 8};
    case 0x3A:
        return Instruction{opcode, "LD A, (HL-)", 1, 8};
    case 0x3B:
        return Instruction{opcode, "DEC SP", 1, 8};
    case 0x3C:
        return Instruction{opcode, "INC A"};
    case 0x3D:
        return Instruction{opcode, "DEC A"};
    case 0x3E:
        return Instruction{opcode, "LD A, d8", 2, 8};
    case 0x3F:
        return Instruction{opcode, "CCF"};

    case 0x40:
        return Instruction{opcode, "LD B, B"};
    case 0x41:
        return Instruction{opcode, "LD B, C"};
    case 0x42:
        return Instruction{opcode, "LD B, D"};
    case 0x43:
        return Instruction{opcode, "LD B, E"};
    case 0x44:
        return Instruction{opcode, "LD B, H"};
    case 0x45:
        return Instruction{opcode, "LD B, L"};
    case 0x46:
        return Instruction{opcode, "LD B, (HL)", 1, 8};
    case 0x47:
        return Instruction{opcode, "LD B, A"};

    case 0x48:
        return Instruction{opcode, "LD C, B"};
    case 0x49:
        return Instruction{opcode, "LD C, C"};
    case 0x4A:
        return Instruction{opcode, "LD C, D"};
    case 0x4B:
        return Instruction{opcode, "LD C, E"};
    case 0x4C:
        return Instruction{opcode, "LD C, H"};
    case 0x4D:
        return Instruction{opcode, "LD C, L"};
    case 0x4E:
        return Instruction{opcode, "LD C, (HL)", 1, 8};
    case 0x4F:
        return Instruction{opcode, "LD C, A"};

    case 0x50:
        return Instruction{opcode, "LD D, B"};
    case 0x51:
        return Instruction{opcode, "LD D, C"};
    case 0x52:
        return Instruction{opcode, "LD D, D"};
    case 0x53:
        return Instruction{opcode, "LD D, E"};
    case 0x54:
        return Instruction{opcode, "LD D, H"};
    case 0x55:
        return Instruction{opcode, "LD D, L"};
    case 0x56:
        return Instruction{opcode, "LD D, (HL)", 1, 8};
    case 0x57:
        return Instruction{opcode, "LD D, A"};

    case 0x58:
        return Instruction{opcode, "LD E, B"};
    case 0x59:
        return Instruction{opcode, "LD E, C"};
    case 0x5A:
        return Instruction{opcode, "LD E, D"};
    case 0x5B:
        return Instruction{opcode, "LD E, E"};
    case 0x5C:
        return Instruction{opcode, "LD E, H"};
    case 0x5D:
        return Instruction{opcode, "LD E, L"};
    case 0x5E:
        return Instruction{opcode, "LD E, (HL)", 1, 8};
    case 0x5F:
        return Instruction{opcode, "LD E, A"};

    case 0x60:
        return Instruction{opcode, "LD H, B"};
    case 0x61:
        return Instruction{opcode, "LD H, C"};
    case 0x62:
        return Instruction{opcode, "LD H, D"};
    case 0x63:
        return Instruction{opcode, "LD H, E"};
    case 0x64:
        return Instruction{opcode, "LD H, H"};
    case 0x65:
        return Instruction{opcode, "LD H, L"};
    case 0x66:
        return Instruction{opcode, "LD H, (HL)", 1, 8};
    case 0x67:
        return Instruction{opcode, "LD H, A"};

    case 0x68:
        return Instruction{opcode, "LD L, B"};
    case 0x69:
        return Instruction{opcode, "LD L, C"};
    case 0x6A:
        return Instruction{opcode, "LD L, D"};
    case 0x6B:
        return Instruction{opcode, "LD L, E"};
    case 0x6C:
        return Instruction{opcode, "LD L, H"};
    case 0x6D:
        return Instruction{opcode, "LD L, L"};
    case 0x6E:
        return Instruction{opcode, "LD L, (HL)", 1, 8};
    case 0x6F:
        return Instruction{opcode, "LD L, A"};

    case 0x70:
        return Instruction{opcode, "LD (HL), B", 1, 8};
    case 0x71:
        return Instruction{opcode, "LD (HL), C", 1, 8};
    case 0x72:
        return Instruction{opcode, "LD (HL), D", 1, 8};
    case 0x73:
        return Instruction{opcode, "LD (HL), E", 1, 8};
    case 0x74:
        return Instruction{opcode, "LD (HL), H", 1, 8};
    case 0x75:
        return Instruction{opcode, "LD (HL), L", 1, 8};
    case 0x76:
        return Instruction{opcode, "HALT"};
    case 0x77:
        return Instruction{opcode, "LD (HL), A", 1, 8};

    case 0x78:
        return Instruction{opcode, "LD A, B"};
    case 0x79:
        return Instruction{opcode, "LD A, C"};
    case 0x7A:
        return Instruction{opcode, "LD A, D"};
    case 0x7B:
        return Instruction{opcode, "LD A, E"};
    case 0x7C:
        return Instruction{opcode, "LD A, H"};
    case 0x7D:
        return Instruction{opcode, "LD A, L"};
    case 0x7E:
        return Instruction{opcode, "LD A, (HL)", 1, 8};
    case 0x7F:
        return Instruction{opcode, "LD A, A"};
    case 0x80:
        return Instruction{opcode, "ADD A, B"};
    case 0x81:
        return Instruction{opcode, "ADD A, C"};
    case 0x82:
        return Instruction{opcode, "ADD A, D"};
    case 0x83:
        return Instruction{opcode, "ADD A, E"};
    case 0x84:
        return Instruction{opcode, "ADD A, H"};
    case 0x85:
        return Instruction{opcode, "ADD A, L"};
    case 0x86:
        return Instruction{opcode, "ADD A, (HL)", 1, 8};
    case 0x87:
        return Instruction{opcode, "ADD A, A"};

    case 0x88:
        return Instruction{opcode, "ADC A, B"};
    case 0x89:
        return Instruction{opcode, "ADC A, C"};
    case 0x8A:
        return Instruction{opcode, "ADC A, D"};
    case 0x8B:
        return Instruction{opcode, "ADC A, E"};
    case 0x8C:
        return Instruction{opcode, "ADC A, H"};
    case 0x8D:
        return Instruction{opcode, "ADC A, L"};
    case 0x8E:
        return Instruction{opcode, "ADC A, (HL)", 1, 8};
    case 0x8F:
        return Instruction{opcode, "ADC A, A"};

    case 0x90:
        return Instruction{opcode, "SUB B"};
    case 0x91:
        return Instruction{opcode, "SUB C"};
    case 0x92:
        return Instruction{opcode, "SUB D"};
    case 0x93:
        return Instruction{opcode, "SUB E"};
    case 0x94:
        return Instruction{opcode, "SUB H"};
    case 0x95:
        return Instruction{opcode, "SUB L"};
    case 0x96:
        return Instruction{opcode, "SUB (HL)", 1, 8};
    case 0x97:
        return Instruction{opcode, "SUB A"};

    case 0x98:
        return Instruction{opcode, "SBC A, B"};
    case 0x99:
        return Instruction{opcode, "SBC A, C"};
    case 0x9A:
        return Instruction{opcode, "SBC A, D"};
    case 0x9B:
        return Instruction{opcode, "SBC A, E"};
    case 0x9C:
        return Instruction{opcode, "SBC A, H"};
    case 0x9D:
        return Instruction{opcode, "SBC A, L"};
    case 0x9E:
        return Instruction{opcode, "SBC A, (HL)", 1, 8};
    case 0x9F:
        return Instruction{opcode, "SBC A, A"};

    case 0xA0:
        return Instruction{opcode, "AND B"};
    case 0xA1:
        return Instruction{opcode, "AND C"};
    case 0xA2:
        return Instruction{opcode, "AND D"};
    case 0xA3:
        return Instruction{opcode, "AND E"};
    case 0xA4:
        return Instruction{opcode, "AND H"};
    case 0xA5:
        return Instruction{opcode, "AND L"};
    case 0xA6:
        return Instruction{opcode, "AND (HL)", 1, 8};
    case 0xA7:
        return Instruction{opcode, "AND A"};

    case 0xA8:
        return Instruction{opcode, "XOR B"};
    case 0xA9:
        return Instruction{opcode, "XOR C"};
    case 0xAA:
        return Instruction{opcode, "XOR D"};
    case 0xAB:
        return Instruction{opcode, "XOR E"};
    case 0xAC:
        return Instruction{opcode, "XOR H"};
    case 0xAD:
        return Instruction{opcode, "XOR L"};
    case 0xAE:
        return Instruction{opcode, "XOR (HL)", 1, 8};
    case 0xAF:
        return Instruction{opcode, "XOR A"};

    case 0xB0:
        return Instruction{opcode, "OR B"};
    case 0xB1:
        return Instruction{opcode, "OR C"};
    case 0xB2:
        return Instruction{opcode, "OR D"};
    case 0xB3:
        return Instruction{opcode, "OR E"};
    case 0xB4:
        return Instruction{opcode, "OR H"};
    case 0xB5:
        return Instruction{opcode, "OR L"};
    case 0xB6:
        return Instruction{opcode, "OR (HL)", 1, 8};
    case 0xB7:
        return Instruction{opcode, "OR A"};

    case 0xB8:
        return Instruction{opcode, "CP B"};
    case 0xB9:
        return Instruction{opcode, "CP C"};
    case 0xBA:
        return Instruction{opcode, "CP D"};
    case 0xBB:
        return Instruction{opcode, "CP E"};
    case 0xBC:
        return Instruction{opcode, "CP H"};
    case 0xBD:
        return Instruction{opcode, "CP L"};
    case 0xBE:
        return Instruction{opcode, "CP (HL)", 1, 8};
    case 0xBF:
        return Instruction{opcode, "CP A"};

    case 0xC0:
        return Instruction{opcode, "RET NZ", 1, 8};
    case 0xC1:
        return Instruction{opcode, "POP BC", 1, 12};
    case 0xC2:
        return Instruction{opcode, "JP NZ, a16", 3, 12};
    case 0xC3:
        return Instruction{opcode, "JP a16", 3, 16};
    case 0xC4:
        return Instruction{opcode, "CALL NZ, a16", 3, 12};
    case 0xC5:
        return Instruction{opcode, "PUSH BC", 1, 16};
    case 0xC6:
        return Instruction{opcode, "ADD A, d8", 2, 8};
    case 0xC7:
        return Instruction{opcode, "RST 00H", 1, 16};

    case 0xC8:
        return Instruction{opcode, "RET Z", 1, 8};
    case 0xC9:
        return Instruction{opcode, "RET", 1, 16};
    case 0xCA:
        return Instruction{opcode, "JP Z, a16", 3, 12};
    case 0xCC:
        return Instruction{opcode, "CALL Z, a16", 3, 12};
    case 0xCD:
        return Instruction{opcode, "CALL a16", 3, 24};
    case 0xCE:
        return Instruction{opcode, "ADC A, d8", 2, 8};
    case 0xCF:
        return Instruction{opcode, "RST 08H", 1, 16};

    case 0xD0:
        return Instruction{opcode, "RET NC", 1, 8};
    case 0xD1:
        return Instruction{opcode, "POP DE", 1, 12};
    case 0xD2:
        return Instruction{opcode, "JP NC, a16", 3, 12};
    case 0xD4:
        return Instruction{opcode, "CALL NC, a16", 3, 12};
    case 0xD5:
        return Instruction{opcode, "PUSH DE", 1, 16};
    case 0xD6:
        return Instruction{opcode, "SUB d8", 2, 8};
    case 0xD7:
        return Instruction{opcode, "RST 10H", 1, 16};

    case 0xD8:
        return Instruction{opcode, "RET C", 1, 8};
    case 0xD9:
        return Instruction{opcode, "RETI", 1, 16};
    case 0xDA:
        return Instruction{opcode, "JP C, a16", 3, 12};
    case 0xDC:
        return Instruction{opcode, "CALL C, a16", 3, 12};
    case 0xDE:
        return Instruction{opcode, "SBC A, d8", 2, 8};
    case 0xDF:
        return Instruction{opcode, "RST 18H", 1, 16};

    case 0xE0:
        return Instruction{opcode, "LDH (a8), A", 2, 12};
    case 0xE1:
        return Instruction{opcode, "POP HL", 1, 12};
    case 0xE2:
        return Instruction{opcode, "LD (C), A", 1, 8};
    case 0xE5:
        return Instruction{opcode, "PUSH HL", 1, 16};
    case 0xE6:
        return Instruction{opcode, "AND d8", 2, 8};
    case 0xE7:
        return Instruction{opcode, "RST 20H", 1, 16};
    case 0xE8:
        return Instruction{opcode, "ADD SP, r8", 2, 16};
    case 0xE9:
        return Instruction{opcode, "JP (HL)", 1, 4};
    case 0xEA:
        return Instruction{opcode, "LD (a16), A", 3, 16};
    case 0xEE:
        return Instruction{opcode, "XOR d8", 2, 8};
    case 0xEF:
        return Instruction{opcode, "RST 28H", 1, 16};

    case 0xF0:
        return Instruction{opcode, "LDH A, (a8)", 2, 12};
    case 0xF1:
        return Instruction{opcode, "POP AF", 1, 12};
    case 0xF2:
        return Instruction{opcode, "LD A, (C)", 1, 8};
    case 0xF3:
        return Instruction{opcode, "DI"};
    case 0xF5:
        return Instruction{opcode, "PUSH AF", 1, 16};
    case 0xF6:
        return Instruction{opcode, "OR d8", 2, 8};
    case 0xF7:
        return Instruction{opcode, "RST 30H", 1, 16};
    case 0xF8:
        return Instruction{opcode, "LD HL, SP+r8", 2, 12};
    case 0xF9:
        return Instruction{opcode, "LD SP, HL", 1, 8};
    case 0xFA:
        return Instruction{opcode, "LD A, (a16)", 3, 16};
    case 0xFB:
        return Instruction{opcode, "EI"};
    case 0xFE:
        return Instruction{opcode, "CP d8", 2, 8};
    case 0xFF:
        return Instruction{opcode, "RST 38H", 1, 16};
    default:
        return Instruction{opcode, "UNKNOWN"};
    }
}

Instruction decodeCBInstruction(uint8 opcode)
{
    switch (opcode)
    {
    case 0x00:
        return Instruction{opcode, "RLC B", 2, 8};
    case 0x01:
        return Instruction{opcode, "RLC C", 2, 8};
    case 0x02:
        return Instruction{opcode, "RLC D", 2, 8};
    case 0x03:
        return Instruction{opcode, "RLC E", 2, 8};
    case 0x04:
        return Instruction{opcode, "RLC H", 2, 8};
    case 0x05:
        return Instruction{opcode, "RLC L", 2, 8};
    case 0x06:
        return Instruction{opcode, "RLC (HL)", 2, 16};
    case 0x07:
        return Instruction{opcode, "RLC A", 2, 8};

    case 0x08:
        return Instruction{opcode, "RRC B", 2, 8};
    case 0x09:
        return Instruction{opcode, "RRC C", 2, 8};
    case 0x0A:
        return Instruction{opcode, "RRC D", 2, 8};
    case 0x0B:
        return Instruction{opcode, "RRC E", 2, 8};
    case 0x0C:
        return Instruction{opcode, "RRC H", 2, 8};
    case 0x0D:
        return Instruction{opcode, "RRC L", 2, 8};
    case 0x0E:
        return Instruction{opcode, "RRC (HL)", 2, 16};
    case 0x0F:
        return Instruction{opcode, "RRC A", 2, 8};

    case 0x10:
        return Instruction{opcode, "RL B", 2, 8};
    case 0x11:
        return Instruction{opcode, "RL C", 2, 8};
    case 0x12:
        return Instruction{opcode, "RL D", 2, 8};
    case 0x13:
        return Instruction{opcode, "RL E", 2, 8};
    case 0x14:
        return Instruction{opcode, "RL H", 2, 8};
    case 0x15:
        return Instruction{opcode, "RL L", 2, 8};
    case 0x16:
        return Instruction{opcode, "RL (HL)", 2, 16};
    case 0x17:
        return Instruction{opcode, "RL A", 2, 8};

    case 0x18:
        return Instruction{opcode, "RR B", 2, 8};
    case 0x19:
        return Instruction{opcode, "RR C", 2, 8};
    case 0x1A:
        return Instruction{opcode, "RR D", 2, 8};
    case 0x1B:
        return Instruction{opcode, "RR E", 2, 8};
    case 0x1C:
        return Instruction{opcode, "RR H", 2, 8};
    case 0x1D:
        return Instruction{opcode, "RR L", 2, 8};
    case 0x1E:
        return Instruction{opcode, "RR (HL)", 2, 16};
    case 0x1F:
        return Instruction{opcode, "RR A", 2, 8};

    case 0x20:
        return Instruction{opcode, "SLA B", 2, 8};
    case 0x21:
        return Instruction{opcode, "SLA C", 2, 8};
    case 0x22:
        return Instruction{opcode, "SLA D", 2, 8};
    case 0x23:
        return Instruction{opcode, "SLA E", 2, 8};
    case 0x24:
        return Instruction{opcode, "SLA H", 2, 8};
    case 0x25:
        return Instruction{opcode, "SLA L", 2, 8};
    case 0x26:
        return Instruction{opcode, "SLA (HL)", 2, 16};
    case 0x27:
        return Instruction{opcode, "SLA A", 2, 8};

    case 0x28:
        return Instruction{opcode, "SRA B", 2, 8};
    case 0x29:
        return Instruction{opcode, "SRA C", 2, 8};
    case 0x2A:
        return Instruction{opcode, "SRA D", 2, 8};
    case 0x2B:
        return Instruction{opcode, "SRA E", 2, 8};
    case 0x2C:
        return Instruction{opcode, "SRA H", 2, 8};
    case 0x2D:
        return Instruction{opcode, "SRA L", 2, 8};
    case 0x2E:
        return Instruction{opcode, "SRA (HL)", 2, 16};
    case 0x2F:
        return Instruction{opcode, "SRA A", 2, 8};

    case 0x30:
        return Instruction{opcode, "SWAP B", 2, 8};
    case 0x31:
        return Instruction{opcode, "SWAP C", 2, 8};
    case 0x32:
        return Instruction{opcode, "SWAP D", 2, 8};
    case 0x33:
        return Instruction{opcode, "SWAP E", 2, 8};
    case 0x34:
        return Instruction{opcode, "SWAP H", 2, 8};
    case 0x35:
        return Instruction{opcode, "SWAP L", 2, 8};
    case 0x36:
        return Instruction{opcode, "SWAP (HL)", 2, 16};
    case 0x37:
        return Instruction{opcode, "SWAP A", 2, 8};

    case 0x38:
        return Instruction{opcode, "SRL B", 2, 8};
    case 0x39:
        return Instruction{opcode, "SRL C", 2, 8};
    case 0x3A:
        return Instruction{opcode, "SRL D", 2, 8};
    case 0x3B:
        return Instruction{opcode, "SRL E", 2, 8};
    case 0x3C:
        return Instruction{opcode, "SRL H", 2, 8};
    case 0x3D:
        return Instruction{opcode, "SRL L", 2, 8};
    case 0x3E:
        return Instruction{opcode, "SRL (HL)", 2, 16};
    case 0x3F:
        return Instruction{opcode, "SRL A", 2, 8};

    case 0x40:
        return Instruction{opcode, "BIT 0, B", 2, 8};
    case 0x41:
        return Instruction{opcode, "BIT 0, C", 2, 8};
    case 0x42:
        return Instruction{opcode, "BIT 0, D", 2, 8};
    case 0x43:
        return Instruction{opcode, "BIT 0, E", 2, 8};
    case 0x44:
        return Instruction{opcode, "BIT 0, H", 2, 8};
    case 0x45:
        return Instruction{opcode, "BIT 0, L", 2, 8};
    case 0x46:
        return Instruction{opcode, "BIT 0, (HL)", 2, 16};
    case 0x47:
        return Instruction{opcode, "BIT 0, A", 2, 8};

    case 0x48:
        return Instruction{opcode, "BIT 1, B", 2, 8};
    case 0x49:
        return Instruction{opcode, "BIT 1, C", 2, 8};
    case 0x4A:
        return Instruction{opcode, "BIT 1, D", 2, 8};
    case 0x4B:
        return Instruction{opcode, "BIT 1, E", 2, 8};
    case 0x4C:
        return Instruction{opcode, "BIT 1, H", 2, 8};
    case 0x4D:
        return Instruction{opcode, "BIT 1, L", 2, 8};
    case 0x4E:
        return Instruction{opcode, "BIT 1, (HL)", 2, 16};
    case 0x4F:
        return Instruction{opcode, "BIT 1, A", 2, 8};

    case 0x50:
        return Instruction{opcode, "BIT 2, B", 2, 8};
    case 0x51:
        return Instruction{opcode, "BIT 2, C", 2, 8};
    case 0x52:
        return Instruction{opcode, "BIT 2, D", 2, 8};
    case 0x53:
        return Instruction{opcode, "BIT 2, E", 2, 8};
    case 0x54:
        return Instruction{opcode, "BIT 2, H", 2, 8};
    case 0x55:
        return Instruction{opcode, "BIT 2, L", 2, 8};
    case 0x56:
        return Instruction{opcode, "BIT 2, (HL)", 2, 16};
    case 0x57:
        return Instruction{opcode, "BIT 2, A", 2, 8};

    case 0x58:
        return Instruction{opcode, "BIT 3, B", 2, 8};
    case 0x59:
        return Instruction{opcode, "BIT 3, C", 2, 8};
    case 0x5A:
        return Instruction{opcode, "BIT 3, D", 2, 8};
    case 0x5B:
        return Instruction{opcode, "BIT 3, E", 2, 8};
    case 0x5C:
        return Instruction{opcode, "BIT 3, H", 2, 8};
    case 0x5D:
        return Instruction{opcode, "BIT 3, L", 2, 8};
    case 0x5E:
        return Instruction{opcode, "BIT 3, (HL)", 2, 16};
    case 0x5F:
        return Instruction{opcode, "BIT 3, A", 2, 8};

    case 0x60:
        return Instruction{opcode, "BIT 4, B", 2, 8};
    case 0x61:
        return Instruction{opcode, "BIT 4, C", 2, 8};
    case 0x62:
        return Instruction{opcode, "BIT 4, D", 2, 8};
    case 0x63:
        return Instruction{opcode, "BIT 4, E", 2, 8};
    case 0x64:
        return Instruction{opcode, "BIT 4, H", 2, 8};
    case 0x65:
        return Instruction{opcode, "BIT 4, L", 2, 8};
    case 0x66:
        return Instruction{opcode, "BIT 4, (HL)", 2, 16};
    case 0x67:
        return Instruction{opcode, "BIT 4, A", 2, 8};

    case 0x68:
        return Instruction{opcode, "BIT 5, B", 2, 8};
    case 0x69:
        return Instruction{opcode, "BIT 5, C", 2, 8};
    case 0x6A:
        return Instruction{opcode, "BIT 5, D", 2, 8};
    case 0x6B:
        return Instruction{opcode, "BIT 5, E", 2, 8};
    case 0x6C:
        return Instruction{opcode, "BIT 5, H", 2, 8};
    case 0x6D:
        return Instruction{opcode, "BIT 5, L", 2, 8};
    case 0x6E:
        return Instruction{opcode, "BIT 5, (HL)", 2, 16};
    case 0x6F:
        return Instruction{opcode, "BIT 5, A", 2, 8};

    case 0x70:
        return Instruction{opcode, "BIT 6, B", 2, 8};
    case 0x71:
        return Instruction{opcode, "BIT 6, C", 2, 8};
    case 0x72:
        return Instruction{opcode, "BIT 6, D", 2, 8};
    case 0x73:
        return Instruction{opcode, "BIT 6, E", 2, 8};
    case 0x74:
        return Instruction{opcode, "BIT 6, H", 2, 8};
    case 0x75:
        return Instruction{opcode, "BIT 6, L", 2, 8};
    case 0x76:
        return Instruction{opcode, "BIT 6, (HL)", 2, 16};
    case 0x77:
        return Instruction{opcode, "BIT 6, A", 2, 8};

    case 0x78:
        return Instruction{opcode, "BIT 7, B", 2, 8};
    case 0x79:
        return Instruction{opcode, "BIT 7, C", 2, 8};
    case 0x7A:
        return Instruction{opcode, "BIT 7, D", 2, 8};
    case 0x7B:
        return Instruction{opcode, "BIT 7, E", 2, 8};
    case 0x7C:
        return Instruction{opcode, "BIT 7, H", 2, 8};
    case 0x7D:
        return Instruction{opcode, "BIT 7, L", 2, 8};
    case 0x7E:
        return Instruction{opcode, "BIT 7, (HL)", 2, 16};
    case 0x7F:
        return Instruction{opcode, "BIT 7, A", 2, 8};

    case 0x80:
        return Instruction{opcode, "RES 0, B", 2, 8};
    case 0x81:
        return Instruction{opcode, "RES 0, C", 2, 8};
    case 0x82:
        return Instruction{opcode, "RES 0, D", 2, 8};
    case 0x83:
        return Instruction{opcode, "RES 0, E", 2, 8};
    case 0x84:
        return Instruction{opcode, "RES 0, H", 2, 8};
    case 0x85:
        return Instruction{opcode, "RES 0, L", 2, 8};
    case 0x86:
        return Instruction{opcode, "RES 0, (HL)", 2, 16};
    case 0x87:
        return Instruction{opcode, "RES 0, A", 2, 8};

    case 0x88:
        return Instruction{opcode, "RES 1, B", 2, 8};
    case 0x89:
        return Instruction{opcode, "RES 1, C", 2, 8};
    case 0x8A:
        return Instruction{opcode, "RES 1, D", 2, 8};
    case 0x8B:
        return Instruction{opcode, "RES 1, E", 2, 8};
    case 0x8C:
        return Instruction{opcode, "RES 1, H", 2, 8};
    case 0x8D:
        return Instruction{opcode, "RES 1, L", 2, 8};
    case 0x8E:
        return Instruction{opcode, "RES 1, (HL)", 2, 16};
    case 0x8F:
        return Instruction{opcode, "RES 1, A", 2, 8};

    case 0x90:
        return Instruction{opcode, "RES 2, B", 2, 8};
    case 0x91:
        return Instruction{opcode, "RES 2, C", 2, 8};
    case 0x92:
        return Instruction{opcode, "RES 2, D", 2, 8};
    case 0x93:
        return Instruction{opcode, "RES 2, E", 2, 8};
    case 0x94:
        return Instruction{opcode, "RES 2, H", 2, 8};
    case 0x95:
        return Instruction{opcode, "RES 2, L", 2, 8};
    case 0x96:
        return Instruction{opcode, "RES 2, (HL)", 2, 16};
    case 0x97:
        return Instruction{opcode, "RES 2, A", 2, 8};

    case 0x98:
        return Instruction{opcode, "RES 3, B", 2, 8};
    case 0x99:
        return Instruction{opcode, "RES 3, C", 2, 8};
    case 0x9A:
        return Instruction{opcode, "RES 3, D", 2, 8};
    case 0x9B:
        return Instruction{opcode, "RES 3, E", 2, 8};
    case 0x9C:
        return Instruction{opcode, "RES 3, H", 2, 8};
    case 0x9D:
        return Instruction{opcode, "RES 3, L", 2, 8};
    case 0x9E:
        return Instruction{opcode, "RES 3, (HL)", 2, 16};
    case 0x9F:
        return Instruction{opcode, "RES 3, A", 2, 8};

    case 0xA0:
        return Instruction{opcode, "RES 4, B", 2, 8};
    case 0xA1:
        return Instruction{opcode, "RES 4, C", 2, 8};
    case 0xA2:
        return Instruction{opcode, "RES 4, D", 2, 8};
    case 0xA3:
        return Instruction{opcode, "RES 4, E", 2, 8};
    case 0xA4:
        return Instruction{opcode, "RES 4, H", 2, 8};
    case 0xA5:
        return Instruction{opcode, "RES 4, L", 2, 8};
    case 0xA6:
        return Instruction{opcode, "RES 4, (HL)", 2, 16};
    case 0xA7:
        return Instruction{opcode, "RES 4, A", 2, 8};

    case 0xA8:
        return Instruction{opcode, "RES 5, B", 2, 8};
    case 0xA9:
        return Instruction{opcode, "RES 5, C", 2, 8};
    case 0xAA:
        return Instruction{opcode, "RES 5, D", 2, 8};
    case 0xAB:
        return Instruction{opcode, "RES 5, E", 2, 8};
    case 0xAC:
        return Instruction{opcode, "RES 5, H", 2, 8};
    case 0xAD:
        return Instruction{opcode, "RES 5, L", 2, 8};
    case 0xAE:
        return Instruction{opcode, "RES 5, (HL)", 2, 16};
    case 0xAF:
        return Instruction{opcode, "RES 5, A", 2, 8};

    case 0xB0:
        return Instruction{opcode, "RES 6, B", 2, 8};
    case 0xB1:
        return Instruction{opcode, "RES 6, C", 2, 8};
    case 0xB2:
        return Instruction{opcode, "RES 6, D", 2, 8};
    case 0xB3:
        return Instruction{opcode, "RES 6, E", 2, 8};
    case 0xB4:
        return Instruction{opcode, "RES 6, H", 2, 8};
    case 0xB5:
        return Instruction{opcode, "RES 6, L", 2, 8};
    case 0xB6:
        return Instruction{opcode, "RES 6, (HL)", 2, 16};
    case 0xB7:
        return Instruction{opcode, "RES 6, A", 2, 8};

    case 0xB8:
        return Instruction{opcode, "RES 7, B", 2, 8};
    case 0xB9:
        return Instruction{opcode, "RES 7, C", 2, 8};
    case 0xBA:
        return Instruction{opcode, "RES 7, D", 2, 8};
    case 0xBB:
        return Instruction{opcode, "RES 7, E", 2, 8};
    case 0xBC:
        return Instruction{opcode, "RES 7, H", 2, 8};
    case 0xBD:
        return Instruction{opcode, "RES 7, L", 2, 8};
    case 0xBE:
        return Instruction{opcode, "RES 7, (HL)", 2, 16};
    case 0xBF:
        return Instruction{opcode, "RES 7, A", 2, 8};

    case 0xC0:
        return Instruction{opcode, "SET 0, B", 2, 8};
    case 0xC1:
        return Instruction{opcode, "SET 0, C", 2, 8};
    case 0xC2:
        return Instruction{opcode, "SET 0, D", 2, 8};
    case 0xC3:
        return Instruction{opcode, "SET 0, E", 2, 8};
    case 0xC4:
        return Instruction{opcode, "SET 0, H", 2, 8};
    case 0xC5:
        return Instruction{opcode, "SET 0, L", 2, 8};
    case 0xC6:
        return Instruction{opcode, "SET 0, (HL)", 2, 16};
    case 0xC7:
        return Instruction{opcode, "SET 0, A", 2, 8};

    case 0xC8:
        return Instruction{opcode, "SET 1, B", 2, 8};
    case 0xC9:
        return Instruction{opcode, "SET 1, C", 2, 8};
    case 0xCA:
        return Instruction{opcode, "SET 1, D", 2, 8};
    case 0xCB:
        return Instruction{opcode, "SET 1, E", 2, 8};
    case 0xCC:
        return Instruction{opcode, "SET 1, H", 2, 8};
    case 0xCD:
        return Instruction{opcode, "SET 1, L", 2, 8};
    case 0xCE:
        return Instruction{opcode, "SET 1, (HL)", 2, 16};
    case 0xCF:
        return Instruction{opcode, "SET 1, A", 2, 8};

    case 0xD0:
        return Instruction{opcode, "SET 2, B", 2, 8};
    case 0xD1:
        return Instruction{opcode, "SET 2, C", 2, 8};
    case 0xD2:
        return Instruction{opcode, "SET 2, D", 2, 8};
    case 0xD3:
        return Instruction{opcode, "SET 2, E", 2, 8};
    case 0xD4:
        return Instruction{opcode, "SET 2, H", 2, 8};
    case 0xD5:
        return Instruction{opcode, "SET 2, L", 2, 8};
    case 0xD6:
        return Instruction{opcode, "SET 2, (HL)", 2, 16};
    case 0xD7:
        return Instruction{opcode, "SET 2, A", 2, 8};

    case 0xD8:
        return Instruction{opcode, "SET 3, B", 2, 8};
    case 0xD9:
        return Instruction{opcode, "SET 3, C", 2, 8};
    case 0xDA:
        return Instruction{opcode, "SET 3, D", 2, 8};
    case 0xDB:
        return Instruction{opcode, "SET 3, E", 2, 8};
    case 0xDC:
        return Instruction{opcode, "SET 3, H", 2, 8};
    case 0xDD:
        return Instruction{opcode, "SET 3, L", 2, 8};
    case 0xDE:
        return Instruction{opcode, "SET 3, (HL)", 2, 16};
    case 0xDF:
        return Instruction{opcode, "SET 3, A", 2, 8};

    case 0xE0:
        return Instruction{opcode, "SET 4, B", 2, 8};
    case 0xE1:
        return Instruction{opcode, "SET 4, C", 2, 8};
    case 0xE2:
        return Instruction{opcode, "SET 4, D", 2, 8};
    case 0xE3:
        return Instruction{opcode, "SET 4, E", 2, 8};
    case 0xE4:
        return Instruction{opcode, "SET 4, H", 2, 8};
    case 0xE5:
        return Instruction{opcode, "SET 4, L", 2, 8};
    case 0xE6:
        return Instruction{opcode, "SET 4, (HL)", 2, 16};
    case 0xE7:
        return Instruction{opcode, "SET 4, A", 2, 8};

    case 0xE8:
        return Instruction{opcode, "SET 5, B", 2, 8};
    case 0xE9:
        return Instruction{opcode, "SET 5, C", 2, 8};
    case 0xEA:
        return Instruction{opcode, "SET 5, D", 2, 8};
    case 0xEB:
        return Instruction{opcode, "SET 5, E", 2, 8};
    case 0xEC:
        return Instruction{opcode, "SET 5, H", 2, 8};
    case 0xED:
        return Instruction{opcode, "SET 5, L", 2, 8};
    case 0xEE:
        return Instruction{opcode, "SET 5, (HL)", 2, 16};
    case 0xEF:
        return Instruction{opcode, "SET 5, A", 2, 8};

    case 0xF0:
        return Instruction{opcode, "SET 6, B", 2, 8};
    case 0xF1:
        return Instruction{opcode, "SET 6, C", 2, 8};
    case 0xF2:
        return Instruction{opcode, "SET 6, D", 2, 8};
    case 0xF3:
        return Instruction{opcode, "SET 6, E", 2, 8};
    case 0xF4:
        return Instruction{opcode, "SET 6, H", 2, 8};
    case 0xF5:
        return Instruction{opcode, "SET 6, L", 2, 8};
    case 0xF6:
        return Instruction{opcode, "SET 6, (HL)", 2, 16};
    case 0xF7:
        return Instruction{opcode, "SET 6, A", 2, 8};

    case 0xF8:
        return Instruction{opcode, "SET 7, B", 2, 8};
    case 0xF9:
        return Instruction{opcode, "SET 7, C", 2, 8};
    case 0xFA:
        return Instruction{opcode, "SET 7, D", 2, 8};
    case 0xFB:
        return Instruction{opcode, "SET 7, E", 2, 8};
    case 0xFC:
        return Instruction{opcode, "SET 7, H", 2, 8};
    case 0xFD:
        return Instruction{opcode, "SET 7, L", 2, 8};
    case 0xFE:
        return Instruction{opcode, "SET 7, (HL)", 2, 16};
    case 0xFF:
        return Instruction{opcode, "SET 7, A", 2, 8};
    default:
        return Instruction{opcode, "UNKNOWN"};
    }
}

#pragma endregion