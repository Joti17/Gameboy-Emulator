#include <cstdint>
#include <cstring>
#include "memory.h"
#include <cstdio>
#include "input.h"
#include "ppu.h"
#include <fstream>
#include <iostream>
#include <iomanip>
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

Memory::Memory(Sound &sound, uint8 mbc)
	: sound(sound),
	  mbc(mbc),
	  controller(sound.controller)
{
	// default values on the DMG
	memset(memory, 0x00, sizeof(memory));

	memory[0xFF04] = 0x00;
	memory[0xFF05] = 0x00;
	memory[0xFF06] = 0x00;
	memory[0xFF07] = 0x00;

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
    if (addr >= 0xE000 && addr < 0xFE00)
        return memory[addr - 0x2000];

    if (addr == 0xFF00)
    {
        uint8_t p1 = memory[0xFF00];
        uint8_t result = p1 & 0x30;
        result |= 0x0F;

        if (!(p1 & 0x10))
            result &= (joypad_bits >> 4);
        else if (!(p1 & 0x20))
            result &= (joypad_bits & 0x0F);

        return result;
    }

    if (addr < 0x8000 || (addr >= 0xA000 && addr < 0xC000))
    {
        if (controller.mbc)
            return controller.read(addr);
        return memory[addr];
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
	if (addr == 0xFF02 && val == 0x81){
		char c = memory[0xFF01];
		std::cout << c;
		memory[0xFF02] = 0;
	}

    if (addr >= 0xE000 && addr < 0xFE00)
        addr -= 0x2000;

    if (addr == 0xFF04)
    {
        timer.internal_counter = 0;
        timer.div_counter = 0;
        return;
    }

	// DMA start (OAM DMA)
	if (addr == 0xFF46)
	{
		uint16 src = val * 0x100;
		std::cerr << "Memory: OAM DMA from 0x" << std::hex << src << " to 0xFE00\n" << std::dec;
		for (int i = 0; i < 0xA0; ++i)
		{
			uint8 b = read8(src + i);
			memory[0xFE00 + i] = b;
		}
		memory[addr] = val;
		return;
	}

	if (addr == 0xFF47)
	{
		std::cerr << "Memory write BGP=0x" << std::hex << (int)val << std::dec << "\n";
		memory[addr] = val;
		if (val == 0x00) memory[addr] = 0xFC;
		return;
	}
	

    if (addr == 0xFF40 || addr == 0xFF47 || addr == 0xFF48 || addr == 0xFF49)
    {
        std::cerr << "Memory write IO[0x" << std::hex << addr 
                  << "] = 0x" << std::setw(2) << std::setfill('0') 
                  << (int)val << std::dec << "\n";
    }

    if ((addr >= 0x8800 && addr < 0x9800) || (addr >= 0x9C00 && addr < 0xA000) 
        || addr == 0x8000 || addr == 0x97F0)
    {
		static int vr_write_log = 0;
		if (vr_write_log < 200)
		{
			std::cerr << "Memory: VRAM write [0x" << std::hex << addr << "] = 0x" 
					  << std::setw(2) << std::setfill('0') << (int)val << std::dec << "\n";
			vr_write_log++;
		}
    }

    if (addr < 0x8000 || (addr >= 0xA000 && addr < 0xC000))
    {
        controller.write(addr, val);
        return;
    }

    if ((addr >= 0xFF10 && addr <= 0xFF3F) || addr == 0xFF26)
    {
        sound.write8(addr, val);
        memory[addr] = val;
        return;
    }

	// Log OAM writes
	if (addr >= 0xFE00 && addr < 0xFEA0)
	{
		static int oam_write_log = 0;
		if (oam_write_log < 200)
		{
			std::cerr << "Memory: OAM write [0x" << std::hex << addr << "] = 0x" << std::setw(2) << std::setfill('0') << (int)val << std::dec << "\n";
			oam_write_log++;
		}
	}

	memory[addr] = val;
}

void Memory::write16(uint16 addr, uint16 val)
{
	write8(addr, val & 0xFF);
	write8(addr + 1, (val >> 8));
}

void Memory::tickTimers(uint32_t cycles)
{
    // DIV always increments (even if timer is stopped)
    timer.div_counter += cycles;
    while (timer.div_counter >= 256)
    {
        timer.div_counter -= 256;
        DIV = (DIV + 1) & 0xFF;
    }

    if (!(TAC & 0x04)) return;  // Timer disabled

    static const uint32_t periods[4] = {1024, 16, 64, 256};
    uint32_t period = periods[TAC & 0x03];

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

	if (romData)
		delete[] romData;

	romData = new uint8[size];
	rom.read((char *)romData, size);
	if (!rom)
	{
		std::cerr << "Failed reading ROM" << std::endl;
		exit(-1);
	}

	uint8 type = romData[0x147];
	uint8 romSizeCode = romData[0x148];
	uint8 ramSizeCode = romData[0x149];

	uint8 headerChecksum = 0;
	for (uint16 i = 0x134; i <= 0x14C; i++)
	{
		headerChecksum = headerChecksum - romData[i] - 1;
	}

	if (headerChecksum != romData[0x14D])
	{
		std::cerr << "Header Checksum incorrect. Expected: " << (int)romData[0x14D] << " Calculated: " << (int)headerChecksum << std::endl;
		// exit(-1) // wont add though
	}

	char name[17] = {0};
	for (int i = 0; i < 16; i++)
		name[i] = std::isprint((unsigned char)romData[0x134 + i]) ? romData[0x134 + i] : ' ';
	std::string tmp(name);
	tmp.erase(std::remove_if(tmp.begin(), tmp.end(), [](unsigned char c)
							 { return std::isspace(c); }),
			  tmp.end());
	SDL_SetWindowTitle(&window, tmp.c_str());

	static const uint32 romSizes[] = {
		32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024,
		512 * 1024, 1024 * 1024, 2 * 1024 * 1024, 4 * 1024 * 1024, 8 * 1024 * 1024};
	uint32 calculatedRomSize = (romSizeCode <= 0x08) ? romSizes[romSizeCode] : size;

	static const uint32 ramSizes[] = {0, 2 * 1024, 8 * 1024, 32 * 1024, 128 * 1024, 64 * 1024};
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

#ifdef DEBUG_SEED_VRAM
	// Debug helper: seed VRAM and BG tilemap with a visible pattern so rendering can be tested
	std::cerr << "Memory: DEBUG_SEED_VRAM active - filling VRAM/tilemap with test pattern\n";
	// Fill tile data area 0x8000-0x8FFF with a repeating pattern
	for (uint32_t a = 0x8000; a <= 0x8FFF; ++a)
	{
		memory[a] = (uint8)((a - 0x8000) & 0xFF);
	}
	// Fill tilemap at 0x9800-0x9BFF with sequential tile indices
	for (uint32_t a = 0x9800; a <= 0x9BFF; ++a)
	{
		memory[a] = (uint8)((a - 0x9800) & 0xFF);
	}
#endif
}

TimerState::TimerState() : internal_counter(0) {}

void TimerState::update(int cycles, uint8 &IF, uint8 &DIV_reg, uint8 &TIMA_reg, uint8 &TMA_reg, uint8 &TAC_reg)
{
	uint16 prev_counter = internal_counter;
	internal_counter += cycles;

	DIV_reg = (internal_counter >> 8) & 0xFF;

	if (TAC_reg & 0x04)
	{
		int bit_to_check = 0;
		switch (TAC_reg & 0x03)
		{
		case 0x00:
			bit_to_check = 9;
			break;
		case 0x01:
			bit_to_check = 3;
			break;
		case 0x02:
			bit_to_check = 5;
			break;
		case 0x03:
			bit_to_check = 7;
			break;
		}

		bool old_bit = (prev_counter >> bit_to_check) & 0x01;
		bool new_bit = (internal_counter >> bit_to_check) & 0x01;

		if (old_bit && !new_bit)
		{
			if (TIMA_reg == 0xFF) // Overflow
			{
				TIMA_reg = TMA_reg;
				IF |= 0x04;
			}
			else
			{
				TIMA_reg++;
			}
		}
	}
}