#include <cstdint>
#include <cstring>
#include "memory.h"
#include "mmu.h"

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t

MMU::MMU(Memory &mem) : memory(mem) {}

uint8 MMU::readIO(uint16 addr)
{
    return memory.read8(addr);
}

void MMU::writeIO(uint16 addr, uint8 val)
{
    memory.write8(addr, val);
}

void MBC_Controller::set(MBC *newMBC)
{
    if (mbc)
        delete mbc;
    mbc = newMBC;
}

uint8 MBC_Controller::read(uint16 addr)
{
    return mbc ? mbc->read(addr) : 0xFF;
}

void MBC_Controller::write(uint16 addr, uint8 val)
{
    if (mbc)
    {
        mbc->write(addr, val);
    }
}

uint8 *MBC_Controller::getROM()
{
    return mbc ? mbc->getROM() : nullptr;
}

uint8 MBC_Controller::getROMSize()
{
    return mbc ? mbc->getROMSize() : 0;
}

MBC_Controller::~MBC_Controller()
{
    if (mbc)
    {
        delete mbc;
        mbc = nullptr;
    }
}

MBC_1::MBC_1(uint8 *rom, uint32 rSize, uint32 ramS)
    : romData(rom), ramData(nullptr), romSize(rSize), ramSize(ramS),
      romBank(1), ramBank(0), ramEnabled(false), bankingMode(false)
{
    if (ramSize)
    {
        ramData = new uint8[ramSize];
        memset(ramData, 0, ramSize);
    }
}

uint8 MBC_1::read(uint16 addr)
{
    if (addr < 0x4000)
    {
        uint8 bank0 = bankingMode ? ((ramBank & 0x03) << 5) : 0;
        uint32 offset = (bank0 * 0x4000) + addr;
        return romData[offset % romSize];
    }
    if (addr < 0x8000)
    {
        uint8 bank = romBank & 0x1F;
        if (bank == 0)
            bank = 1;
        bank |= (ramBank & 0x03) << 5;
        uint32 offset = (bank * 0x4000) + (addr - 0x4000);
        return romData[offset % romSize];
    }
    if (addr >= 0xA000 && addr < 0xC000)
    {
        if (!ramEnabled || !ramData)
            return 0xFF;
        uint8 bank = bankingMode ? (ramBank & 0x03) : 0;
        uint32 offset = (bank * 0x2000) + (addr - 0xA000);
        return ramData[offset % ramSize];
    }
    return 0xFF;
}

void MBC_1::write(uint16 addr, uint8 val)
{
    if (addr < 0x2000)
    {
        ramEnabled = ((val & 0x0F) == 0x0A);
    }
    else if (addr < 0x4000)
    {
        uint8 lower = val & 0x1F;
        if (lower == 0)
            lower = 1;
        romBank = (romBank & 0x60) | lower;
    }
    else if (addr < 0x6000)
    {
        ramBank = val & 0x03;
    }
    else if (addr < 0x8000)
    {
        bankingMode = val & 1;
    }
    else if (addr >= 0xA000 && addr < 0xC000)
    {
        if (!ramEnabled || !ramData)
            return;
        uint8 bank = bankingMode ? (ramBank & 0x03) : 0;
        uint32 offset = (bank * 0x2000) + (addr - 0xA000);
        ramData[offset % ramSize] = val;
    }
}

uint8 *MBC_1::getROM()
{
    return romData;
}

uint8 MBC_1::getROMSize()
{
    return romSize;
}

MBC_2::MBC_2(uint8 *rom, uint32 rSize)
    : romData(rom), romSize(rSize), romBank(1), ramEnabled(false)
{
    memset(ram, 0x0F, sizeof(ram));
}

uint8 MBC_2::read(uint16 addr)
{
    if (addr < 0x4000)
        return romData[addr];
    if (addr < 0x8000)
    {
        uint8 bank = romBank & 0x0F;
        if (bank == 0)
            bank = 1;
        uint32 offset = (bank * 0x4000) + (addr - 0x4000);
        return romData[offset % romSize];
    }
    if (addr >= 0xA000 && addr < 0xA200)
    {
        if (!ramEnabled)
            return 0xFF;
        return ram[addr - 0xA000] & 0x0F;
    }
    return 0xFF;
}

void MBC_2::write(uint16 addr, uint8 val)
{
    if (addr < 0x4000)
    {
        if (addr & 0x0100)
        {
            romBank = val & 0x0F;
            if (romBank == 0)
                romBank = 1;
        }
        else
        {
            ramEnabled = ((val & 0x0F) == 0x0A);
        }
        return;
    }
    if (addr >= 0xA000 && addr < 0xA200)
    {
        if (ramEnabled)
            ram[addr - 0xA000] = val & 0x0F;
    }
}

uint8 *MBC_2::getROM()
{
    return romData;
}

uint8 MBC_2::getROMSize()
{
    return romSize;
}

