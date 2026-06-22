#include "memory.h"
#include "sound.h"
#include "mmu.h"
#include "cpu.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <SDL2/SDL.h>

// ── Duty table ───────────────────────────────────────────────────────────────
// 4 waveforms × 8 steps
static const uint8_t DUTY_TABLE[4][8] = {
    {0,0,0,0,0,0,0,1},   // 12.5 %
    {1,0,0,0,0,0,0,1},   // 25 %
    {1,0,0,0,1,1,0,1},   // 50 %
    {0,1,1,1,1,1,1,0},   // 75 %
};

// Divisor table for noise channel
static const uint8_t NOISE_DIV[8] = {8,16,32,48,64,80,96,112};

// ── SquareChannel ────────────────────────────────────────────────────────────

void SquareChannel::trigger(bool hasSweep) {
    enabled = dacOn;

    // Length counter: if 0, reload to 64
    if (lengthCounter == 0) lengthCounter = 64;

    // Reload freq timer
    uint16 freq = ((uint16)(NRx4 & 0x07) << 8) | NRx3;
    freqTimer = (2048 - freq) * 4;

    // Envelope
    envVolume = (NRx2 >> 4) & 0x0F;
    envPeriod = NRx2 & 0x07;
    envAdd    = (NRx2 >> 3) & 1;
    envTimer  = envPeriod ? envPeriod : 8;

    // Sweep (CH1)
    if (hasSweep) {
        shadowFreq  = freq;
        sweepPeriod = (NRx0 >> 4) & 0x07;
        sweepNeg    = (NRx0 >> 3) & 1;
        sweepShift  = NRx0 & 0x07;
        sweepTimer  = sweepPeriod ? sweepPeriod : 8;
        sweepEnabled = sweepPeriod || sweepShift;
        if (sweepShift) sweepCalc();   // initial overflow check
    }
}

void SquareChannel::clockLength() {
    if ((NRx4 & 0x40) && lengthCounter > 0) {
        --lengthCounter;
        if (lengthCounter == 0) enabled = false;
    }
}

void SquareChannel::clockEnvelope() {
    if (envPeriod == 0) return;
    if (--envTimer == 0) {
        envTimer = envPeriod;
        if (envAdd && envVolume < 15) ++envVolume;
        else if (!envAdd && envVolume > 0) --envVolume;
    }
}

uint16 SquareChannel::sweepCalc() const {
    uint16 newFreq = shadowFreq >> sweepShift;
    return sweepNeg ? shadowFreq - newFreq : shadowFreq + newFreq;
}

void SquareChannel::clockSweep() {
    if (sweepTimer > 0) --sweepTimer;
    if (sweepTimer != 0) return;

    sweepTimer = sweepPeriod ? sweepPeriod : 8;
    if (!sweepEnabled || sweepPeriod == 0) return;

    uint16 newFreq = sweepCalc();
    if (newFreq > 2047) {
        enabled = false;
        return;
    }
    if (sweepShift > 0) {
        shadowFreq = newFreq;
        NRx3 = newFreq & 0xFF;
        NRx4 = (NRx4 & 0xF8) | ((newFreq >> 8) & 0x07);
        // second overflow check
        if (sweepCalc() > 2047) enabled = false;
    }
}

float SquareChannel::sample() const {
    if (!enabled || !dacOn) return 0.0f;
    uint8 duty = (NRx1 >> 6) & 0x03;
    float out = DUTY_TABLE[duty][dutyStep] ? 1.0f : -1.0f;
    return out * (envVolume / 15.0f);
}

// ── WaveChannel ──────────────────────────────────────────────────────────────

void WaveChannel::trigger() {
    enabled = dacOn;
    if (lengthCounter == 0) lengthCounter = 256;
    uint16 freq = ((uint16)(NR34 & 0x07) << 8) | NR33;
    freqTimer = (2048 - freq) * 2;
    wavePosition = 0;
    outputLevel = (NR32 >> 5) & 0x03;
}

void WaveChannel::clockLength() {
    if ((NR34 & 0x40) && lengthCounter > 0) {
        --lengthCounter;
        if (lengthCounter == 0) enabled = false;
    }
}

float WaveChannel::sample() const {
    if (!enabled || !dacOn) return 0.0f;
    uint8 byteIdx  = wavePosition / 2;
    uint8 nibble   = (wavePosition & 1) ? (waveRAM[byteIdx] & 0x0F)
                                         : (waveRAM[byteIdx] >> 4);
    if (outputLevel == 0) return 0.0f;
    uint8 shifted = nibble >> (outputLevel - 1);
    return (shifted / 7.5f) - 1.0f;   // 0-15 → -1..+1
}

