#include <cstdint>
#include "memory.h"
#include <string>

#define uint8 uint8_t
#define uint16 uint16_t

struct CPU{
    uint8 A, F;
    uint8 B, C;
    uint8 D, E;
    uint8 H, L;


    uint16 SP;
    uint16 PC;

    void reset(){
        A = F = B = C = D = E = H = L = 0;
        SP = 0xFFFE;    // grows downwards
    }

    // allows combining 2 registers into 16 bit
    uint16 AF() { return (A << 8) | F; }
    uint16 BC() { return (B << 8) | C; }
    uint16 DE() { return (D << 8) | E; }
    uint16 HL() { return (H << 8) | L; }
    void setAF(uint16 val){
        A = val >> 8;
        F = val & 0xFF;
    }

    void setBC(uint16 val){
        B = val >> 8;
        C = val & 0xFF;
    }

    void setDE(uint16 val){
        D = val >> 8;
        E = val & 0xFF;
    }

    void setHL(uint16 val){
        H = val >> 8;
        L = val & 0xFF;
    }

    // Flag reset functions
    void resetZ() { F &= ~0x80; }   // Clear Z flag (bit 7)
    void resetN() { F &= ~0x40; }   // Clear N flag (bit 6)
    void resetH() { F &= ~0x20; }   // Clear H flag (bit 5)
    void resetC() { F &= ~0x10; }   // Clear C flag (bit 4)

    // Flag set functions
    void setZ() { F |= 0x80; }      // Set Z flag (bit 7)
    void setN() { F |= 0x40; }      // Set N flag (bit 6)
    void setH() { F |= 0x20; }      // Set H flag (bit 5)
    void setC() { F |= 0x10; }      // Set C flag (bit 4)
};

struct Instruction{
    uint16 opcode;
    std::string mnemonic;
    uint8 length = 1;
    uint8 cycles = 4;
};

Instruction decodeInstruction(uint16 opcode);
Instruction decodeCBInstruction(uint8 opcode);
Instruction decodeNormalInstruction(uint8 opcode);

CPU cpu {};

Instruction decodeInstruction(uint16 opcode){
    if ((opcode & 0xFF00) == 0xCB00){
        uint8 cb_opcode = opcode & 0xFF;
        cpu.PC += 1;
        return decodeCBInstruction(cb_opcode);
    }
    else{
        uint8 normal_opcode = opcode & 0xFF;
        return decodeNormalInstruction(normal_opcode);
    }
}


