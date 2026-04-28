#include <cstdint>
#include <cstring>
#include "memory.h"
#include <cstdio>
#include "input.h"
#include "ppu.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "mmu.h"

#define uint8 uint8_t
#define uint16 uint16_t

#define MBC1 0x01
#define MCB1R 0x02	// R means RAM
#define MCB1RB 0x03 // B means Battery

#define MBC2 0x05
#define MBC2B 0x06

#define RR 0x08 // ROM + RAM
#define RRB 0x09

#define MBC3TB 0x0F // T = Timer
#define MBC3TRB 0x10
#define MBC3 0x11
#define MBC3R 0x12
#define MBC3RB 0x13

#define MBC4 0x15
#define MBC4R 0x16
#define MBC4RB 0x17

#define MBC5 0x19
#define MBC5R 0x1A
#define MBC5RB 0x1B

#define MBC5RUM 0x1C // RUM = Rumble
#define MBC5RUMR 0x1D
#define MBC5RUMRB 0x1E

Memory::Memory(Sound &sound, uint8 mbc) : sound(sound), mbc(mbc), controller(sound.controller)
{
	// default values on the DMG
	memset(memory, 0x00, sizeof(memory));

	memory[0xFF10] = 0x80;
	memory[0xFF11] = 0xBF;
	memory[0xFF12] = 0xF3;
	memory[0xFF14] = 0xBF;
	memory[0xFF16] = 0x3F;
	memory[0xFF19] = 0xBF;
	memory[0xFF1A] = 0x7F;
	memory[0xFF1B] = 0xFF;
	memory[0xFF1C] = 0x9F;
	memory[0xFF1E] = 0xBF;
	memory[0xFF20] = 0xFF;
	memory[0xFF23] = 0xBF;
	memory[0xFF24] = 0x77;
	memory[0xFF25] = 0xF3;
	memory[0xFF26] = 0xF1;
	memory[0xFF40] = 0x91;
	memory[0xFF47] = 0xFC;
	memory[0xFF48] = 0xFF;
	memory[0xFF49] = 0xFF;
}

uint8 Memory::read8(uint16 addr)
{
	if (addr < 0x8000 || (addr >= 0xA000 && addr < 0xC000))
	{
		return controller.read(addr);
	}
	return memory[addr];
}

uint16 Memory::read16(uint16 addr)
{
	// little Endian, because the gameboy was designed that way
	return read8(addr) | (read8(addr + 1) << 8);
}

void Memory::write8(uint16 addr, uint8 val)
{
	if (addr < 0x8000 || (addr >= 0xA000 && addr < 0xC000))
	{
		controller.write(addr, val);
		return;
	}

	switch (addr)
	{
	case 0xFF00:
		P1 = (P1 & 0x0F) | (val & 0x30);
		break;

	case 0xFF01:
		SB = val;
		break;

	case 0xFF02:
		SC = val & 0x83;
		if (SC & 0x80)
		{
			IF |= 0x08;
			SC &= ~0x80;
		}
		break;

	case 0xFF04:
		timer.internal_counter = 0;
		timer.div_counter = 0;

	case 0xFF05:
		TIMA = val;
		break;

	case 0xFF06:
		TMA = val;
		break;

	case 0xFF07:
		TAC = val;
		timer.enabled = (TAC >> 2) & 1;
		break;

	case 0xFF0F:
		IF = val;
		break;

	case 0xFF10:
	case 0xFF11:
	case 0xFF12:
	case 0xFF13:
	case 0xFF14:
	case 0xFF15:
	case 0xFF16:
	case 0xFF17:
	case 0xFF18:
	case 0xFF19:
	case 0xFF1A:
	case 0xFF1B:
	case 0xFF1C:
	case 0xFF1D:
	case 0xFF1E:
	case 0xFF20:
	case 0xFF21:
	case 0xFF22:
	case 0xFF23:
	case 0xFF24:
	case 0xFF25:
	case 0xFF26:
	case 0xFF30:
	case 0xFF31:
	case 0xFF32:
	case 0xFF33:
	case 0xFF34:
	case 0xFF35:
	case 0xFF36:
	case 0xFF37:
	case 0xFF38:
	case 0xFF39:
	case 0xFF3A:
	case 0xFF3B:
	case 0xFF3C:
	case 0xFF3D:
	case 0xFF3E:
	case 0xFF3F:
		sound.write8(addr, val);
		break;

	default:
		memory[addr] = val;
		break;
	}
}

void Memory::write16(uint16 addr, uint16 val)
{
	write8(addr, val & 0xFF);
	write8(addr + 1, (val >> 8));
}

