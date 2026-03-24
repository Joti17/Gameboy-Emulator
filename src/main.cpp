#include <iostream>
#include "memory.h"
#include "cpu.h"
#include "mmu.h"

int main(){
    std::cout << "Hello World!" << std::endl;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    Memory memory;
    CPU cpu {memory};
    MMU mmu {memory};

    while(true){
        /* Basic Structure for interupts + reading opcodes
        uint8 opcode = cpu.step();
        SDL stuff
        */

    }
    return 0;
}