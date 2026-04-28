#include <cstdint>
#include <array>
#include <SDL2/SDL.h>
#include "memory.h"

#define uint8 uint8_t
#define vmem mem.memory

PPU::PPU(Memory &mem) : LCDC(vmem[0xFF40]),
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
    vram = &vmem[0x8000];
    oam = &vmem[0xFE00];

    palette[0] = {155, 188, 15, 255};
    palette[1] = {139, 172, 15, 255};
    palette[2] = {48, 98, 48, 255};
    palette[3] = {15, 56, 15, 255};
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

void PPU::tick(uint32 cycles)
{
    dots += cycles;

    switch (currentMode)
    {
    case PPUMode::OAMScan:
        if (dots >= 80)
        {
            dots -= 80;
            currentMode = PPUMode::PixelTransfer;
        }
        break;
    case PPUMode::PixelTransfer:
        if (dots >= 172)
        {
            dots -= 172;
            renderScanline();
            currentMode = PPUMode::HBlank;
        }
        break;
    case PPUMode::HBlank:
        if (dots >= 204)
        {
            dots -= 204;
            LY++;
            if (LY == 144)
            {
                currentMode = PPUMode::VBlank;
                requestVBlankInterrupt();
            }
            else
            {
                currentMode = PPUMode::OAMScan;
            }
        }
        break;

    case PPUMode::VBlank:
        if (dots >= 456)
        {
            dots -= 456;
            LY++;
            if (LY == 154)
            {
                LY = 0;
                windowLineCounter = 0;
                currentMode = PPUMode::OAMScan;
            }
        }
        break;
    }
    STAT = (STAT & 0xFC) | (uint8_t)currentMode;
}

uint8_t PPU::applyPalette(uint8_t colorIndex, uint8_t paletteReg)
{
    return (paletteReg >> (colorIndex * 2)) & 0x03;
}

void PPU::renderScanline()
{
    if (!(LCDC & 0x80))
        return;

    bool bgEnabled = LCDC & 0x01;
    bool winEnabled = (LCDC & 0x20) && WY <= LY;
    bool sprEnabled = LCDC & 0x02;

    uint16_t bgTileMap = (LCDC & 0x08) ? 0x9C00 : 0x9800;
    uint16_t winTileMap = (LCDC & 0x40) ? 0x9C00 : 0x9800;

    // tile data addressing mode
    bool signedAddressing = !(LCDC & 0x10);
    uint16_t tileDataBase = signedAddressing ? 0x8800 : 0x8000;

    uint8_t windowX = WX - 7;
    bool windowStarted = false;

    for (int x = 0; x < 160; x++)
    {

        uint8_t pixelColor = 0;

        if (winEnabled && x >= windowX)
        {
            windowStarted = true;
            int winPixelX = x - windowX;
            int winPixelY = windowLineCounter;

            uint8_t tileCol = winPixelX / 8;
            uint8_t tileRow = winPixelY / 8;

            uint16_t tileMapAddr = winTileMap + tileRow * 32 + tileCol;
            uint8_t tileIndex = mem.read8(tileMapAddr);

            uint16_t tileAddr;
            if (signedAddressing)
            {
                tileAddr = tileDataBase + ((int8_t)tileIndex + 128) * 16;
            }
            else
            {
                tileAddr = tileDataBase + tileIndex * 16;
            }

            std::array<uint8_t, 16> tileData;
            for (int i = 0; i < 16; i++)
                tileData[i] = mem.read8(tileAddr + i);

            auto pixels = decode(tileData);
            uint8_t raw = pixels[(winPixelY % 8) * 8 + (winPixelX % 8)];
            pixelColor = applyPalette(raw, BGP);
        }
        else if (bgEnabled)
        {
            uint8_t scrolledX = x + SCX;
            uint8_t scrolledY = LY + SCY;

            uint8_t tileCol = scrolledX / 8;
            uint8_t tileRow = scrolledY / 8;

            uint16_t tileMapAddr = bgTileMap + tileRow * 32 + tileCol;
            uint8_t tileIndex = mem.read8(tileMapAddr);

            uint16_t tileAddr;
            if (signedAddressing)
            {
                tileAddr = tileDataBase + ((int8_t)tileIndex + 128) * 16;
            }
            else
            {
                tileAddr = tileDataBase + tileIndex * 16;
            }

            std::array<uint8_t, 16> tileData;
            for (int i = 0; i < 16; i++)
                tileData[i] = mem.read8(tileAddr + i);

            auto pixels = decode(tileData);
            uint8_t raw = pixels[(scrolledY % 8) * 8 + (scrolledX % 8)];
            pixelColor = applyPalette(raw, BGP);
        }

        bgColorBuffer[x] = pixelColor;
        setPixel(x, LY, pixelColor);
    }

    if (sprEnabled)
    {
        bool tall = LCDC & 0x04;
        int spriteHeight = tall ? 16 : 8;

        int spritesDrawn = 0;
        for (int i = 0; i < 40 && spritesDrawn < 10; i++)
        {
            uint8_t sprY = mem.read8(0xFE00 + i * 4 + 0) - 16;
            uint8_t sprX = mem.read8(0xFE00 + i * 4 + 1) - 8;
            uint8_t tileIdx = mem.read8(0xFE00 + i * 4 + 2);
            uint8_t attrs = mem.read8(0xFE00 + i * 4 + 3);

            if (LY < sprY || LY >= sprY + spriteHeight)
                continue;
            spritesDrawn++;

            bool flipX = attrs & 0x20;
            bool flipY = attrs & 0x40;
            bool priority = attrs & 0x80;
            uint8_t pal = (attrs & 0x10) ? OBP1 : OBP0;

            if (tall)
                tileIdx &= 0xFE;

            int row = LY - sprY;
            if (flipY)
                row = (spriteHeight - 1) - row;

            uint16_t tileAddr = 0x8000 + tileIdx * 16 + row * 2;
            uint8_t lo = mem.read8(tileAddr);
            uint8_t hi = mem.read8(tileAddr + 1);

            for (int px = 0; px < 8; px++)
            {
                int screenX = sprX + px;
                if (screenX < 0 || screenX >= 160)
                    continue;

                int bit = flipX ? px : (7 - px);
                uint8_t colorIdx = ((hi >> bit & 1) << 1) | (lo >> bit & 1);

                if (colorIdx == 0)
                    continue;

                if (priority && bgColorBuffer[screenX] != 0)
                    continue;

                setPixel(screenX, LY, applyPalette(colorIdx, pal));
            }
        }
    }

    if (windowStarted)
        windowLineCounter++;
}


void PPU::requestVBlankInterrupt() {
    mem.IF |= 0x01;
}