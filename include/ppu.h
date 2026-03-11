#ifndef PPU_H
#define PPU_H

#include <array>
#include <cstdint>
#include <SDL2/SDL.h>

#define uint8 uint8_t

std::array<uint8, 64> decode(const std::array<uint8, 16>& tilemap);

void setPixel(uint8 x, uint8 y, uint8 color);

uint8 getPixel(uint8 x, uint8 y);

#endif