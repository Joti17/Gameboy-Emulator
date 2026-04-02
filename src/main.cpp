#include <iostream>
#include "memory.h"
#include "cpu.h"
#include "mmu.h"
#include "input.h";

int main(){
    std::cout << "Hello World!" << std::endl;
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        exit(-1);
    }
    SDL_Event e;
    
    Memory memory;
    CPU cpu {memory};
    MMU mmu {memory};
    Input input {memory};

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
            if (e.type == SDL_JOYBUTTONDOWN){
                
            }
        }
    }
    return 0;
}