Instruction decodeNormalInstruction(uint8 opcode){
    switch(opcode){
        case 0x00: return Instruction{opcode,"NOP"};
        case 0x01: return Instruction{opcode,"LD BC, d16",3,12};
        case 0x02: return Instruction{opcode,"LD (BC), A",1,8};
        case 0x03: return Instruction{opcode,"INC BC",1,8};
        case 0x04: return Instruction{opcode,"INC B"};
        case 0x05: return Instruction{opcode,"DEC B"};
        case 0x06: return Instruction{opcode,"LD B, d8",2,8};
        case 0x07: return Instruction{opcode,"RLCA"};
        case 0x08: return Instruction{opcode,"LD (a16), SP",3,20};
        case 0x09: return Instruction{opcode,"ADD HL, BC",1,8};
        case 0x0A: return Instruction{opcode,"LD A, (BC)",1,8};
        case 0x0B: return Instruction{opcode,"DEC BC",1,8};
        case 0x0C: return Instruction{opcode,"INC C"};
        case 0x0D: return Instruction{opcode,"DEC C"};
        case 0x0E: return Instruction{opcode,"LD C, d8",2,8};
        case 0x0F: return Instruction{opcode,"RRCA"};

        case 0x10: return Instruction{opcode,"STOP",2,4};
        case 0x11: return Instruction{opcode,"LD DE, d16",3,12};
        case 0x12: return Instruction{opcode,"LD (DE), A",1,8};
        case 0x13: return Instruction{opcode,"INC DE",1,8};
        case 0x14: return Instruction{opcode,"INC D"};
        case 0x15: return Instruction{opcode,"DEC D"};
        case 0x16: return Instruction{opcode,"LD D, d8",2,8};
        case 0x17: return Instruction{opcode,"RLA"};
        case 0x18: return Instruction{opcode,"JR r8",2,12};
        case 0x19: return Instruction{opcode,"ADD HL, DE",1,8};
        case 0x1A: return Instruction{opcode,"LD A, (DE)",1,8};
        case 0x1B: return Instruction{opcode,"DEC DE",1,8};
        case 0x1C: return Instruction{opcode,"INC E"};
        case 0x1D: return Instruction{opcode,"DEC E"};
        case 0x1E: return Instruction{opcode,"LD E, d8",2,8};
        case 0x1F: return Instruction{opcode,"RRA"};

        case 0x20: return Instruction{opcode,"JR NZ, r8",2,12};
        case 0x21: return Instruction{opcode,"LD HL, d16",3,12};
        case 0x22: return Instruction{opcode,"LD (HL+), A",1,8};
        case 0x23: return Instruction{opcode,"INC HL",1,8};
        case 0x24: return Instruction{opcode,"INC H"};
        case 0x25: return Instruction{opcode,"DEC H"};
        case 0x26: return Instruction{opcode,"LD H, d8",2,8};
        case 0x27: return Instruction{opcode,"DAA"};
        case 0x28: return Instruction{opcode,"JR Z, r8",2,12};
        case 0x29: return Instruction{opcode,"ADD HL, HL",1,8};
        case 0x2A: return Instruction{opcode,"LD A, (HL+)",1,8};
        case 0x2B: return Instruction{opcode,"DEC HL",1,8};
        case 0x2C: return Instruction{opcode,"INC L"};
        case 0x2D: return Instruction{opcode,"DEC L"};
        case 0x2E: return Instruction{opcode,"LD L, d8",2,8};
        case 0x2F: return Instruction{opcode,"CPL"};

        case 0x30: return Instruction{opcode,"JR NC, r8",2,12};
        case 0x31: return Instruction{opcode,"LD SP, d16",3,12};
        case 0x32: return Instruction{opcode,"LD (HL-), A",1,8};
        case 0x33: return Instruction{opcode,"INC SP",1,8};
        case 0x34: return Instruction{opcode,"INC (HL)",1,12};
        case 0x35: return Instruction{opcode,"DEC (HL)",1,12};
        case 0x36: return Instruction{opcode,"LD (HL), d8",2,12};
        case 0x37: return Instruction{opcode,"SCF"};
        case 0x38: return Instruction{opcode,"JR C, r8",2,12};
        case 0x39: return Instruction{opcode,"ADD HL, SP",1,8};
        case 0x3A: return Instruction{opcode,"LD A, (HL-)",1,8};
        case 0x3B: return Instruction{opcode,"DEC SP",1,8};
        case 0x3C: return Instruction{opcode,"INC A"};
        case 0x3D: return Instruction{opcode,"DEC A"};
        case 0x3E: return Instruction{opcode,"LD A, d8",2,8};
        case 0x3F: return Instruction{opcode,"CCF"};

        case 0x40: return Instruction{opcode,"LD B, B"};
        case 0x41: return Instruction{opcode,"LD B, C"};
        case 0x42: return Instruction{opcode,"LD B, D"};
        case 0x43: return Instruction{opcode,"LD B, E"};
        case 0x44: return Instruction{opcode,"LD B, H"};
        case 0x45: return Instruction{opcode,"LD B, L"};
        case 0x46: return Instruction{opcode,"LD B, (HL)",1,8};
        case 0x47: return Instruction{opcode,"LD B, A"};

        case 0x48: return Instruction{opcode,"LD C, B"};
        case 0x49: return Instruction{opcode,"LD C, C"};
        case 0x4A: return Instruction{opcode,"LD C, D"};
        case 0x4B: return Instruction{opcode,"LD C, E"};
        case 0x4C: return Instruction{opcode,"LD C, H"};
        case 0x4D: return Instruction{opcode,"LD C, L"};
        case 0x4E: return Instruction{opcode,"LD C, (HL)",1,8};
        case 0x4F: return Instruction{opcode,"LD C, A"};

        case 0x50: return Instruction{opcode,"LD D, B"};
        case 0x51: return Instruction{opcode,"LD D, C"};
        case 0x52: return Instruction{opcode,"LD D, D"};
        case 0x53: return Instruction{opcode,"LD D, E"};
        case 0x54: return Instruction{opcode,"LD D, H"};
        case 0x55: return Instruction{opcode,"LD D, L"};
        case 0x56: return Instruction{opcode,"LD D, (HL)",1,8};
        case 0x57: return Instruction{opcode,"LD D, A"};

        case 0x58: return Instruction{opcode,"LD E, B"};
        case 0x59: return Instruction{opcode,"LD E, C"};
        case 0x5A: return Instruction{opcode,"LD E, D"};
        case 0x5B: return Instruction{opcode,"LD E, E"};
        case 0x5C: return Instruction{opcode,"LD E, H"};
        case 0x5D: return Instruction{opcode,"LD E, L"};
        case 0x5E: return Instruction{opcode,"LD E, (HL)",1,8};
        case 0x5F: return Instruction{opcode,"LD E, A"};

        case 0x60: return Instruction{opcode,"LD H, B"};
        case 0x61: return Instruction{opcode,"LD H, C"};
        case 0x62: return Instruction{opcode,"LD H, D"};
        case 0x63: return Instruction{opcode,"LD H, E"};
        case 0x64: return Instruction{opcode,"LD H, H"};
        case 0x65: return Instruction{opcode,"LD H, L"};
        case 0x66: return Instruction{opcode,"LD H, (HL)",1,8};
        case 0x67: return Instruction{opcode,"LD H, A"};

        case 0x68: return Instruction{opcode,"LD L, B"};
        case 0x69: return Instruction{opcode,"LD L, C"};
        case 0x6A: return Instruction{opcode,"LD L, D"};
        case 0x6B: return Instruction{opcode,"LD L, E"};
        case 0x6C: return Instruction{opcode,"LD L, H"};
        case 0x6D: return Instruction{opcode,"LD L, L"};
        case 0x6E: return Instruction{opcode,"LD L, (HL)",1,8};
        case 0x6F: return Instruction{opcode,"LD L, A"};

        case 0x70: return Instruction{opcode,"LD (HL), B",1,8};
        case 0x71: return Instruction{opcode,"LD (HL), C",1,8};
        case 0x72: return Instruction{opcode,"LD (HL), D",1,8};
        case 0x73: return Instruction{opcode,"LD (HL), E",1,8};
        case 0x74: return Instruction{opcode,"LD (HL), H",1,8};
        case 0x75: return Instruction{opcode,"LD (HL), L",1,8};
        case 0x76: return Instruction{opcode,"HALT"};
        case 0x77: return Instruction{opcode,"LD (HL), A",1,8};

        case 0x78: return Instruction{opcode,"LD A, B"};
        case 0x79: return Instruction{opcode,"LD A, C"};
        case 0x7A: return Instruction{opcode,"LD A, D"};
        case 0x7B: return Instruction{opcode,"LD A, E"};
        case 0x7C: return Instruction{opcode,"LD A, H"};
        case 0x7D: return Instruction{opcode,"LD A, L"};
        case 0x7E: return Instruction{opcode,"LD A, (HL)",1,8};
        case 0x7F: return Instruction{opcode,"LD A, A"};
        case 0x80: return Instruction{opcode,"ADD A, B"};
        case 0x81: return Instruction{opcode,"ADD A, C"};
        case 0x82: return Instruction{opcode,"ADD A, D"};
        case 0x83: return Instruction{opcode,"ADD A, E"};
        case 0x84: return Instruction{opcode,"ADD A, H"};
        case 0x85: return Instruction{opcode,"ADD A, L"};
        case 0x86: return Instruction{opcode,"ADD A, (HL)",1,8};
        case 0x87: return Instruction{opcode,"ADD A, A"};

        case 0x88: return Instruction{opcode,"ADC A, B"};
        case 0x89: return Instruction{opcode,"ADC A, C"};
        case 0x8A: return Instruction{opcode,"ADC A, D"};
        case 0x8B: return Instruction{opcode,"ADC A, E"};
        case 0x8C: return Instruction{opcode,"ADC A, H"};
        case 0x8D: return Instruction{opcode,"ADC A, L"};
        case 0x8E: return Instruction{opcode,"ADC A, (HL)",1,8};
        case 0x8F: return Instruction{opcode,"ADC A, A"};

        case 0x90: return Instruction{opcode,"SUB B"};
        case 0x91: return Instruction{opcode,"SUB C"};
        case 0x92: return Instruction{opcode,"SUB D"};
        case 0x93: return Instruction{opcode,"SUB E"};
        case 0x94: return Instruction{opcode,"SUB H"};
        case 0x95: return Instruction{opcode,"SUB L"};
        case 0x96: return Instruction{opcode,"SUB (HL)",1,8};
        case 0x97: return Instruction{opcode,"SUB A"};

        case 0x98: return Instruction{opcode,"SBC A, B"};
        case 0x99: return Instruction{opcode,"SBC A, C"};
        case 0x9A: return Instruction{opcode,"SBC A, D"};
        case 0x9B: return Instruction{opcode,"SBC A, E"};
        case 0x9C: return Instruction{opcode,"SBC A, H"};
        case 0x9D: return Instruction{opcode,"SBC A, L"};
        case 0x9E: return Instruction{opcode,"SBC A, (HL)",1,8};
        case 0x9F: return Instruction{opcode,"SBC A, A"};

        case 0xA0: return Instruction{opcode,"AND B"};
        case 0xA1: return Instruction{opcode,"AND C"};
        case 0xA2: return Instruction{opcode,"AND D"};
        case 0xA3: return Instruction{opcode,"AND E"};
        case 0xA4: return Instruction{opcode,"AND H"};
        case 0xA5: return Instruction{opcode,"AND L"};
        case 0xA6: return Instruction{opcode,"AND (HL)",1,8};
        case 0xA7: return Instruction{opcode,"AND A"};

        case 0xA8: return Instruction{opcode,"XOR B"};
        case 0xA9: return Instruction{opcode,"XOR C"};
        case 0xAA: return Instruction{opcode,"XOR D"};
        case 0xAB: return Instruction{opcode,"XOR E"};
        case 0xAC: return Instruction{opcode,"XOR H"};
        case 0xAD: return Instruction{opcode,"XOR L"};
        case 0xAE: return Instruction{opcode,"XOR (HL)",1,8};
        case 0xAF: return Instruction{opcode,"XOR A"};

        case 0xB0: return Instruction{opcode,"OR B"};
        case 0xB1: return Instruction{opcode,"OR C"};
        case 0xB2: return Instruction{opcode,"OR D"};
        case 0xB3: return Instruction{opcode,"OR E"};
        case 0xB4: return Instruction{opcode,"OR H"};
        case 0xB5: return Instruction{opcode,"OR L"};
        case 0xB6: return Instruction{opcode,"OR (HL)",1,8};
        case 0xB7: return Instruction{opcode,"OR A"};

        case 0xB8: return Instruction{opcode,"CP B"};
        case 0xB9: return Instruction{opcode,"CP C"};
        case 0xBA: return Instruction{opcode,"CP D"};
        case 0xBB: return Instruction{opcode,"CP E"};
        case 0xBC: return Instruction{opcode,"CP H"};
        case 0xBD: return Instruction{opcode,"CP L"};
        case 0xBE: return Instruction{opcode,"CP (HL)",1,8};
        case 0xBF: return Instruction{opcode,"CP A"};

        case 0xC0: return Instruction{opcode,"RET NZ",1,8};
        case 0xC1: return Instruction{opcode,"POP BC",1,12};
        case 0xC2: return Instruction{opcode,"JP NZ, a16",3,12};
        case 0xC3: return Instruction{opcode,"JP a16",3,16};
        case 0xC4: return Instruction{opcode,"CALL NZ, a16",3,12};
        case 0xC5: return Instruction{opcode,"PUSH BC",1,16};
        case 0xC6: return Instruction{opcode,"ADD A, d8",2,8};
        case 0xC7: return Instruction{opcode,"RST 00H",1,16};

        case 0xC8: return Instruction{opcode,"RET Z",1,8};
        case 0xC9: return Instruction{opcode,"RET",1,16};
        case 0xCA: return Instruction{opcode,"JP Z, a16",3,12};
        case 0xCC: return Instruction{opcode,"CALL Z, a16",3,12};
        case 0xCD: return Instruction{opcode,"CALL a16",3,24};
        case 0xCE: return Instruction{opcode,"ADC A, d8",2,8};
        case 0xCF: return Instruction{opcode,"RST 08H",1,16};

        case 0xD0: return Instruction{opcode,"RET NC",1,8};
        case 0xD1: return Instruction{opcode,"POP DE",1,12};
        case 0xD2: return Instruction{opcode,"JP NC, a16",3,12};
        case 0xD4: return Instruction{opcode,"CALL NC, a16",3,12};
        case 0xD5: return Instruction{opcode,"PUSH DE",1,16};
        case 0xD6: return Instruction{opcode,"SUB d8",2,8};
        case 0xD7: return Instruction{opcode,"RST 10H",1,16};

        case 0xD8: return Instruction{opcode,"RET C",1,8};
        case 0xD9: return Instruction{opcode,"RETI",1,16};
        case 0xDA: return Instruction{opcode,"JP C, a16",3,12};
        case 0xDC: return Instruction{opcode,"CALL C, a16",3,12};
        case 0xDE: return Instruction{opcode,"SBC A, d8",2,8};
        case 0xDF: return Instruction{opcode,"RST 18H",1,16};

        case 0xE0: return Instruction{opcode,"LDH (a8), A",2,12};
        case 0xE1: return Instruction{opcode,"POP HL",1,12};
        case 0xE2: return Instruction{opcode,"LD (C), A",1,8};
        case 0xE5: return Instruction{opcode,"PUSH HL",1,16};
        case 0xE6: return Instruction{opcode,"AND d8",2,8};
        case 0xE7: return Instruction{opcode,"RST 20H",1,16};
        case 0xE8: return Instruction{opcode,"ADD SP, r8",2,16};
        case 0xE9: return Instruction{opcode,"JP (HL)",1,4};
        case 0xEA: return Instruction{opcode,"LD (a16), A",3,16};
        case 0xEE: return Instruction{opcode,"XOR d8",2,8};
        case 0xEF: return Instruction{opcode,"RST 28H",1,16};

        case 0xF0: return Instruction{opcode,"LDH A, (a8)",2,12};
        case 0xF1: return Instruction{opcode,"POP AF",1,12};
        case 0xF2: return Instruction{opcode,"LD A, (C)",1,8};
        case 0xF3: return Instruction{opcode,"DI"};
        case 0xF5: return Instruction{opcode,"PUSH AF",1,16};
        case 0xF6: return Instruction{opcode,"OR d8",2,8};
        case 0xF7: return Instruction{opcode,"RST 30H",1,16};
        case 0xF8: return Instruction{opcode,"LD HL, SP+r8",2,12};
        case 0xF9: return Instruction{opcode,"LD SP, HL",1,8};
        case 0xFA: return Instruction{opcode,"LD A, (a16)",3,16};
        case 0xFB: return Instruction{opcode,"EI"};
        case 0xFE: return Instruction{opcode,"CP d8",2,8};
        case 0xFF: return Instruction{opcode,"RST 38H",1,16};
        default: return Instruction {opcode, "UNKNOWN"};
    }
}

