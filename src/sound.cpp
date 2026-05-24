#include "sound.h"
#include <cstring>

const uint8 Sound::READ_MASK[0x27] = {
    0x80, 0x3F, 0x00, 0xFF, 0xBF,
    0xFF, 0x3F, 0x00, 0xFF, 0xBF,
    0x7F, 0xFF, 0x9F, 0xFF, 0xBF,
    0xFF, 0xFF, 0x00, 0x00, 0xBF,
    0x00, 0x00, 0x70,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

const uint8 Sound::DUTY[4][8] = {
    {0,0,0,0,0,0,0,1},  // 12.5%
    {1,0,0,0,0,0,0,1},  // 25%
    {1,0,0,0,1,1,1,1},  // 50%
    {0,1,1,1,1,1,1,0},  // 75%
};

const uint8 Sound::DIVISORS[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };


static inline bool square_dac_on(const SquareChannel &ch) {
    return (ch.env_initial != 0) || ch.env_add;
}


void Sound::power_off()
{
    power = false;
    ch1 = SquareChannel{};
    ch2 = SquareChannel{};
    ch3 = WaveChannel{};
    ch4 = NoiseChannel{};
    nr50 = 0;
    nr51 = 0;
    fs_timer = 8192;
    fs_step  = 0;
}

void Sound::power_on()
{
    power    = true;
    fs_step  = 0;
    fs_timer = 8192;
}


void Sound::write8(uint16 addr, uint8 val)
{
    if (addr == 0xFF26) {
        bool now_on = (val & 0x80) != 0;
        if (power && !now_on) power_off();
        else if (!power && now_on) power_on();
        nr52 = (nr52 & 0x0F) | (val & 0x80);
        return;
    }

    if (addr >= 0xFF30 && addr <= 0xFF3F) {
        ch3.wave_ram[addr - 0xFF30] = val;
        return;
    }

    if (!power) return;

    if (addr == 0xFF24) { nr50 = val; return; }
    if (addr == 0xFF25) { nr51 = val; return; }

    switch (addr)
    {
    case 0xFF10:
        ch1.sweep_period  = (val >> 4) & 0x07;
        ch1.sweep_negate  = (val & 0x08) != 0;
        ch1.sweep_shift   = val & 0x07;
        ch1.reg[0] = val;
        break;

    case 0xFF11:
        ch1.duty       = (val >> 6) & 0x03;
        ch1.length_ctr = 64 - (val & 0x3F);
        ch1.reg[1]     = val;
        break;

    case 0xFF12:
        ch1.env_initial = val >> 4;
        ch1.env_add     = (val & 0x08) != 0;
        ch1.env_period  = val & 0x07;
        ch1.reg[2]      = val;
        if (!square_dac_on(ch1)) ch1.enabled = false;
        break;

    case 0xFF13:
        ch1.freq_bits = (ch1.freq_bits & 0x0700) | val;
        ch1.reg[3]    = val;
        break;

    case 0xFF14:
        ch1.freq_bits = (ch1.freq_bits & 0x00FF) | ((uint16)(val & 0x07) << 8);
        ch1.length_en = (val & 0x40) != 0;
        ch1.reg[4]    = val;
        if ((val & 0x40) && !(fs_step & 1) && ch1.length_ctr > 0)
            clock_length(ch1);
        if (val & 0x80)
            trigger_square(ch1, true);
        break;

    case 0xFF15: break;

    case 0xFF16:
        ch2.duty       = (val >> 6) & 0x03;
        ch2.length_ctr = 64 - (val & 0x3F);
        ch2.reg[1]     = val;
        break;

    case 0xFF17:
        ch2.env_initial = val >> 4;
        ch2.env_add     = (val & 0x08) != 0;
        ch2.env_period  = val & 0x07;
        ch2.reg[2]      = val;
        if (!square_dac_on(ch2)) ch2.enabled = false;
        break;

    case 0xFF18:
        ch2.freq_bits = (ch2.freq_bits & 0x0700) | val;
        ch2.reg[3]    = val;
        break;

    case 0xFF19:
        ch2.freq_bits = (ch2.freq_bits & 0x00FF) | ((uint16)(val & 0x07) << 8);
        ch2.length_en = (val & 0x40) != 0;
        ch2.reg[4]    = val;
        if ((val & 0x40) && !(fs_step & 1) && ch2.length_ctr > 0)
            clock_length(ch2);
        if (val & 0x80)
            trigger_square(ch2, false);
        break;

    case 0xFF1A:
        ch3.dac_on = (val & 0x80) != 0;
        ch3.reg[0] = val;
        if (!ch3.dac_on) ch3.enabled = false;
        break;

    case 0xFF1B:
        ch3.length_ctr = 256 - val;
        ch3.reg[1]     = val;
        break;

    case 0xFF1C:
        ch3.output_lvl = (val >> 5) & 0x03;
        ch3.reg[2]     = val;
        break;

    case 0xFF1D:
        ch3.freq_bits = (ch3.freq_bits & 0x0700) | val;
        ch3.reg[3]    = val;
        break;

    case 0xFF1E:
        ch3.freq_bits = (ch3.freq_bits & 0x00FF) | ((uint16)(val & 0x07) << 8);
        ch3.length_en = (val & 0x40) != 0;
        ch3.reg[4]    = val;
        if ((val & 0x40) && !(fs_step & 1) && ch3.length_ctr > 0)
            clock_length_wave();
        if (val & 0x80)
            trigger_wave();
        break;

    case 0xFF1F: break;

    case 0xFF20:
        ch4.length_ctr = 64 - (val & 0x3F);
        ch4.reg[1]     = val;
        break;

    case 0xFF21:
        ch4.env_initial = val >> 4;
        ch4.env_add     = (val & 0x08) != 0;
        ch4.env_period  = val & 0x07;
        ch4.reg[2]      = val;
        if (ch4.env_initial == 0 && !ch4.env_add) ch4.enabled = false;
        break;

    case 0xFF22:
        ch4.clock_shift   = val >> 4;
        ch4.width_mode    = (val & 0x08) != 0;
        ch4.divisor_code  = val & 0x07;
        ch4.reg[3]        = val;
        break;

    case 0xFF23:
        ch4.length_en = (val & 0x40) != 0;
        ch4.reg[4]    = val;
        if ((val & 0x40) && !(fs_step & 1) && ch4.length_ctr > 0)
            clock_length_noise();
        if (val & 0x80)
            trigger_noise();
        break;
    }
}


uint8 Sound::read8(uint16 addr) const
{
    if (addr == 0xFF26) {
        uint8 s = power ? 0x80 : 0x00;
        if (power) {
            if (ch1.enabled) s |= 0x01;
            if (ch2.enabled) s |= 0x02;
            if (ch3.enabled) s |= 0x04;
            if (ch4.enabled) s |= 0x08;
        }
        return s | 0x70;
    }

    if (addr >= 0xFF30 && addr <= 0xFF3F)
        return ch3.wave_ram[addr - 0xFF30];

    if (addr >= 0xFF10 && addr <= 0xFF25) {
        uint8 raw = 0x00;
        switch (addr) {
            case 0xFF10: raw = ch1.reg[0]; break;
            case 0xFF11: raw = ch1.reg[1]; break;
            case 0xFF12: raw = ch1.reg[2]; break;
            case 0xFF13: raw = ch1.reg[3]; break;
            case 0xFF14: raw = ch1.reg[4]; break;
            case 0xFF15: raw = 0x00;       break;
            case 0xFF16: raw = ch2.reg[1]; break;
            case 0xFF17: raw = ch2.reg[2]; break;
            case 0xFF18: raw = ch2.reg[3]; break;
            case 0xFF19: raw = ch2.reg[4]; break;
            case 0xFF1A: raw = ch3.reg[0]; break;
            case 0xFF1B: raw = ch3.reg[1]; break;
            case 0xFF1C: raw = ch3.reg[2]; break;
            case 0xFF1D: raw = ch3.reg[3]; break;
            case 0xFF1E: raw = ch3.reg[4]; break;
            case 0xFF1F: raw = 0x00;       break;
            case 0xFF20: raw = ch4.reg[1]; break;
            case 0xFF21: raw = ch4.reg[2]; break;
            case 0xFF22: raw = ch4.reg[3]; break;
            case 0xFF23: raw = ch4.reg[4]; break;
            case 0xFF24: raw = nr50;       break;
            case 0xFF25: raw = nr51;       break;
        }
        return raw | READ_MASK[addr - 0xFF10];
    }

    return 0xFF;
}


void Sound::trigger_square(SquareChannel &ch, bool is_ch1)
{
    if (!square_dac_on(ch)) { ch.enabled = false; return; }
    ch.enabled = true;

    if (ch.length_ctr == 0) {
        ch.length_ctr = 64;
        if (ch.length_en && !(fs_step & 1))
            clock_length(ch);
    }

    ch.freq_timer = (2048 - ch.freq_bits) * 4;
    ch.env_timer  = ch.env_period;
    ch.volume     = ch.env_initial;

    if (is_ch1) {
        ch.sweep_shadow  = ch.freq_bits;
        ch.sweep_timer   = (ch.sweep_period != 0) ? ch.sweep_period : 8;
        ch.sweep_enabled = (ch.sweep_period != 0) || (ch.sweep_shift != 0);
        if (ch.sweep_shift != 0)
            sweep_calculate();
    }
}

void Sound::trigger_wave()
{
    if (!ch3.dac_on) { ch3.enabled = false; return; }
    ch3.enabled = true;
    if (ch3.length_ctr == 0) {
        ch3.length_ctr = 256;
        if (ch3.length_en && !(fs_step & 1))
            clock_length_wave();
    }
    ch3.freq_timer = (2048 - ch3.freq_bits) * 2;
    ch3.pos = 0;
}

void Sound::trigger_noise()
{
    if (ch4.env_initial == 0 && !ch4.env_add) { ch4.enabled = false; return; }
    ch4.enabled = true;
    if (ch4.length_ctr == 0) {
        ch4.length_ctr = 64;
        if (ch4.length_en && !(fs_step & 1))
            clock_length_noise();
    }
    ch4.env_timer  = ch4.env_period;
    ch4.volume     = ch4.env_initial;
    ch4.lfsr       = 0x7FFF;
    ch4.freq_timer = (uint32)DIVISORS[ch4.divisor_code] << ch4.clock_shift;
}


void Sound::clock_length(SquareChannel &ch)
{
    if (ch.length_en && ch.length_ctr > 0)
        if (--ch.length_ctr == 0) ch.enabled = false;
}

void Sound::clock_length_wave()
{
    if (ch3.length_en && ch3.length_ctr > 0)
        if (--ch3.length_ctr == 0) ch3.enabled = false;
}

void Sound::clock_length_noise()
{
    if (ch4.length_en && ch4.length_ctr > 0)
        if (--ch4.length_ctr == 0) ch4.enabled = false;
}


void Sound::clock_envelope(SquareChannel &ch)
{
    if (ch.env_period == 0) return;
    if (--ch.env_timer == 0) {
        ch.env_timer = ch.env_period;
        if      ( ch.env_add && ch.volume < 15) ch.volume++;
        else if (!ch.env_add && ch.volume >  0) ch.volume--;
    }
}

void Sound::clock_envelope_noise()
{
    if (ch4.env_period == 0) return;
    if (--ch4.env_timer == 0) {
        ch4.env_timer = ch4.env_period;
        if      ( ch4.env_add && ch4.volume < 15) ch4.volume++;
        else if (!ch4.env_add && ch4.volume >  0) ch4.volume--;
    }
}


uint16 Sound::sweep_calculate()
{
    uint16 delta   = ch1.sweep_shadow >> ch1.sweep_shift;
    uint16 newfreq = ch1.sweep_negate
        ? ch1.sweep_shadow - delta
        : ch1.sweep_shadow + delta;
    if (newfreq > 2047) ch1.enabled = false;
    return newfreq;
}

void Sound::clock_sweep()
{
    if (ch1.sweep_timer > 0) ch1.sweep_timer--;
    if (ch1.sweep_timer == 0) {
        ch1.sweep_timer = (ch1.sweep_period != 0) ? ch1.sweep_period : 8;
        if (ch1.sweep_enabled && ch1.sweep_period != 0) {
            uint16 newfreq = sweep_calculate();
            if (newfreq <= 2047 && ch1.sweep_shift != 0) {
                ch1.freq_bits    = newfreq;
                ch1.sweep_shadow = newfreq;
                sweep_calculate();
            }
        }
    }
}

void Sound::tick_frame_sequencer()
{
    switch (fs_step) {
        case 0: case 4:
            clock_length(ch1); clock_length(ch2);
            clock_length_wave(); clock_length_noise();
            break;
        case 2: case 6:
            clock_length(ch1); clock_length(ch2);
            clock_length_wave(); clock_length_noise();
            clock_sweep();
            break;
        case 7:
            clock_envelope(ch1); clock_envelope(ch2);
            clock_envelope_noise();
            break;
        default: break;
    }
    fs_step = (fs_step + 1) & 7;
}


void Sound::update(uint32 cycles)
{
    if (!power) return;

    if (cycles >= fs_timer) {
        cycles   -= fs_timer;
        fs_timer  = 8192;
        tick_frame_sequencer();
        if (cycles >= fs_timer) {
            cycles   -= fs_timer;
            fs_timer  = 8192;
            tick_frame_sequencer();
        }
    }
    fs_timer -= cycles;

    if (ch1.enabled) {
        if (cycles >= ch1.freq_timer) {
            ch1.duty_pos   = (ch1.duty_pos + 1) & 7;
            ch1.freq_timer = (2048 - ch1.freq_bits) * 4 - (cycles - ch1.freq_timer);
        } else {
            ch1.freq_timer -= cycles;
        }
    }

    if (ch2.enabled) {
        if (cycles >= ch2.freq_timer) {
            ch2.duty_pos   = (ch2.duty_pos + 1) & 7;
            ch2.freq_timer = (2048 - ch2.freq_bits) * 4 - (cycles - ch2.freq_timer);
        } else {
            ch2.freq_timer -= cycles;
        }
    }

    if (ch3.enabled) {
        if (cycles >= ch3.freq_timer) {
            ch3.pos        = (ch3.pos + 1) & 31;
            ch3.freq_timer = (2048 - ch3.freq_bits) * 2 - (cycles - ch3.freq_timer);
        } else {
            ch3.freq_timer -= cycles;
        }
    }

    if (ch4.enabled) {
        uint32 period = (uint32)DIVISORS[ch4.divisor_code] << ch4.clock_shift;
        if (cycles >= ch4.freq_timer) {
            uint8 xorbit = (ch4.lfsr ^ (ch4.lfsr >> 1)) & 1;
            ch4.lfsr >>= 1;
            ch4.lfsr  |= (xorbit << 14);
            if (ch4.width_mode) {
                ch4.lfsr &= ~(1 << 6);
                ch4.lfsr |= (xorbit << 6);
            }
            ch4.freq_timer = period - (cycles - ch4.freq_timer);
        } else {
            ch4.freq_timer -= cycles;
        }
    }
}

void Sound::fill_buffer(float *stream, int len)
{
    for (int i = 0; i < len; i++) {
        float sample = 0.0f;

        if (power) {
            if (ch1.enabled && square_dac_on(ch1)) {
                float amp = ch1.volume / 15.0f;
                sample += DUTY[ch1.duty][ch1.duty_pos] ? amp : -amp;
            }
            if (ch2.enabled && square_dac_on(ch2)) {
                float amp = ch2.volume / 15.0f;
                sample += DUTY[ch2.duty][ch2.duty_pos] ? amp : -amp;
            }
            if (ch3.enabled && ch3.dac_on) {
                uint8 nibble = (ch3.pos & 1)
                    ? (ch3.wave_ram[ch3.pos >> 1] & 0x0F)
                    : (ch3.wave_ram[ch3.pos >> 1] >> 4);
                static const float SHR[4] = {0.0f, 1.0f, 0.5f, 0.25f};
                sample += (nibble * SHR[ch3.output_lvl] / 15.0f) * 2.0f - 1.0f;
            }
            if (ch4.enabled && (ch4.env_initial != 0 || ch4.env_add)) {
                float amp = ch4.volume / 15.0f;
                sample += !(ch4.lfsr & 1) ? amp : -amp;
            }
            sample *= 0.25f;
        }

        stream[i] = sample;
    }
}