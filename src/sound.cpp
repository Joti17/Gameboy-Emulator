#include "memory.h"
#include "sound.h"

void Sound::write8(uint16 addr, uint8 val)
{
    switch (addr)
    {
    case 0xFF12:
    case 0xFF17:
        period = val & 0x07;
        goes_up = (val & 0x08) == 0x08;
        initial_volume = val >> 4;
        break;

    case 0xFF13:
    case 0xFF18:
        frequency = (frequency & 0x0700) | val;
        break;

    case 0xFF14:
    case 0xFF19:
        frequency = (frequency & 0x00FF) | ((val & 0x07) << 8);
        
        if (val & 0x80)
        {
            delay = period;
            volume = initial_volume;
        }
        break;

    case 0xFF16:
        switch (val >> 6)
        {
            case 0: duty_cycle = 0.125f; break;
            case 1: duty_cycle = 0.25f;  break;
            case 2: duty_cycle = 0.50f;  break;
            case 3: duty_cycle = 0.75f;  break;
        }
        break;

    case 0xFF24:
        break;

    case 0xFF25:
        break;

    case 0xFF26:
        if (!(val & 0x80))
        {
            volume = 0;
        }
        break;
    }
}

void Sound::update(uint8 cycles){
    static int envelope_counter = 0;
    envelope_counter += cycles;

    if (envelope_counter >= 65536)
    {
        envelope_counter -= 65536;

        if (period > 0)
        {
            if (delay > 0)
            {
                delay--;
            }

            if (delay == 0)
            {
                delay = period;

                if (goes_up && volume < 0xF)
                {
                    volume++;
                }
                else if (!goes_up && volume > 0)
                {
                    volume--;
                }
            }
        }
    }
}

void Sound::fill_buffer(float* stream, int len) {
    for (int i = 0; i < len; i++) {
        float amplitude = (float)volume / 15.0f;
        
        stream[i] = (wave_ptr < (duty_cycle * 8)) ? amplitude : -amplitude;
        
        wave_ptr = (wave_ptr + 1) % 8; 
    }
}