Instruction decodeCBInstruction(uint8 opcode){
    switch(opcode){
        case 0x00: return Instruction{opcode,"RLC B",2,8};
        case 0x01: return Instruction{opcode,"RLC C",2,8};
        case 0x02: return Instruction{opcode,"RLC D",2,8};
        case 0x03: return Instruction{opcode,"RLC E",2,8};
        case 0x04: return Instruction{opcode,"RLC H",2,8};
        case 0x05: return Instruction{opcode,"RLC L",2,8};
        case 0x06: return Instruction{opcode,"RLC (HL)",2,16};
        case 0x07: return Instruction{opcode,"RLC A",2,8};

        case 0x08: return Instruction{opcode,"RRC B",2,8};
        case 0x09: return Instruction{opcode,"RRC C",2,8};
        case 0x0A: return Instruction{opcode,"RRC D",2,8};
        case 0x0B: return Instruction{opcode,"RRC E",2,8};
        case 0x0C: return Instruction{opcode,"RRC H",2,8};
        case 0x0D: return Instruction{opcode,"RRC L",2,8};
        case 0x0E: return Instruction{opcode,"RRC (HL)",2,16};
        case 0x0F: return Instruction{opcode,"RRC A",2,8};

        case 0x10: return Instruction{opcode,"RL B",2,8};
        case 0x11: return Instruction{opcode,"RL C",2,8};
        case 0x12: return Instruction{opcode,"RL D",2,8};
        case 0x13: return Instruction{opcode,"RL E",2,8};
        case 0x14: return Instruction{opcode,"RL H",2,8};
        case 0x15: return Instruction{opcode,"RL L",2,8};
        case 0x16: return Instruction{opcode,"RL (HL)",2,16};
        case 0x17: return Instruction{opcode,"RL A",2,8};

        case 0x18: return Instruction{opcode,"RR B",2,8};
        case 0x19: return Instruction{opcode,"RR C",2,8};
        case 0x1A: return Instruction{opcode,"RR D",2,8};
        case 0x1B: return Instruction{opcode,"RR E",2,8};
        case 0x1C: return Instruction{opcode,"RR H",2,8};
        case 0x1D: return Instruction{opcode,"RR L",2,8};
        case 0x1E: return Instruction{opcode,"RR (HL)",2,16};
        case 0x1F: return Instruction{opcode,"RR A",2,8};

        case 0x20: return Instruction{opcode,"SLA B",2,8};
        case 0x21: return Instruction{opcode,"SLA C",2,8};
        case 0x22: return Instruction{opcode,"SLA D",2,8};
        case 0x23: return Instruction{opcode,"SLA E",2,8};
        case 0x24: return Instruction{opcode,"SLA H",2,8};
        case 0x25: return Instruction{opcode,"SLA L",2,8};
        case 0x26: return Instruction{opcode,"SLA (HL)",2,16};
        case 0x27: return Instruction{opcode,"SLA A",2,8};

        case 0x28: return Instruction{opcode,"SRA B",2,8};
        case 0x29: return Instruction{opcode,"SRA C",2,8};
        case 0x2A: return Instruction{opcode,"SRA D",2,8};
        case 0x2B: return Instruction{opcode,"SRA E",2,8};
        case 0x2C: return Instruction{opcode,"SRA H",2,8};
        case 0x2D: return Instruction{opcode,"SRA L",2,8};
        case 0x2E: return Instruction{opcode,"SRA (HL)",2,16};
        case 0x2F: return Instruction{opcode,"SRA A",2,8};

        case 0x30: return Instruction{opcode,"SWAP B",2,8};
        case 0x31: return Instruction{opcode,"SWAP C",2,8};
        case 0x32: return Instruction{opcode,"SWAP D",2,8};
        case 0x33: return Instruction{opcode,"SWAP E",2,8};
        case 0x34: return Instruction{opcode,"SWAP H",2,8};
        case 0x35: return Instruction{opcode,"SWAP L",2,8};
        case 0x36: return Instruction{opcode,"SWAP (HL)",2,16};
        case 0x37: return Instruction{opcode,"SWAP A",2,8};

        case 0x38: return Instruction{opcode,"SRL B",2,8};
        case 0x39: return Instruction{opcode,"SRL C",2,8};
        case 0x3A: return Instruction{opcode,"SRL D",2,8};
        case 0x3B: return Instruction{opcode,"SRL E",2,8};
        case 0x3C: return Instruction{opcode,"SRL H",2,8};
        case 0x3D: return Instruction{opcode,"SRL L",2,8};
        case 0x3E: return Instruction{opcode,"SRL (HL)",2,16};
        case 0x3F: return Instruction{opcode,"SRL A",2,8};

        case 0x40: return Instruction{opcode,"BIT 0, B",2,8};
        case 0x41: return Instruction{opcode,"BIT 0, C",2,8};
        case 0x42: return Instruction{opcode,"BIT 0, D",2,8};
        case 0x43: return Instruction{opcode,"BIT 0, E",2,8};
        case 0x44: return Instruction{opcode,"BIT 0, H",2,8};
        case 0x45: return Instruction{opcode,"BIT 0, L",2,8};
        case 0x46: return Instruction{opcode,"BIT 0, (HL)",2,16};
        case 0x47: return Instruction{opcode,"BIT 0, A",2,8};

        case 0x48: return Instruction{opcode,"BIT 1, B",2,8};
        case 0x49: return Instruction{opcode,"BIT 1, C",2,8};
        case 0x4A: return Instruction{opcode,"BIT 1, D",2,8};
        case 0x4B: return Instruction{opcode,"BIT 1, E",2,8};
        case 0x4C: return Instruction{opcode,"BIT 1, H",2,8};
        case 0x4D: return Instruction{opcode,"BIT 1, L",2,8};
        case 0x4E: return Instruction{opcode,"BIT 1, (HL)",2,16};
        case 0x4F: return Instruction{opcode,"BIT 1, A",2,8};

        case 0x50: return Instruction{opcode,"BIT 2, B",2,8};
        case 0x51: return Instruction{opcode,"BIT 2, C",2,8};
        case 0x52: return Instruction{opcode,"BIT 2, D",2,8};
        case 0x53: return Instruction{opcode,"BIT 2, E",2,8};
        case 0x54: return Instruction{opcode,"BIT 2, H",2,8};
        case 0x55: return Instruction{opcode,"BIT 2, L",2,8};
        case 0x56: return Instruction{opcode,"BIT 2, (HL)",2,16};
        case 0x57: return Instruction{opcode,"BIT 2, A",2,8};

        case 0x58: return Instruction{opcode,"BIT 3, B",2,8};
        case 0x59: return Instruction{opcode,"BIT 3, C",2,8};
        case 0x5A: return Instruction{opcode,"BIT 3, D",2,8};
        case 0x5B: return Instruction{opcode,"BIT 3, E",2,8};
        case 0x5C: return Instruction{opcode,"BIT 3, H",2,8};
        case 0x5D: return Instruction{opcode,"BIT 3, L",2,8};
        case 0x5E: return Instruction{opcode,"BIT 3, (HL)",2,16};
        case 0x5F: return Instruction{opcode,"BIT 3, A",2,8};

        case 0x60: return Instruction{opcode,"BIT 4, B",2,8};
        case 0x61: return Instruction{opcode,"BIT 4, C",2,8};
        case 0x62: return Instruction{opcode,"BIT 4, D",2,8};
        case 0x63: return Instruction{opcode,"BIT 4, E",2,8};
        case 0x64: return Instruction{opcode,"BIT 4, H",2,8};
        case 0x65: return Instruction{opcode,"BIT 4, L",2,8};
        case 0x66: return Instruction{opcode,"BIT 4, (HL)",2,16};
        case 0x67: return Instruction{opcode,"BIT 4, A",2,8};

        case 0x68: return Instruction{opcode,"BIT 5, B",2,8};
        case 0x69: return Instruction{opcode,"BIT 5, C",2,8};
        case 0x6A: return Instruction{opcode,"BIT 5, D",2,8};
        case 0x6B: return Instruction{opcode,"BIT 5, E",2,8};
        case 0x6C: return Instruction{opcode,"BIT 5, H",2,8};
        case 0x6D: return Instruction{opcode,"BIT 5, L",2,8};
        case 0x6E: return Instruction{opcode,"BIT 5, (HL)",2,16};
        case 0x6F: return Instruction{opcode,"BIT 5, A",2,8};

        case 0x70: return Instruction{opcode,"BIT 6, B",2,8};
        case 0x71: return Instruction{opcode,"BIT 6, C",2,8};
        case 0x72: return Instruction{opcode,"BIT 6, D",2,8};
        case 0x73: return Instruction{opcode,"BIT 6, E",2,8};
        case 0x74: return Instruction{opcode,"BIT 6, H",2,8};
        case 0x75: return Instruction{opcode,"BIT 6, L",2,8};
        case 0x76: return Instruction{opcode,"BIT 6, (HL)",2,16};
        case 0x77: return Instruction{opcode,"BIT 6, A",2,8};

        case 0x78: return Instruction{opcode,"BIT 7, B",2,8};
        case 0x79: return Instruction{opcode,"BIT 7, C",2,8};
        case 0x7A: return Instruction{opcode,"BIT 7, D",2,8};
        case 0x7B: return Instruction{opcode,"BIT 7, E",2,8};
        case 0x7C: return Instruction{opcode,"BIT 7, H",2,8};
        case 0x7D: return Instruction{opcode,"BIT 7, L",2,8};
        case 0x7E: return Instruction{opcode,"BIT 7, (HL)",2,16};
        case 0x7F: return Instruction{opcode,"BIT 7, A",2,8};

        case 0x80: return Instruction{opcode,"RES 0, B",2,8};
        case 0x81: return Instruction{opcode,"RES 0, C",2,8};
        case 0x82: return Instruction{opcode,"RES 0, D",2,8};
        case 0x83: return Instruction{opcode,"RES 0, E",2,8};
        case 0x84: return Instruction{opcode,"RES 0, H",2,8};
        case 0x85: return Instruction{opcode,"RES 0, L",2,8};
        case 0x86: return Instruction{opcode,"RES 0, (HL)",2,16};
        case 0x87: return Instruction{opcode,"RES 0, A",2,8};

        case 0x88: return Instruction{opcode,"RES 1, B",2,8};
        case 0x89: return Instruction{opcode,"RES 1, C",2,8};
        case 0x8A: return Instruction{opcode,"RES 1, D",2,8};
        case 0x8B: return Instruction{opcode,"RES 1, E",2,8};
        case 0x8C: return Instruction{opcode,"RES 1, H",2,8};
        case 0x8D: return Instruction{opcode,"RES 1, L",2,8};
        case 0x8E: return Instruction{opcode,"RES 1, (HL)",2,16};
        case 0x8F: return Instruction{opcode,"RES 1, A",2,8};

        case 0x90: return Instruction{opcode,"RES 2, B",2,8};
        case 0x91: return Instruction{opcode,"RES 2, C",2,8};
        case 0x92: return Instruction{opcode,"RES 2, D",2,8};
        case 0x93: return Instruction{opcode,"RES 2, E",2,8};
        case 0x94: return Instruction{opcode,"RES 2, H",2,8};
        case 0x95: return Instruction{opcode,"RES 2, L",2,8};
        case 0x96: return Instruction{opcode,"RES 2, (HL)",2,16};
        case 0x97: return Instruction{opcode,"RES 2, A",2,8};

        case 0x98: return Instruction{opcode,"RES 3, B",2,8};
        case 0x99: return Instruction{opcode,"RES 3, C",2,8};
        case 0x9A: return Instruction{opcode,"RES 3, D",2,8};
        case 0x9B: return Instruction{opcode,"RES 3, E",2,8};
        case 0x9C: return Instruction{opcode,"RES 3, H",2,8};
        case 0x9D: return Instruction{opcode,"RES 3, L",2,8};
        case 0x9E: return Instruction{opcode,"RES 3, (HL)",2,16};
        case 0x9F: return Instruction{opcode,"RES 3, A",2,8};

        case 0xA0: return Instruction{opcode,"RES 4, B",2,8};
        case 0xA1: return Instruction{opcode,"RES 4, C",2,8};
        case 0xA2: return Instruction{opcode,"RES 4, D",2,8};
        case 0xA3: return Instruction{opcode,"RES 4, E",2,8};
        case 0xA4: return Instruction{opcode,"RES 4, H",2,8};
        case 0xA5: return Instruction{opcode,"RES 4, L",2,8};
        case 0xA6: return Instruction{opcode,"RES 4, (HL)",2,16};
        case 0xA7: return Instruction{opcode,"RES 4, A",2,8};

        case 0xA8: return Instruction{opcode,"RES 5, B",2,8};
        case 0xA9: return Instruction{opcode,"RES 5, C",2,8};
        case 0xAA: return Instruction{opcode,"RES 5, D",2,8};
        case 0xAB: return Instruction{opcode,"RES 5, E",2,8};
        case 0xAC: return Instruction{opcode,"RES 5, H",2,8};
        case 0xAD: return Instruction{opcode,"RES 5, L",2,8};
        case 0xAE: return Instruction{opcode,"RES 5, (HL)",2,16};
        case 0xAF: return Instruction{opcode,"RES 5, A",2,8};

        case 0xB0: return Instruction{opcode,"RES 6, B",2,8};
        case 0xB1: return Instruction{opcode,"RES 6, C",2,8};
        case 0xB2: return Instruction{opcode,"RES 6, D",2,8};
        case 0xB3: return Instruction{opcode,"RES 6, E",2,8};
        case 0xB4: return Instruction{opcode,"RES 6, H",2,8};
        case 0xB5: return Instruction{opcode,"RES 6, L",2,8};
        case 0xB6: return Instruction{opcode,"RES 6, (HL)",2,16};
        case 0xB7: return Instruction{opcode,"RES 6, A",2,8};

        case 0xB8: return Instruction{opcode,"RES 7, B",2,8};
        case 0xB9: return Instruction{opcode,"RES 7, C",2,8};
        case 0xBA: return Instruction{opcode,"RES 7, D",2,8};
        case 0xBB: return Instruction{opcode,"RES 7, E",2,8};
        case 0xBC: return Instruction{opcode,"RES 7, H",2,8};
        case 0xBD: return Instruction{opcode,"RES 7, L",2,8};
        case 0xBE: return Instruction{opcode,"RES 7, (HL)",2,16};
        case 0xBF: return Instruction{opcode,"RES 7, A",2,8};

        case 0xC0: return Instruction{opcode,"SET 0, B",2,8};
        case 0xC1: return Instruction{opcode,"SET 0, C",2,8};
        case 0xC2: return Instruction{opcode,"SET 0, D",2,8};
        case 0xC3: return Instruction{opcode,"SET 0, E",2,8};
        case 0xC4: return Instruction{opcode,"SET 0, H",2,8};
        case 0xC5: return Instruction{opcode,"SET 0, L",2,8};
        case 0xC6: return Instruction{opcode,"SET 0, (HL)",2,16};
        case 0xC7: return Instruction{opcode,"SET 0, A",2,8};

        case 0xC8: return Instruction{opcode,"SET 1, B",2,8};
        case 0xC9: return Instruction{opcode,"SET 1, C",2,8};
        case 0xCA: return Instruction{opcode,"SET 1, D",2,8};
        case 0xCB: return Instruction{opcode,"SET 1, E",2,8};
        case 0xCC: return Instruction{opcode,"SET 1, H",2,8};
        case 0xCD: return Instruction{opcode,"SET 1, L",2,8};
        case 0xCE: return Instruction{opcode,"SET 1, (HL)",2,16};
        case 0xCF: return Instruction{opcode,"SET 1, A",2,8};

        case 0xD0: return Instruction{opcode,"SET 2, B",2,8};
        case 0xD1: return Instruction{opcode,"SET 2, C",2,8};
        case 0xD2: return Instruction{opcode,"SET 2, D",2,8};
        case 0xD3: return Instruction{opcode,"SET 2, E",2,8};
        case 0xD4: return Instruction{opcode,"SET 2, H",2,8};
        case 0xD5: return Instruction{opcode,"SET 2, L",2,8};
        case 0xD6: return Instruction{opcode,"SET 2, (HL)",2,16};
        case 0xD7: return Instruction{opcode,"SET 2, A",2,8};

        case 0xD8: return Instruction{opcode,"SET 3, B",2,8};
        case 0xD9: return Instruction{opcode,"SET 3, C",2,8};
        case 0xDA: return Instruction{opcode,"SET 3, D",2,8};
        case 0xDB: return Instruction{opcode,"SET 3, E",2,8};
        case 0xDC: return Instruction{opcode,"SET 3, H",2,8};
        case 0xDD: return Instruction{opcode,"SET 3, L",2,8};
        case 0xDE: return Instruction{opcode,"SET 3, (HL)",2,16};
        case 0xDF: return Instruction{opcode,"SET 3, A",2,8};

        case 0xE0: return Instruction{opcode,"SET 4, B",2,8};
        case 0xE1: return Instruction{opcode,"SET 4, C",2,8};
        case 0xE2: return Instruction{opcode,"SET 4, D",2,8};
        case 0xE3: return Instruction{opcode,"SET 4, E",2,8};
        case 0xE4: return Instruction{opcode,"SET 4, H",2,8};
        case 0xE5: return Instruction{opcode,"SET 4, L",2,8};
        case 0xE6: return Instruction{opcode,"SET 4, (HL)",2,16};
        case 0xE7: return Instruction{opcode,"SET 4, A",2,8};

        case 0xE8: return Instruction{opcode,"SET 5, B",2,8};
        case 0xE9: return Instruction{opcode,"SET 5, C",2,8};
        case 0xEA: return Instruction{opcode,"SET 5, D",2,8};
        case 0xEB: return Instruction{opcode,"SET 5, E",2,8};
        case 0xEC: return Instruction{opcode,"SET 5, H",2,8};
        case 0xED: return Instruction{opcode,"SET 5, L",2,8};
        case 0xEE: return Instruction{opcode,"SET 5, (HL)",2,16};
        case 0xEF: return Instruction{opcode,"SET 5, A",2,8};

        case 0xF0: return Instruction{opcode,"SET 6, B",2,8};
        case 0xF1: return Instruction{opcode,"SET 6, C",2,8};
        case 0xF2: return Instruction{opcode,"SET 6, D",2,8};
        case 0xF3: return Instruction{opcode,"SET 6, E",2,8};
        case 0xF4: return Instruction{opcode,"SET 6, H",2,8};
        case 0xF5: return Instruction{opcode,"SET 6, L",2,8};
        case 0xF6: return Instruction{opcode,"SET 6, (HL)",2,16};
        case 0xF7: return Instruction{opcode,"SET 6, A",2,8};

        case 0xF8: return Instruction{opcode,"SET 7, B",2,8};
        case 0xF9: return Instruction{opcode,"SET 7, C",2,8};
        case 0xFA: return Instruction{opcode,"SET 7, D",2,8};
        case 0xFB: return Instruction{opcode,"SET 7, E",2,8};
        case 0xFC: return Instruction{opcode,"SET 7, H",2,8};
        case 0xFD: return Instruction{opcode,"SET 7, L",2,8};
        case 0xFE: return Instruction{opcode,"SET 7, (HL)",2,16};
        case 0xFF: return Instruction{opcode,"SET 7, A",2,8};
        default: return Instruction {opcode, "UNKNOWN"};
    }
}

