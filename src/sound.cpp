#include "memory.h"
#include "sound.h"

void Sound::write8(uint16 addr, uint8 val)
{
    switch (addr)
    {
    case 0xFF12:
    case 0xFF17:
    case 0xFF21:
        period = val & 0x7;
        goes_up = val & 0x8 == 0x8;
        initial_volume = val >> 4;
        volume = initial_volume;
        break;
    case 0xFF14:
    case 0xFF19:
    case 0xFF23:
        if ((val & 0x80) == 0x80)
        {
            delay = period;
            volume = initial_volume;
        }
        break;
    }
}