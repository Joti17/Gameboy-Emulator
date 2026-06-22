#pragma once
#include <cstdint>
#include <SDL2/SDL.h>

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t

struct MBC_Controller;
struct Memory;
struct CPU;

// ── Per-channel state ────────────────────────────────────────────────────────

struct SquareChannel
{
    // Registers (raw)
    uint8 NRx0 = 0; // CH1 sweep only
    uint8 NRx1 = 0; // duty / length
    uint8 NRx2 = 0; // envelope
    uint8 NRx3 = 0; // freq low
    uint8 NRx4 = 0; // freq high / trigger / length-enable

    // Derived state
    bool enabled = false;
    bool dacOn = false;
    bool leftOut = false;
    bool rightOut = false;

    // Frequency timer
    uint16 freqTimer = 0;
    uint8 dutyStep = 0; // 0-7

    // Length counter
    uint8 lengthCounter = 0;

    // Envelope
    uint8 envVolume = 0;
    uint8 envTimer = 0;
    bool envAdd = false;
    uint8 envPeriod = 0;

    // Sweep (CH1 only)
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
    float sample() const; // -1..+1
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
    uint8 wavePosition = 0; // 0-31
    uint16 lengthCounter = 0;
    uint8 outputLevel = 0; // 0-3
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

// ── Main Sound struct ────────────────────────────────────────────────────────

struct Sound
{
    MBC_Controller &mbc;
    Memory &memory;
    CPU *cpu = nullptr;

    // SDL audio
    SDL_AudioDeviceID audioDevice = 0;
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int BUFFER_FRAMES = 1024;
    static constexpr float GB_CLOCK = 4194304.0f;

    // Register addresses (kept for write8 dispatch)
    const uint16 a_NR52 = 0xFF26;
    const uint16 a_NR51 = 0xFF25;
    const uint16 a_NR50 = 0xFF24;
    const uint16 a_NR10 = 0xFF10;

    // References into memory[] for legacy callers
    uint8 &NR52;
    uint8 &NR51;
    uint8 &NR50;
    uint8 &NR10;

    bool off = false;

    // Legacy Channel stubs so existing code compiles unchanged
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

    // Real APU channels
    SquareChannel sq1, sq2;
    WaveChannel wave;
    NoiseChannel noise;

    // Frame sequencer: fires at 512 Hz (every 8192 T-cycles)
    uint32 frameSeqCycles = 0;
    uint8 frameSeqStep = 0; // 0-7

    // Sample-rate downsampling accumulator
    uint32 cyclesPerSample = static_cast<uint32>(GB_CLOCK / SAMPLE_RATE);
    uint32 accumCycles = 0;
    float accumLeft = 0.0f;
    float accumRight = 0.0f;
    uint32 accumCount = 0;

    Sound(MBC_Controller &mbc, Memory *mem);
    ~Sound();

    void setCPU(CPU *cpu);
    void write8(uint16 addr, uint8 val);

    // Call from main loop with the same T-cycle count as ppu.tick()
    void tick(uint32 cycles);

    // Legacy stubs
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