// ── NoiseChannel ─────────────────────────────────────────────────────────────

void NoiseChannel::trigger() {
    enabled = dacOn;
    if (lengthCounter == 0) lengthCounter = 64;
    envVolume = (NR42 >> 4) & 0x0F;
    envPeriod = NR42 & 0x07;
    envAdd    = (NR42 >> 3) & 1;
    envTimer  = envPeriod ? envPeriod : 8;

    uint8 div   = NR43 & 0x07;
    uint8 shift = (NR43 >> 4) & 0x0F;
    freqTimer = NOISE_DIV[div] << shift;

    lfsr = 0x7FFF;
}

void NoiseChannel::clockLength() {
    if ((NR44 & 0x40) && lengthCounter > 0) {
        --lengthCounter;
        if (lengthCounter == 0) enabled = false;
    }
}

void NoiseChannel::clockEnvelope() {
    if (envPeriod == 0) return;
    if (--envTimer == 0) {
        envTimer = envPeriod;
        if (envAdd && envVolume < 15) ++envVolume;
        else if (!envAdd && envVolume > 0) --envVolume;
    }
}

float NoiseChannel::sample() const {
    if (!enabled || !dacOn) return 0.0f;
    float out = (lfsr & 1) ? -1.0f : 1.0f;
    return out * (envVolume / 15.0f);
}

// ── Sound ────────────────────────────────────────────────────────────────────

Sound::Sound(MBC_Controller& mbc, Memory* mem)
    : mbc(mbc),
      memory(*mem),
      NR52(memory.memory[a_NR52]),
      NR51(memory.memory[a_NR51]),
      NR50(memory.memory[a_NR50]),
      NR10(memory.memory[a_NR10])
{
    // Copy wave RAM initial values from memory
    std::memcpy(wave.waveRAM, &memory.memory[0xFF30], 16);

    SDL_AudioSpec want{}, got{};
    want.freq     = SAMPLE_RATE;
    want.format   = AUDIO_F32SYS;
    want.channels = 2;
    want.samples  = BUFFER_FRAMES;
    want.callback = nullptr;   // push mode

    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
    if (audioDevice == 0) {
        SDL_Log("Sound: SDL_OpenAudioDevice failed: %s", SDL_GetError());
    } else {
        // Check if SDL forced a fallback configuration
        if (got.freq != want.freq || got.format != want.format || got.channels != want.channels) {
            SDL_Log("Sound: SDL audio spec mismatch (wanted %dHz, got %dHz). Adjusting timing.",
                    want.freq, got.freq);
        }

        // Dynamically recalculate your downsampling rate based on actual hardware frequency.
        // The Game Boy CPU runs at exactly 4,194,304 T-cycles per second.
        cyclesPerSample = 4194304 / got.freq;

        SDL_PauseAudioDevice(audioDevice, 0);
    }
}

Sound::~Sound() {
    if (audioDevice) SDL_CloseAudioDevice(audioDevice);
}

void Sound::setCPU(CPU* c) { cpu = c; }

// ── write8 ───────────────────────────────────────────────────────────────────

