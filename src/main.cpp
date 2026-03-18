#include <iostream>
#include "memory.h"
#include "cpu.h"

int main(){
    std::cout << "Hello World!" << std::endl;
    Memory memory;
    CPU cpu {memory};

    return 0;
}