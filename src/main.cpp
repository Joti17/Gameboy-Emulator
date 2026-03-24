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

    return 0;
}