void Memory::tickTimers(uint32_t cycles)
{
	timer.div_counter += cycles;
	while (timer.div_counter >= 256)
	{
		timer.div_counter -= 256;
		DIV++;
	}

	if (!timer.enabled)
	{
		return;
	}

	uint32_t period;
	switch (TAC & 0x03)
	{
	case 0:
		period = 1024;
		break;
	case 1:
		period = 16;
		break;
	case 2:
		period = 64;
		break;
	case 3:
		period = 256;
		break;
	default:
		period = 1024;
		break;
	}

	timer.timer_counter += cycles;
	while (timer.timer_counter >= period)
	{
		timer.timer_counter -= period;
		if (TIMA == 0xFF)
		{
			TIMA = TMA;
			IF |= 0x04;
		}
		else
		{
			TIMA++;
		}
	}
}

void Memory::open(std::ifstream &rom, SDL_Window &window)
{
    rom.seekg(0, std::ios::end);
    std::streamoff fileSize = rom.tellg();
    if (fileSize < 0x150)
    {
        std::cerr << "ROM too small" << std::endl;
        exit(-1);
    }
    size = static_cast<uint32>(fileSize);
    rom.seekg(0, std::ios::beg);
 
    if (romData) delete[] romData;

    romData = new uint8[size];
    rom.read((char *)romData, size);
    if (!rom)
    {
        std::cerr << "Failed reading ROM" << std::endl;
        exit(-1);
    }
 
    uint8 type        = romData[0x147];
    uint8 romSizeCode = romData[0x148];
    uint8 ramSizeCode = romData[0x149];
 
	uint8 headerChecksum = 0;
	for (uint16 i = 0x134; i <= 0x14C; i++){
		headerChecksum = headerChecksum - romData[i] - 1;
	}

	if (headerChecksum != romData[0x14D]){
		std::cerr << "Header Checksum incorrect. Expected: " << (int)romData[0x14D] << " Calculated: " << (int)headerChecksum << std::endl;
		// exit(-1) // wont add though
	}

    char name[17] = {0};
    for (int i = 0; i < 16; i++)
        name[i] = std::isprint((unsigned char)romData[0x134 + i]) ? romData[0x134 + i] : ' ';
    std::string tmp(name);
    tmp.erase(std::remove_if(tmp.begin(), tmp.end(), [](unsigned char c){ return std::isspace(c); }), tmp.end());
    SDL_SetWindowTitle(&window, tmp.c_str());
 
    static const uint32 romSizes[] = {
        32*1024, 64*1024, 128*1024, 256*1024,
        512*1024, 1024*1024, 2*1024*1024, 4*1024*1024, 8*1024*1024
    };
    uint32 calculatedRomSize = (romSizeCode <= 0x08) ? romSizes[romSizeCode] : size;
 
    static const uint32 ramSizes[] = { 0, 2*1024, 8*1024, 32*1024, 128*1024, 64*1024 };
    uint32 calculatedRamSize = (ramSizeCode <= 0x05) ? ramSizes[ramSizeCode] : 0;
 
    
    switch (type)
    {
    case 0x00:
    case RR:
    case RRB:
        memcpy(memory, romData, std::min(calculatedRomSize, (uint32)0x8000));
        controller.set(nullptr);
        break;
 
    case MBC1:
    case MCB1R:
    case MCB1RB:
        controller.set(new MBC_1(romData, calculatedRomSize, calculatedRamSize));
        break;
 
    case MBC2:
    case MBC2B:
        controller.set(new MBC_2(romData, calculatedRomSize));
        break;
 
    case MBC3:
    case MBC3R:
    case MBC3RB:
    case MBC3TB:
    case MBC3TRB:
        controller.set(new MBC_3(romData, calculatedRomSize, calculatedRamSize));
        break;
 
    case MBC4:
    case MBC4R:
    case MBC4RB:
        controller.set(new MBC_4(romData, calculatedRomSize, calculatedRamSize));
        break;
 
    case MBC5:
    case MBC5R:
    case MBC5RB:
    case MBC5RUM:
    case MBC5RUMR:
    case MBC5RUMRB:
        controller.set(new MBC_5(romData, calculatedRomSize, calculatedRamSize));
        break;
 
    default:
        std::cerr << "Unknown MBC type: " << (int)type << ", treating as ROM-only" << std::endl;
        memcpy(memory, romData, std::min(calculatedRomSize, (uint32)0x8000));
        controller.set(nullptr);
        break;
    }
}