void Sound::write8(uint16 addr, uint8 val) {
    // Writes to most registers are ignored while APU is off (NR52 bit 7 = 0),
    // EXCEPT NR52 itself and wave RAM.
    if (off && addr != a_NR52 && !(addr >= 0xFF30 && addr <= 0xFF3F))
        return;

    memory.memory[addr] = val;   // always mirror into raw memory

    switch (addr) {
    // ── NR52: APU master switch ──────────────────────────────────────────
    case 0xFF26:
        if (!(val & 0x80)) {
            // power off: clear all registers
            off = true;
            clearSound();
        } else {
            off = false;
        }
        // Preserve channel status bits (read-only)
        NR52 = (val & 0x80) | 0x70 |
               (sq1.enabled  ? 0x01 : 0) |
               (sq2.enabled  ? 0x02 : 0) |
               (wave.enabled ? 0x04 : 0) |
               (noise.enabled? 0x08 : 0);
        break;

    // ── NR51: panning ────────────────────────────────────────────────────
    case 0xFF25:
        NR51 = val;
        sq1.rightOut  = val & 0x01; sq1.leftOut  = val & 0x10;
        sq2.rightOut  = val & 0x02; sq2.leftOut  = val & 0x20;
        wave.rightOut = val & 0x04; wave.leftOut = val & 0x40;
        noise.rightOut= val & 0x08; noise.leftOut= val & 0x80;
        // legacy
        CH1.left  = val & 0x10; CH1.right = val & 0x01;
        CH2.left  = val & 0x20; CH2.right = val & 0x02;
        CH3.left  = val & 0x40; CH3.right = val & 0x04;
        CH4.left  = val & 0x80; CH4.right = val & 0x08;
        break;

    // ── NR50: master volume ──────────────────────────────────────────────
    case 0xFF24:
        NR50 = val;
        volumeL = (((val >> 4) & 0x07) + 1) * volumeFactor;
        volumeR = ((val & 0x07) + 1) * volumeFactor;
        break;

    // ── CH1: sweep ───────────────────────────────────────────────────────
    case 0xFF10:
        sq1.NRx0 = val;
        break;
    case 0xFF11:
        sq1.NRx1 = val;
        sq1.lengthCounter = 64 - (val & 0x3F);
        break;
    case 0xFF12:
        sq1.NRx2 = val;
        sq1.dacOn = (val & 0xF8) != 0;
        if (!sq1.dacOn) sq1.enabled = false;
        break;
    case 0xFF13:
        sq1.NRx3 = val;
        break;
    case 0xFF14:
        sq1.NRx4 = val;
        if (val & 0x80) sq1.trigger(true);
        break;

    // ── CH2: square no sweep ─────────────────────────────────────────────
    case 0xFF16:
        sq2.NRx1 = val;
        sq2.lengthCounter = 64 - (val & 0x3F);
        break;
    case 0xFF17:
        sq2.NRx2 = val;
        sq2.dacOn = (val & 0xF8) != 0;
        if (!sq2.dacOn) sq2.enabled = false;
        break;
    case 0xFF18:
        sq2.NRx3 = val;
        break;
    case 0xFF19:
        sq2.NRx4 = val;
        if (val & 0x80) sq2.trigger(false);
        break;

    // ── CH3: wave ────────────────────────────────────────────────────────
    case 0xFF1A:
        wave.NR30 = val;
        wave.dacOn = (val & 0x80) != 0;
        if (!wave.dacOn) wave.enabled = false;
        break;
    case 0xFF1B:
        wave.NR31 = val;
        wave.lengthCounter = 256 - val;
        break;
    case 0xFF1C:
        wave.NR32 = val;
        wave.outputLevel = (val >> 5) & 0x03;
        break;
    case 0xFF1D:
        wave.NR33 = val;
        break;
    case 0xFF1E:
        wave.NR34 = val;
        if (val & 0x80) wave.trigger();
        break;

    // ── CH4: noise ───────────────────────────────────────────────────────
    case 0xFF20:
        noise.NR41 = val;
        noise.lengthCounter = 64 - (val & 0x3F);
        break;
    case 0xFF21:
        noise.NR42 = val;
        noise.dacOn = (val & 0xF8) != 0;
        if (!noise.dacOn) noise.enabled = false;
        break;
    case 0xFF22:
        noise.NR43 = val;
        break;
    case 0xFF23:
        noise.NR44 = val;
        if (val & 0x80) noise.trigger();
        break;

    // ── Wave RAM ─────────────────────────────────────────────────────────
    default:
        if (addr >= 0xFF30 && addr <= 0xFF3F)
            wave.waveRAM[addr - 0xFF30] = val;
        break;
    }
}

// ── tick (called every T-cycle block from main loop) ─────────────────────────

void Sound::tick(uint32 cycles) {
    if (audioDevice == 0) return;

    tickFrameSequencer(cycles);
    tickChannels(cycles);
}

void Sound::tickFrameSequencer(uint32 cycles) {
    frameSeqCycles += cycles;
    while (frameSeqCycles >= 8192) {
        frameSeqCycles -= 8192;

        // Frame sequencer step
        switch (frameSeqStep & 7) {
        case 0: sq1.clockLength(); sq2.clockLength();
                wave.clockLength(); noise.clockLength(); break;
        case 2: sq1.clockLength(); sq2.clockLength();
                wave.clockLength(); noise.clockLength();
                sq1.clockSweep();   break;
        case 4: sq1.clockLength(); sq2.clockLength();
                wave.clockLength(); noise.clockLength(); break;
        case 6: sq1.clockLength(); sq2.clockLength();
                wave.clockLength(); noise.clockLength();
                sq1.clockSweep();   break;
        case 7: sq1.clockEnvelope(); sq2.clockEnvelope();
                noise.clockEnvelope(); break;
        default: break;
        }
        frameSeqStep = (frameSeqStep + 1) & 7;

        // Update NR52 channel-active bits
        NR52 = (NR52 & 0xF0) |
               (sq1.enabled   ? 0x01 : 0) |
               (sq2.enabled   ? 0x02 : 0) |
               (wave.enabled  ? 0x04 : 0) |
               (noise.enabled ? 0x08 : 0);
    }
}

