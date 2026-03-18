#include <cstdint>
#include <array>
#include <SDL2/SDL.h>
#include "ppu.h"

#define uint8 uint8_t

PPU::PPU(){
    palette[0] = {155, 188, 15, 255};
    palette[1] = {139, 172, 15, 255};
    palette[2] = {48, 98, 48, 255};
    palette[3] = {15, 56, 15, 255};
}

std::array<uint8, 64> PPU::decode(const std::array<uint8, 16>& tilemap){
    std::array<uint8, 64> colors {};

    for (int i = 0; i < 8; i++){
        uint8 low_bits = tilemap[i*2];
        uint8 high_bits = tilemap[i*2 + 1];
        for (int j = 0; j < 8; j++){
            uint8 bit = 7 - j;
            uint8 color = ((high_bits >> (bit) & 1) << 1) | ((low_bits >> bit) & 1);
            colors[i * 8 + j] = color;
        }
    }
    return colors;
}

void PPU::setPixel(uint8 x, uint8 y, uint8 color){
    screenPixels[y*160 + x] = color;
}

uint8 PPU::getPixel(uint8 x, uint8 y){
    return screenPixels[y * 160 + x];
}