// SET B, X 
// X = X | (1 << B)
void set(uint8 bit, uint8 &reg){
    reg |= (1 << bit);
}

void setHL(uint8 bit){
    uint16 addr = cpu.HL();
    uint8 val = read8(addr);
    val |= (1 << bit);
    write8(addr, val);
}

// ~ = NOT
void res(uint8 bit, uint8 &reg){
    reg &= ~(1 << bit);
}

void resHL(uint8 bit){
    uint16 addr = cpu.HL();
    uint8 val = read8(addr);
    val &= ~(1 << bit);
    write8(addr, val);
}

void testbit(uint8  bit, uint8 &reg){
    cpu.setH();                                 // H flag always set
    cpu.resetN();                               // N flag always cleared
    if (!(reg & (1 << bit)))
        cpu.setZ();                             // Set Z if bit is 0
    else
        cpu.resetZ();                           // Clear Z if bit is 1
}

void shiftl(uint8 &reg){
    uint8 MSB = (reg >> 7);
    if (MSB){
        cpu.setC();
    }
    else {
        cpu.resetC();
    }
    reg = (reg << 1);
    if (reg == 0){
        cpu.setZ();
    }
    else{
        cpu.resetZ();
    }
    cpu.resetN();
    cpu.resetH();
}

void shiftlHL(uint8 bit){
    uint16 addr = cpu.HL();
    uint8 val = read8(addr);
    uint8 MSB = (val >> 7);
    if (MSB){
        cpu.setC();
    }
    else{
        cpu.resetC();
    }
    val = (val << 1);
    if (val == 0){
        cpu.setZ();
    }
    else{
        cpu.resetZ();
    }
    cpu.resetN();
    cpu.resetH();
    write8(addr, val);
}

