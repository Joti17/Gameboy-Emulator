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

    uint16 frequency;
    int wave_ptr; 
    int sample_rate = 44100;
    float duty_cycle = 0.5f;

    MBC_Controller &controller;

    Sound(MBC_Controller &Controller) : controller(Controller) {};

    void write8(uint16 addr, uint8 val);
    void update(uint8 cycles);
    // the play func
    void fill_buffer(float* stream, int len);
};



#endif