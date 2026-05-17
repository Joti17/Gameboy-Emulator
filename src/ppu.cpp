#include <cstdint>
#include <array>
#include <SDL2/SDL.h>
#include "memory.h"
#include <iostream>
#include <vector>

#define uint8 uint8_t
#define vmem mem.memory

PPU::PPU(Memory &mem) : currentMode(PPUMode::OAMScan),
                        dots(0),
                        LCDC(vmem[0xFF40]),
                        STAT(vmem[0xFF41]),
                        SCY(vmem[0xFF42]),
                        SCX(vmem[0xFF43]),
                        LY(vmem[0xFF44]),
                        LYC(vmem[0xFF45]),
                        BGP(vmem[0xFF47]),
                        OBP0(vmem[0xFF48]),
                        OBP1(vmem[0xFF49]),
                        WY(vmem[0xFF4A]),
                        WX(vmem[0xFF4B]),
                        mem(mem)
{
    LY = 0;
    STAT = 0x80;
    dots = 0;
    windowLineCounter = 0;

    vram = &vmem[0x8000];
    oam = &vmem[0xFE00];
    screenPixels.fill(0);

    palette[0] = {155, 188, 15, 255};
    palette[1] = {139, 172, 15, 255};
    palette[2] = {48, 98, 48, 255};
    palette[3] = {15, 56, 15, 255};
    BGP = 0xFC;
    OBP0 = 0xFF;
    OBP1 = 0xFF;
    std::cerr << "PPU: Forced BGP=0xFC for visibility\n";
}

std::array<uint8, 64> PPU::decode(const std::array<uint8, 16> &tilemap)
{
    std::array<uint8, 64> colors{};

    for (int i = 0; i < 8; i++)
    {
        uint8 low_bits = tilemap[i * 2];
        uint8 high_bits = tilemap[i * 2 + 1];
        for (int j = 0; j < 8; j++)
        {
            uint8 bit = 7 - j;
            uint8 color = ((high_bits >> (bit) & 1) << 1) | ((low_bits >> bit) & 1);
            colors[i * 8 + j] = color;
        }
    }
    return colors;
}

void PPU::setPixel(uint8 x, uint8 y, uint8 color)
{
    screenPixels[y * 160 + x] = color;
}

uint8 PPU::getPixel(uint8 x, uint8 y)
{
    return screenPixels[y * 160 + x];
}

void PPU::tick(uint32 cycles) {
    if (!(LCDC & 0x80)) {           // LCD off
        LY = 0;
        dots = 0;
        currentMode = PPUMode::HBlank;
        STAT = (STAT & 0xFC);
        return;
    }

    dots += cycles;

    PPUMode oldMode = currentMode;
    uint8 oldLY = LY;

    switch (currentMode) {
    case PPUMode::OAMScan:
        if (dots >= 80) {
            dots -= 80;
            currentMode = PPUMode::PixelTransfer;
        }
        break;

    case PPUMode::PixelTransfer:
        if (dots >= 172) {
            dots -= 172;
            renderScanline();
            currentMode = PPUMode::HBlank;
        }
        break;

    case PPUMode::HBlank:
        if (dots >= 204) {
            dots -= 204;
            LY++;
            if (LY == 144) {
                currentMode = PPUMode::VBlank;
                requestVBlankInterrupt();
            } else {
                currentMode = PPUMode::OAMScan;
            }
        }
        break;

    case PPUMode::VBlank:
        if (dots >= 456) {
            dots -= 456;
            LY++;
            if (LY >= 154) {
                LY = 0;
                windowLineCounter = 0;
                currentMode = PPUMode::OAMScan;
            }
        }
        break;
    }

    updateSTAT(oldMode);
}

uint8_t PPU::applyPalette(uint8_t colorIndex, uint8_t paletteReg)
{
    if (colorIndex > 3) colorIndex = 0;
    return (paletteReg >> (colorIndex * 2)) & 0x03;
}

void PPU::requestLCDSTATInterrupt()
{
    mem.IF |= 0x02;
}

void PPU::updateSTAT(PPUMode oldMode)
{
    bool lycMatch = (LY == LYC);

    if (lycMatch)
        STAT |= 0x04;
    else
        STAT &= ~0x04;

    uint8 modeFlags = static_cast<uint8>(currentMode);
    STAT = (STAT & 0xFC) | modeFlags;

    if (lycMatch && (STAT & 0x40))
    {
        requestLCDSTATInterrupt();
    }

    if (currentMode != oldMode)
    {
        bool trigger = false;

        switch (currentMode)
        {
        case PPUMode::HBlank:
            trigger = (STAT & 0x08);
            break;
        case PPUMode::VBlank:
            trigger = (STAT & 0x10);
            break;
        case PPUMode::OAMScan:
            trigger = (STAT & 0x20);
            break;
        default:
            break;
        }

        if (trigger)
        {
            requestLCDSTATInterrupt();
        }
    }
}

