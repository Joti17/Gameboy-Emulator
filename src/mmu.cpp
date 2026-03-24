#include <cstdint>
#include "memory.h"
#include "mmu.h"
#include <SDL2/SDL.h>



#define uint8 uint8_t
#define uint16 uint16_t

MMU::MMU(Memory &mem) : memory(mem)
{ }

uint8 MMU::readIO(uint16 addr){
    
}

void MMU::writeIO(uint16 addr, uint8 val){
    
}