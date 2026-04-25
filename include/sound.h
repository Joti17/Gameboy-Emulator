#ifndef SOUND_H
#define SOUND_H
#include "mmu.h"

#define uint8 uint8_t
#define uint16 uint16_t

struct Sound{
    uint8 period;
    bool goes_up;
    uint8 delay;
    uint8 initial_volume;
    uint8 volume;

    MBC_Controller controller;
    Sound(MBC_Controller Controller) : controller(Controller){}; 

    void write8(uint16 addr, uint8 val);
};



#endif