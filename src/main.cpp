#include <iostream>
#include "memory.h"
#include "cpu.h"
#include "mmu.h"

int main(){
    std::cout << "Hello World!" << std::endl;
    
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    SDL_Event e;
    
    Memory memory;
    CPU cpu {memory};
    MMU mmu {memory};

    bool running = true;
    while(running){
        /* Basic Structure for interupts + reading opcodes
        uint8 opcode = cpu.step();
        SDL stuff
        */
        while (SDL_PollEvent(&e)){
            if (e.type == SDL_QUIT){
                running = false;
            }
        }
    }
    return 0;
}