void Sound::tickChannels(uint32 cycles) {
    // Run one T-cycle at a time for accuracy; channels tick at 4 MHz.
    for (uint32 i = 0; i < cycles; ++i) {
        // ── Square 1 ──
        if (sq1.freqTimer > 0) --sq1.freqTimer;
        if (sq1.freqTimer == 0) {
            uint16 freq = ((uint16)(sq1.NRx4 & 0x07) << 8) | sq1.NRx3;
            sq1.freqTimer = (2048 - freq) * 4;
            sq1.dutyStep = (sq1.dutyStep + 1) & 7;
        }

        // ── Square 2 ──
        if (sq2.freqTimer > 0) --sq2.freqTimer;
        if (sq2.freqTimer == 0) {
            uint16 freq = ((uint16)(sq2.NRx4 & 0x07) << 8) | sq2.NRx3;
            sq2.freqTimer = (2048 - freq) * 4;
            sq2.dutyStep = (sq2.dutyStep + 1) & 7;
        }

        // ── Wave ──
        if (wave.freqTimer > 0) --wave.freqTimer;
        if (wave.freqTimer == 0) {
            uint16 freq = ((uint16)(wave.NR34 & 0x07) << 8) | wave.NR33;
            wave.freqTimer = (2048 - freq) * 2;
            wave.wavePosition = (wave.wavePosition + 1) & 31;
        }

        // ── Noise ──
        if (noise.freqTimer > 0) --noise.freqTimer;
        if (noise.freqTimer == 0) {
            uint8  div   = noise.NR43 & 0x07;
            uint8  shift = (noise.NR43 >> 4) & 0x0F;
            noise.freqTimer = (uint16)(NOISE_DIV[div]) << shift;
            if (noise.freqTimer == 0) noise.freqTimer = 8;

            uint16 xored = (noise.lfsr ^ (noise.lfsr >> 1)) & 1;
            noise.lfsr >>= 1;
            noise.lfsr |= xored << 14;
            if (noise.NR43 & 0x08) {   // 7-bit mode
                noise.lfsr &= ~(1 << 6);
                noise.lfsr |= xored << 6;
            }
        }

        // ── Accumulate for downsampling ──
        accumCycles++;
        if (accumCycles >= cyclesPerSample) {
            accumCycles -= cyclesPerSample;

            float s1 = sq1.sample();
            float s2 = sq2.sample();
            float s3 = wave.sample();
            float s4 = noise.sample();

            float left  = 0.0f, right = 0.0f;
            if (!off) {
                if (sq1.leftOut)   left  += s1;
                if (sq1.rightOut)  right += s1;
                if (sq2.leftOut)   left  += s2;
                if (sq2.rightOut)  right += s2;
                if (wave.leftOut)  left  += s3;
                if (wave.rightOut) right += s3;
                if (noise.leftOut) left  += s4;
                if (noise.rightOut)right += s4;

                left  = (left  / 4.0f) * (float)volumeL;
                right = (right / 4.0f) * (float)volumeR;

                left  = std::max(-1.0f, std::min(1.0f, left));
                right = std::max(-1.0f, std::min(1.0f, right));
            }

            pushSample(left, right);
        }
    }
}

void Sound::pushSample(float left, float right) {
    float samples[2] = {left, right};
    SDL_QueueAudio(audioDevice, samples, sizeof(samples));

    // Prevent audio queue from growing unbounded
    // Target: ~2 frames of audio latency (2 * 1/60 * SAMPLE_RATE * 2ch * 4bytes)
    const uint32_t MAX_QUEUE = SAMPLE_RATE / 20 * 2 * sizeof(float);
    while (SDL_GetQueuedAudioSize(audioDevice) > MAX_QUEUE) {
        SDL_Delay(1);
    }
}

// ── Legacy stubs ─────────────────────────────────────────────────────────────

void Sound::clearSound() {
    // Clear all channel registers when APU powers off
    for (uint16 a = 0xFF10; a <= 0xFF25; ++a)
        memory.memory[a] = 0;

    sq1  = SquareChannel{};
    sq2  = SquareChannel{};
    wave = WaveChannel{};
    noise = NoiseChannel{};
    frameSeqStep = 0;
}

void Sound::scanNR10() {
    sq1.sweepPeriod = (NR10 >> 4) & 0x07;
    sq1.sweepNeg    = (NR10 >> 3) & 1;
    sq1.sweepShift  = NR10 & 0x07;
    CH1.pace = sq1.sweepPeriod;
}

void Sound::updatePeriod(LegacyChannel&) {
    // no-op: frame sequencer handles sweep timing now
}