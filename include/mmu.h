#ifndef MMU_H
#define MMU_H

#include <cstdint>
#include <cstring>

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t

struct Memory;

#define ROM_ONLY         0x00
#define MBC1             0x01
#define MBC1_RAM         0x02
#define MBC1_RAM_BATTERY 0x03

#define MBC2             0x05
#define MBC2_BATTERY     0x06

#define ROM_RAM          0x08
#define ROM_RAM_BATTERY  0x09

#define MBC3             0x11
#define MBC3_RAM         0x12
#define MBC3_RAM_BATTERY 0x13

#define MBC4             0x15
#define MBC4_RAM         0x16
#define MBC4_RAM_BATTERY 0x17

#define MBC5             0x19
#define MBC5_RAM         0x1A
#define MBC5_RAM_BATTERY 0x1B

#define MBC5_RUMBLE              0x1C
#define MBC5_RUMBLE_RAM          0x1D
#define MBC5_RUMBLE_RAM_BATTERY  0x1E

struct MMU {
    Memory& memory;
    MMU(Memory& mem);
    uint8 readIO(uint16 addr);
    void writeIO(uint16 addr, uint8 val);
};

struct MBC {
    virtual uint8 read(uint16 addr) = 0;
    virtual void write(uint16 addr, uint8 val) = 0;
    virtual ~MBC() = default;
};

struct MBC_Controller {
    MBC* mbc = nullptr;
    void set(MBC* newMBC);
    uint8 read(uint16 addr);
    void write(uint16 addr, uint8 val);
};

struct MBC_1 : MBC {
    uint8* romData;
    uint8* ramData;
    uint32 romSize;
    uint32 ramSize;
    uint8  romBank;
    uint8  ramBank;
    bool   ramEnabled;
    bool   bankingMode;

    MBC_1(uint8* rom, uint32 rSize, uint32 ramS);
    ~MBC_1() { delete[] ramData; }
    uint8 read(uint16 addr) override;
    void write(uint16 addr, uint8 val) override;
};

struct MBC_2 : MBC {
    uint8* romData;
    uint8  ram[512];
    uint32 romSize;
    uint8  romBank;
    bool   ramEnabled;

    MBC_2(uint8* rom, uint32 rSize);
    uint8 read(uint16 addr) override;
    void write(uint16 addr, uint8 val) override;
};

struct MBC_3 : MBC {
    uint8* romData;
    uint8* ramData;
    uint32 romSize;
    uint32 ramSize;
    uint8  romBank;
    uint8  ramBank;
    bool   ramEnabled;

    MBC_3(uint8* rom, uint32 rSize, uint32 ramS);
    ~MBC_3() { delete[] ramData; }
    uint8 read(uint16 addr) override;
    void write(uint16 addr, uint8 val) override;
};

struct MBC_4 : MBC {
    uint8*  romData;
    uint8*  ramData;
    uint32  romSize;
    uint32  ramSize;
    uint16  romBank;
    uint8   ramBank;
    bool    ramEnabled;

    MBC_4(uint8* rom, uint32 rSize, uint32 ramS);
    ~MBC_4() { delete[] ramData; }
    uint8 read(uint16 addr) override;
    void write(uint16 addr, uint8 val) override;
};

struct MBC_5 : MBC {
    uint8*  romData;
    uint8*  ramData;
    uint32  romSize;
    uint32  ramSize;
    uint16  romBank;
    uint8   ramBank;
    bool    ramEnabled;

    MBC_5(uint8* rom, uint32 rSize, uint32 ramS);
    ~MBC_5() { delete[] ramData; }
    uint8 read(uint16 addr) override;
    void write(uint16 addr, uint8 val) override;
};

#endif