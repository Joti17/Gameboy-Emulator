#include <cstdint>
#include "memory.h"

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
        A = F = B =C =D =E = H = L = 0;
        SP = 0xFFFE;    // grows downwards
    }

    // allows combining 2 registers into 16 bit
    uint16 AD() { return (A << 8) | F; }
    uint16 BC() { return (B << 8) | C; }
    uint16 DE() { return (D << 8) | E; }
    uint16 HL() { return (H << 8) | L; }
};