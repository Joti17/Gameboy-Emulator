#ifndef SOUND_H
#define SOUND_H
#include "mmu.h"

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t

struct SquareChannel
{
    uint8 duty = 0;
    uint8 length_ctr = 0;

    uint8 env_initial = 0;
    bool env_add = false;
    uint8 env_period = 0;
    uint8 env_timer = 0;
    uint8 volume = 0;

    uint16 freq_bits = 0;
    bool length_en = false;
    bool enabled = false;

    uint8 sweep_period = 0;
    bool sweep_negate = false;
    uint8 sweep_shift = 0;
    uint8 sweep_timer = 0;
    uint16 sweep_shadow = 0;
    bool sweep_enabled = false;

    uint8 duty_pos = 0;
    uint32 freq_timer = 0;

    uint8 reg[5] = {};
};

struct WaveChannel
{
    bool dac_on = false;
    uint16 length_ctr = 0;
    bool length_en = false;
    uint8 output_lvl = 0;
    uint16 freq_bits = 0;
    bool enabled = false;
    uint8 wave_ram[16] = {};
    uint8 pos = 0;
    uint32 freq_timer = 0;
    uint8 reg[5] = {};
};

struct NoiseChannel
{
    uint8 length_ctr = 0;
    bool length_en = false;
    uint8 env_initial = 0;
    bool env_add = false;
    uint8 env_period = 0;
    uint8 env_timer = 0;
    uint8 volume = 0;
    uint8 clock_shift = 0;
    bool width_mode = false;
    uint8 divisor_code = 0;
    uint16 lfsr = 0x7FFF;
    bool enabled = false;
    uint32 freq_timer = 0;
    uint8 reg[5] = {};
};

struct Sound
{
    int sample_rate = 44100;

    MBC_Controller &controller;

    Sound(MBC_Controller &ctrl) : controller(ctrl) {}

    void write8(uint16 addr, uint8 val);
    uint8 read8(uint16 addr) const;
    void update(uint32 cycles);
    void fill_buffer(float *stream, int len);

    bool power = true;
    uint8 nr50 = 0x77;
    uint8 nr51 = 0xF3;
    uint8 nr52 = 0xF1;

    SquareChannel ch1;
    SquareChannel ch2;
    WaveChannel ch3;
    NoiseChannel ch4;

private:
    uint32 fs_timer = 8192;
    uint8 fs_step = 0;

    void tick_frame_sequencer();
    void clock_length(SquareChannel &ch);
    void clock_length_wave();
    void clock_length_noise();
    void clock_envelope(SquareChannel &ch);
    void clock_envelope_noise();
    void clock_sweep();
    uint16 sweep_calculate();

    void trigger_square(SquareChannel &ch, bool is_ch1);
    void trigger_wave();
    void trigger_noise();

    void power_off();
    void power_on();

    static const uint8 DUTY[4][8];
    static const uint8 DIVISORS[8];
    static const uint8 READ_MASK[0x27];
};

#endif