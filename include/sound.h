#pragma once
#include <cstdint>
#include <SDL2/SDL.h>

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t

struct MBC_Controller;
struct Memory;
struct CPU;


struct SquareChannel
{
    uint8 NRx0 = 0;
    uint8 NRx1 = 0;
    uint8 NRx2 = 0;
    uint8 NRx3 = 0;
    uint8 NRx4 = 0;

    bool enabled = false;
    bool dacOn = false;
    bool leftOut = false;
    bool rightOut = false;

    uint16 freqTimer = 0;
    uint8 dutyStep = 0;
    
    uint8 lengthCounter = 0;

    uint8 envVolume = 0;
    uint8 envTimer = 0;
    bool envAdd = false;
    uint8 envPeriod = 0;

    uint8 sweepTimer = 0;
    uint8 sweepPeriod = 0;
    bool sweepNeg = false;
    uint8 sweepShift = 0;
    uint16 shadowFreq = 0;
    bool sweepEnabled = false;

    void trigger(bool hasSweep);
    void clockLength();
    void clockEnvelope();
    void clockSweep();
    uint16 sweepCalc() const;
    float sample() const;
};

struct WaveChannel
{
    uint8 NR30 = 0;
    uint8 NR31 = 0;
    uint8 NR32 = 0;
    uint8 NR33 = 0;
    uint8 NR34 = 0;

    bool enabled = false;
    bool dacOn = false;
    bool leftOut = false;
    bool rightOut = false;

    uint16 freqTimer = 0;
    uint8 wavePosition = 0;
    uint16 lengthCounter = 0;
    uint8 outputLevel = 0;
    uint8 waveRAM[16] = {};

    void trigger();
    void clockLength();
    float sample() const;
};

struct NoiseChannel
{
    uint8 NR41 = 0;
    uint8 NR42 = 0;
    uint8 NR43 = 0;
    uint8 NR44 = 0;

    bool enabled = false;
    bool dacOn = false;
    bool leftOut = false;
    bool rightOut = false;

    uint16 freqTimer = 0;
    uint16 lfsr = 0x7FFF;
    uint8 lengthCounter = 0;

    uint8 envVolume = 0;
    uint8 envTimer = 0;
    bool envAdd = false;
    uint8 envPeriod = 0;

    void trigger();
    void clockLength();
    void clockEnvelope();
    float sample() const;
};

struct Sound
{
    MBC_Controller &mbc;
    Memory &memory;
    CPU *cpu = nullptr;

    SDL_AudioDeviceID audioDevice = 0;
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int BUFFER_FRAMES = 1024;
    static constexpr float GB_CLOCK = 4194304.0f;

    const uint16 a_NR52 = 0xFF26;
    const uint16 a_NR51 = 0xFF25;
    const uint16 a_NR50 = 0xFF24;
    const uint16 a_NR10 = 0xFF10;

    uint8 &NR52;
    uint8 &NR51;
    uint8 &NR50;
    uint8 &NR10;

    bool off = false;

    struct LegacyChannel
    {
        double clocksPerMs = 4194.304;
        bool left = false;
        bool right = false;
        uint8 pace = 0;
        uint8 direction = 0;
        uint8 step = 0;
    };
    LegacyChannel CH1, CH2, CH3, CH4;

    const double volumeFactor = 0.125;
    double volumeL = 0.125;
    double volumeR = 0.125;

    SquareChannel sq1, sq2;
    WaveChannel wave;
    NoiseChannel noise;

    uint32 frameSeqCycles = 0;
    uint8 frameSeqStep = 0;

    uint32 cyclesPerSample = static_cast<uint32>(GB_CLOCK / SAMPLE_RATE);
    uint32 accumCycles = 0;
    float accumLeft = 0.0f;
    float accumRight = 0.0f;
    uint32 accumCount = 0;

    Sound(MBC_Controller &mbc, Memory *mem);
    ~Sound();

    void setCPU(CPU *cpu);
    void write8(uint16 addr, uint8 val);

    void tick(uint32 cycles);

    void clearSound();
    void scanNR10();
    void updatePeriod(LegacyChannel &ch);

private:
    uint16 getRawFreq(const SquareChannel &ch) const;
    uint16 getRawFreqWave() const;
    void pushSample(float left, float right);
    void tickChannels(uint32 cycles);
    void tickFrameSequencer(uint32 cycles);
};