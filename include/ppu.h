#ifndef PPU_H
#define PPU_H

#include <array>
#include <cstdint>
#include <SDL2/SDL.h>

#define uint8 uint8_t

enum class PPUMode { OAMScan = 2, PixelTransfer = 3, HBlank = 0, VBlank = 1 };

struct PPU{
    SDL_Color palette[4];

    PPUMode currentMode;
    int dots;

    uint8 &LCDC;            // FF40
    uint8 &STAT;            // FF41
    uint8 &SCY, &SCX;       // FF42, FF43 background scroll
    uint8 &LY;              // FF44 current scanline
    uint8 &LYC;             // FF45 scanline compare
    uint8 &BGP;             // FF47 background palette map
    uint8 &OBP0, &OBP1;     // FF48, FF49 sprite palettes
    uint8 &WY, &WX;         // FF4A FF4B window position

    uint8 *vram;
    uint16 vramSize = 0x2000;

    uint8 *oam;
    uint16 oamSize = 0xA0;

    uint8_t bgColorBuffer[160] = {};
    int windowLineCounter = 0;

    Memory &mem;
    PPU(Memory &mem);  // constructor for color palette

    std::array<uint8, 160*144> screenPixels;

    std::array<uint8, 64> decode(const std::array<uint8, 16>& tilemap);

    void setPixel(uint8 x, uint8 y, uint8 color);

    uint8 getPixel(uint8 x, uint8 y);

    void tick(uint32 cycles);
    void renderScanline();
    uint8_t applyPalette(uint8_t colorIndex, uint8_t paletteReg);

    void drawToScreen(SDL_Renderer* renderer, SDL_Texture *texture);

    void requestVBlankInterrupt();
    void requestLCDSTATInterrupt();
    void updateSTAT(PPUMode oldMode);
    void renderSprites(int height);
    std::array<uint8, 8> decodeTile(uint16 addr, int row);
};

#endif