#include "basetest.h"
#include <iostream>
#include <sstream>

int main(){
    std::stringstream test_buffer;
    std::streambuf* original_cout_buffer = std::cout.rdbuf(test_buffer.rdbuf()); // for testing if anyhting was printed
    std::streampos pos = test_buffer.tellp(); 

    BaseTest test;

    // "normal" Instructions
    for (int i = 0; i <= 0xFF; i++){
        switch(i){
            case 0xfd: continue;
            case 0xfc: continue;
            case 0xf4: continue;
            case 0xed: continue;
            case 0xec: continue;
            case 0xeb: continue;
            case 0xe4: continue;
            case 0xe3: continue;
            case 0xdd: continue;
            case 0xdb: continue;
            case 0xd3: continue;
            case 0xcb: continue;
        }
        test.cpu.PC = 0x100;
        test.cpu.execute((uint16_t)i);
    }
    // CB-Instructions
    for (int i = 0; i <= 0xFF; i++){
        test.cpu.PC = 0x100;
        test.cpu.execute(0xCB00 | i);
    }

    std::cout.rdbuf(original_cout_buffer);

    if (test_buffer.tellp() != pos) {
        return 1; // TODO: add universal test
    }
    return 0;
}

int test_execute(){
    return main();
}