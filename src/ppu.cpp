#include <cstdint>
#include <array>

#define uint8 uint8_t

std::array<uint8, 64> decode(const std::array<uint8, 16>& tilemap){
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