MBC_3::MBC_3(uint8 *rom, uint32 rSize, uint32 ramS)
    : romData(rom), ramData(nullptr), romSize(rSize), ramSize(ramS),
      romBank(1), ramBank(0), ramEnabled(false)
{
    if (ramSize)
    {
        ramData = new uint8[ramSize];
        memset(ramData, 0, ramSize);
    }
}

uint8 MBC_3::read(uint16 addr)
{
    if (addr < 0x4000)
    {
        return romData[addr];
    }

    if (addr < 0x8000)
    {
        uint32 offset = (romBank * 0x4000) + (addr - 0x4000);
        return romData[offset % romSize];
    }

    if (addr >= 0xA000 && addr < 0xC000)
    {
        if (!ramEnabled)
            return 0xFF;
        if (ramBank <= 0x03 && ramData)
        {
            uint32 offset = (ramBank * 0x2000) + (addr - 0xA000);
            return ramData[offset % ramSize];
        }
    }
    return 0xFF;
}

void MBC_3::write(uint16 addr, uint8 val)
{
    if (addr < 0x2000)
    {
        ramEnabled = ((val & 0x0F) == 0x0A);
    }
    else if (addr < 0x4000)
    {
        romBank = val & 0x7F;
        if (romBank == 0)
            romBank = 1;
    }
    else if (addr < 0x6000)
    {
        if (val <= 0x03)
            ramBank = val;
    }
}

uint8 *MBC_3::getROM()
{
    return romData;
}

uint8 MBC_3::getROMSize()
{
    return romSize;
}

MBC_4::MBC_4(uint8 *rom, uint32 rSize, uint32 ramS)
    : romData(rom), ramData(nullptr), romSize(rSize), ramSize(ramS),
      romBank(1), ramBank(0), ramEnabled(false)
{
    if (ramSize)
    {
        ramData = new uint8[ramSize];
        memset(ramData, 0, ramSize);
    }
}

uint8 MBC_4::read(uint16 addr)
{
    if (addr < 0x4000)
        return romData[addr];
    if (addr < 0x8000)
    {
        uint32 offset = (romBank * 0x4000) + (addr - 0x4000);
        return romData[offset % romSize];
    }
    if (addr >= 0xA000 && addr < 0xC000)
    {
        if (!ramEnabled || !ramData)
            return 0xFF;
        uint32 offset = (ramBank * 0x2000) + (addr - 0xA000);
        return ramData[offset % ramSize];
    }
    return 0xFF;
}

void MBC_4::write(uint16 addr, uint8 val)
{
    if (addr < 0x2000)
    {
        ramEnabled = ((val & 0x0F) == 0x0A);
    }
    else if (addr < 0x4000)
    {
        romBank = val ? val : 1;
    }
    else if (addr < 0x6000)
    {
        ramBank = val & 0x03;
    }
    else if (addr >= 0xA000 && addr < 0xC000)
    {
        if (!ramEnabled || !ramData)
            return;
        uint32 offset = (ramBank * 0x2000) + (addr - 0xA000);
        ramData[offset % ramSize] = val;
    }
}

uint8 *MBC_4::getROM()
{
    return romData;
}

uint8 MBC_4::getROMSize()
{
    return romSize;
}

MBC_5::MBC_5(uint8 *rom, uint32 rSize, uint32 ramS)
    : romData(rom), ramData(nullptr), romSize(rSize), ramSize(ramS),
      romBank(1), ramBank(0), ramEnabled(false)
{
    if (ramSize)
    {
        ramData = new uint8[ramSize];
        memset(ramData, 0, ramSize);
    }
}

uint8 MBC_5::read(uint16 addr)
{
    if (addr < 0x4000)
        return romData[addr];
    if (addr < 0x8000)
    {
        uint32 offset = (romBank * 0x4000) + (addr - 0x4000);
        return romData[offset % romSize];
    }
    if (addr >= 0xA000 && addr < 0xC000)
    {
        if (!ramEnabled || !ramData)
            return 0xFF;
        uint32 offset = (ramBank * 0x2000) + (addr - 0xA000);
        return ramData[offset % ramSize];
    }
    return 0xFF;
}

void MBC_5::write(uint16 addr, uint8 val)
{
    if (addr < 0x2000)
    {
        ramEnabled = ((val & 0x0F) == 0x0A);
    }
    else if (addr < 0x3000)
    {
        romBank = (romBank & 0x100) | val;
    }
    else if (addr < 0x4000)
    {
        romBank = (romBank & 0xFF) | ((val & 1) << 8);
    }
    else if (addr < 0x6000)
    {
        ramBank = val & 0x0F;
    }
    else if (addr >= 0xA000 && addr < 0xC000)
    {
        if (!ramEnabled || !ramData)
            return;
        uint32 offset = (ramBank * 0x2000) + (addr - 0xA000);
        ramData[offset % ramSize] = val;
    }
}

uint8 *MBC_5::getROM()
{
    return romData;
}

uint8 MBC_5::getROMSize()
{
    return romSize;
}