void PPU::renderScanline() {
    if (!(LCDC & 0x80)) return;

    bool bgEnabled = LCDC & 0x01;
    bool winEnabled = (LCDC & 0x20) && (WY <= LY);
    bool spritesEnabled = LCDC & 0x02;
    bool tallSprites = LCDC & 0x04;

    uint16 bgMap  = (LCDC & 0x08) ? 0x9C00 : 0x9800;
    uint16 winMap = (LCDC & 0x40) ? 0x9C00 : 0x9800;
    uint16 tileBase = (LCDC & 0x10) ? 0x8000 : 0x8800;

    int winX = WX - 7;

    for (int x = 0; x < 160; ++x) {
        uint8 colorIdx = 0;

        // Window
        if (winEnabled && x >= winX) {
            int wx = x - winX;
            int wy = windowLineCounter;

            uint8 tileX = wx / 8;
            uint8 tileY = wy / 8;
            uint16 mapAddr = winMap + tileY * 32 + tileX;
            uint8 tileId = mem.read8(mapAddr);

            uint16 tileAddr = tileBase + (tileBase == 0x8800 ? 
                ((int8_t)tileId + 128) * 16 : tileId * 16);

            auto tile = decodeTile(tileAddr, wy % 8);
            colorIdx = tile[wx % 8];
        }
        // Background
        else if (bgEnabled) {
            uint8 scrollX = x + SCX;
            uint8 scrollY = LY + SCY;

            uint8 tileX = scrollX / 8;
            uint8 tileY = scrollY / 8;
            uint16 mapAddr = bgMap + tileY * 32 + tileX;
            uint8 tileId = mem.read8(mapAddr);

            uint16 tileAddr = tileBase + (tileBase == 0x8800 ? 
                ((int8_t)tileId + 128) * 16 : tileId * 16);

            auto tile = decodeTile(tileAddr, scrollY % 8);
            colorIdx = tile[scrollX % 8];
        }

        bgColorBuffer[x] = colorIdx;
        setPixel(x, LY, applyPalette(colorIdx, BGP));
    }

    // Sprites
    if (spritesEnabled) {
        renderSprites(tallSprites ? 16 : 8);
    }

    if (winEnabled && LY >= WY) windowLineCounter++;
}

void PPU::requestVBlankInterrupt()
{
    // Use Memory's write path so any side-effects/hooks run
    uint8 prev = mem.read8(0xFF0F);
    mem.write8(0xFF0F, prev | 0x01);
    // std::cerr << "PPU: requestVBlankInterrupt -> IF=0x" << std::hex << (int)mem.read8(0xFF0F) << std::dec << "\n";
}

void PPU::drawToScreen(SDL_Renderer *renderer, SDL_Texture *texture)
{
    static uint32_t frame = 0;
    frame++;

    if (!renderer || !texture)
        return;

    /*std::cerr << "[PPU Frame " << frame << "] LCDC=0x" << std::hex << (int)LCDC
              << " BGP=0x" << (int)BGP << " LY=" << std::dec << (int)LY << "\n";

    if (frame == 3)
    {
        std::cerr << "PPU: Dump VRAM 0x8000..0x801F:\n";
        for (int i = 0; i < 32; i++)
        {
            std::cerr << std::hex << (int)mem.read8(0x8000 + i) << " ";
        }
        std::cerr << std::dec << "\n";
    }*/

    SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
    if (!format)
        return;

    std::vector<uint32_t> pixels(160 * 144);
    int nonZero = 0;

    for (int y = 0; y < 144; y++)
    {
        for (int x = 0; x < 160; x++)
        {
            uint8_t colorIndex = getPixel(x, y);
            SDL_Color color = palette[colorIndex];
            pixels[y * 160 + x] = SDL_MapRGBA(format, color.r, color.g, color.b, color.a);

            if (colorIndex != 0)
                nonZero++;
        }
    }

    SDL_UpdateTexture(texture, nullptr, pixels.data(), 160 * sizeof(uint32_t));
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_FreeFormat(format);

    // std::cerr << "   -> nonzero pixels = " << nonZero << "\n";
}

std::array<uint8, 8> PPU::decodeTile(uint16 addr, int row) {
    std::array<uint8, 8> pixels{};
    uint8 lo = mem.read8(addr + row * 2);
    uint8 hi = mem.read8(addr + row * 2 + 1);

    for (int i = 0; i < 8; ++i) {
        int bit = 7 - i;
        pixels[i] = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
    }
    return pixels;
}

void PPU::renderSprites(int height) {
    int drawn = 0;
    for (int i = 39; i >= 0; --i) {  // reverse for correct priority
        if (drawn >= 10) break;

        uint16 base = 0xFE00 + i * 4;
        int16_t sprY = mem.read8(base) - 16;
        int16_t sprX = mem.read8(base + 1) - 8;
        uint8 tileIdx = mem.read8(base + 2);
        uint8 attrs = mem.read8(base + 3);

        if (LY < sprY || LY >= sprY + height) continue;
        drawn++;

        bool flipX = attrs & 0x20;
        bool flipY = attrs & 0x40;
        bool bgPriority = attrs & 0x80;
        uint8 pal = (attrs & 0x10) ? OBP1 : OBP0;

        int spriteRow = LY - sprY;
        if (flipY) spriteRow = height - 1 - spriteRow;

        uint16 tileAddr = 0x8000 + (tileIdx * 16) + spriteRow * 2;
        if (height == 16) tileIdx &= 0xFE;

        uint8 lo = mem.read8(tileAddr);
        uint8 hi = mem.read8(tileAddr + 1);

        for (int px = 0; px < 8; ++px) {
            int screenX = sprX + px;
            if (screenX < 0 || screenX >= 160) continue;

            int bit = flipX ? px : 7 - px;
            uint8 color = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
            if (color == 0) continue;

            if (bgPriority && bgColorBuffer[screenX] != 0) continue;

            setPixel(screenX, LY, applyPalette(color, pal));
        }
    }
}