void shiftr(uint8 &reg){
    uint8 LSB = (reg & 1);
    if (LSB){
        cpu.setC();
    }
    else{
        cpu.resetC();
    }
    reg = (reg >> 1);
    if (reg == 0){
        cpu.setZ();
    }
    else{
        cpu.resetZ();
    }
    cpu.resetN();
    cpu.resetH();
}

void shiftrHL(){
    uint16 addr = cpu.HL();
    uint8 val = read8(addr);
    uint8 LSB = (val & 1);
    if (LSB){
        cpu.setC();
    }
    else{
        cpu.resetC();
    }
    val = (val >> 1);
    if (val == 0){
        cpu.setZ();
    }
    else{
        cpu.resetZ();
    }
    cpu.resetN();
    cpu.resetH();
    write8(addr, val);
}



void swap(uint8 &reg){
    cpu.resetC();
    cpu.resetH();
    cpu.resetN();

    
    uint8 lower = reg & 0x0F;
    uint8 upper = (reg >> 4) & 0x0F;
    reg = (lower << 4) | upper;
    
    if (reg == 0){
        cpu.setZ();
    }
    else{
        cpu.resetZ();
    }
}

void swapHL(){
    uint16 addr = cpu.HL();
    uint8 val = read8(addr);
    cpu.resetC();
    cpu.resetH();
    cpu.resetN();
    
    uint8 lower = val & 0x0F;
    uint8 upper = (val >> 4) & 0x0F;
    val = (lower << 4) | upper;
    
    if (val == 0){
        cpu.setZ();
    }
    else{
        cpu.resetZ();
    }
    write8(addr, val);
}





