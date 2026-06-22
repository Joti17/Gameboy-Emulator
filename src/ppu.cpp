#include <cstdint>
#include <array>
#include <SDL2/SDL.h>
#include "memory.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "logger.h"

#define uint8 uint8_t

PPU::PPU(Memory &mem) : currentMode(PPUMode::OAMScan),
                        dots(0),
                        mem(mem),
                        LCDC(mem.memory[0xFF40]),
                        STAT(mem.memory[0xFF41]),
                        SCY(mem.memory[0xFF42]),
                        SCX(mem.memory[0xFF43]),
                        LY(mem.memory[0xFF44]),
                        LYC(mem.memory[0xFF45]),
                        BGP(mem.memory[0xFF47]),
                        OBP0(mem.memory[0xFF48]),
                        OBP1(mem.memory[0xFF49]),
                        WY(mem.memory[0xFF4A]),
                        WX(mem.memory[0xFF4B])
{
    STAT = 0x80;
    dots = 0;
    windowLineCounter = 0;

    vram = &mem.memory[0x8000];
    oam = &mem.memory[0xFE00];
    screenPixels.fill(0);

    palette[0] = {155, 188, 15, 255};
    palette[1] = {139, 172, 15, 255};
    palette[2] = {48, 98, 48, 255};
    palette[3] = {15, 56, 15, 255};
    // BGP = 0xFC;
    OBP0 = 0xFF;
    OBP1 = 0xFF;
    // std::cerr << "PPU: Forced BGP=0xFC for visibility\n";
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
    if (x >= 160 || y >= 144)
        return; // compiler error on arch otherwise
    screenPixels[y * 160 + x] = color;
    if (y < 4 && x < 16)
    {
        g_logger.log("PPU: setPixel LY={} x={} colorIndex={}", y, x, color);
    }
}

uint8 PPU::getPixel(uint8 x, uint8 y)
{
    return screenPixels[y * 160 + x];
}

void PPU::tick(uint32 cycles)
{
    if (!(LCDC & 0x80))
    {
        LY = 0;
        dots = 0;
        currentMode = PPUMode::HBlank;
        STAT = (STAT & 0xFC);
        return;
    }

    dots += cycles;

    PPUMode oldMode = currentMode;
    uint8 oldLY = LY;

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
            if (LY >= 154)
            {
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
    if (colorIndex > 3)
        colorIndex = 0;
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

void PPU::renderScanline()
{
    if (!(LCDC & 0x80))
        return;
    uint8_t LCDC_reg = mem.read8(0xFF40);
    uint8_t SCX_reg = mem.read8(0xFF43);
    uint8_t SCY_reg = mem.read8(0xFF42);
    uint8_t WX_reg = mem.read8(0xFF4B);
    uint8_t WY_reg = mem.read8(0xFF4A);
    uint8_t BGP_reg = mem.read8(0xFF47);

    bool bgEnabled = LCDC_reg & 0x01;
    bool winEnabled = (LCDC_reg & 0x20) && (WY_reg <= LY);
    bool spritesEnabled = LCDC_reg & 0x02;
    bool tallSprites = LCDC_reg & 0x04;

    uint16_t bgMap = (LCDC_reg & 0x08) ? 0x9C00 : 0x9800;
    uint16_t winMap = (LCDC_reg & 0x40) ? 0x9C00 : 0x9800;

    int winX = WX_reg - 7;
    bool windowDrawnOnLine = false;

    for (int x = 0; x < 160; ++x)
    {
        uint8 colorIdx = 0;

        if (winEnabled && x >= winX && winX >= 0)
        {
            windowDrawnOnLine = true;
            int wx = x - winX;
            int wy = windowLineCounter;
            uint8_t tileX = wx / 8;
            uint8_t tileY = (wy / 8) & 31;
            uint16_t mapAddr = winMap + tileY * 32 + (tileX & 31);
            uint8_t tileId = mem.read8(mapAddr);

            int row = wy % 8;
            uint16_t tileAddr;
            if (LCDC_reg & 0x10)
            {
                tileAddr = 0x8000 + (static_cast<uint16_t>(tileId) * 16);
            }
            else
            {
                int8_t signedId = static_cast<int8_t>(tileId);
                tileAddr = 0x9000 + (static_cast<int16_t>(signedId) * 16);
            }
            auto tile = decodeTile(tileAddr, row);
            colorIdx = tile[wx % 8];
        }
        else if (bgEnabled)
        {
            uint8_t currentPixelX = static_cast<uint8_t>((SCX_reg + x) & 0xFF);
            uint8_t currentPixelY = static_cast<uint8_t>((SCY_reg + LY) & 0xFF);

            uint8_t tileX = currentPixelX / 8;
            uint8_t tileY = currentPixelY / 8;
            uint16_t mapAddr = bgMap + tileY * 32 + tileX;
            uint8_t tileId = mem.read8(mapAddr);

            int row = currentPixelY % 8;
            uint16_t tileAddr;
            if (LCDC_reg & 0x10)
            {
                tileAddr = 0x8000 + (static_cast<uint16_t>(tileId) * 16);
            }
            else
            {
                int8_t signedId = static_cast<int8_t>(tileId);
                tileAddr = 0x9000 + (static_cast<int16_t>(signedId) * 16);
            }
            auto tile = decodeTile(tileAddr, row);
            colorIdx = tile[currentPixelX % 8];
        }

        bgColorBuffer[x] = colorIdx;
        setPixel(x, LY, applyPalette(colorIdx, BGP_reg));
    }

    if (spritesEnabled)
    {
        renderSprites(tallSprites ? 16 : 8);
    }

    if (winEnabled && windowDrawnOnLine)
        windowLineCounter++;
}

void PPU::requestVBlankInterrupt()
{
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

    static bool infoLogged = false;
    if (!infoLogged)
    {
        g_logger.log("PPU draw start: frame={} LCDC=0x{:02X} BGP=0x{:02X} SCX=0x{:02X} SCY=0x{:02X}", frame, LCDC, BGP, SCX, SCY);
        infoLogged = true;
    }

    auto packRGBA = [](const SDL_Color &color) -> uint32_t
    {
        return (static_cast<uint32_t>(color.r) << 24) |
               (static_cast<uint32_t>(color.g) << 16) |
               (static_cast<uint32_t>(color.b) << 8) |
               (static_cast<uint32_t>(color.a) << 0);
    };

    std::vector<uint32_t> pixels(160 * 144);
    int nonZero = 0;

    for (int y = 0; y < 144; y++)
    {
        for (int x = 0; x < 160; x++)
        {
            uint8_t colorIndex = getPixel(x, y);
            SDL_Color color = palette[colorIndex];
            pixels[y * 160 + x] = packRGBA(color);

            if (colorIndex != 0)
                nonZero++;
        }
    }

    {
        std::string firstRow;
        for (int x = 0; x < 32; ++x)
        {
            char buf[8];
            std::snprintf(buf, sizeof(buf), " %01X", (int)getPixel(x, 0));
            firstRow += buf;
        }
        g_logger.log("PPU first-row indices:{}", firstRow);
    }

    SDL_UpdateTexture(texture, nullptr, pixels.data(), 160 * sizeof(uint32_t));
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);

    g_logger.log("PPU: frame {} nonzero pixels = {}", frame, nonZero);

    // Dump the first useful frame to a PPM file for offline inspection
    static bool dumped = false;
    if (!dumped && nonZero > 0)
    {
        const char *path = "/tmp/ppu_frame.ppm";
        std::ofstream f(path, std::ios::binary);
        if (f)
        {
            f << "P6\n160 144\n255\n";
            for (int y = 0; y < 144; ++y)
            {
                for (int x = 0; x < 160; ++x)
                {
                    uint8_t idx = getPixel(x, y);
                    SDL_Color c = palette[idx];
                    f.put(static_cast<char>(c.r));
                    f.put(static_cast<char>(c.g));
                    f.put(static_cast<char>(c.b));
                }
            }
            f.close();
            g_logger.log("PPU: dumped framebuffer to {}", path);
            dumped = true;
        }
        else
        {
            g_logger.log("PPU: failed to open framebuffer dump file {}");
        }
    }

    if (nonZero == 0)
    {
        g_logger.log("PPU: framebuffer empty, performing fallback full-frame background render");
        std::string sample_vram;
        for (uint32_t i = 0; i < 16; ++i)
        {
            char buf[8];
            std::snprintf(buf, sizeof(buf), " %02X", mem.read8(0x8000 + i));
            sample_vram += buf;
        }
        g_logger.log("PPU: VRAM[0x8000..0x800F]:{}", sample_vram);
        std::string sample_map;
        for (uint32_t i = 0; i < 16; ++i)
        {
            char buf[8];
            std::snprintf(buf, sizeof(buf), " %02X", mem.read8(0x9800 + i));
            sample_map += buf;
        }
        g_logger.log("PPU: BGMap[0x9800..0x980F]:{}", sample_map);
        bool bgEnabled = LCDC & 0x01;
        uint16 bgMap = (LCDC & 0x08) ? 0x9C00 : 0x9800;
        uint16 tileBase = (LCDC & 0x10) ? 0x8000 : 0x8800;
        for (int y = 0; y < 144; ++y)
        {
            for (int x = 0; x < 160; ++x)
            {
                uint8 colorIdx = 0;
                if (bgEnabled)
                {
                    uint8 scrollX = x + SCX;
                    uint8 scrollY = y + SCY;
                    uint8 tileX = scrollX / 8;
                    uint8 tileY = scrollY / 8;
                    uint16 mapAddr = bgMap + tileY * 32 + tileX;
                    uint8 tileId = mem.read8(mapAddr);

                    uint16_t tileAddr;
                    if (LCDC & 0x10)
                    {
                        tileAddr = 0x8000 + (tileId * 16);
                    }
                    else
                    {
                        int8_t signedId = static_cast<int8_t>(tileId);
                        tileAddr = 0x9000 + (signedId * 16);
                    }
                    auto tile = decodeTile(tileAddr, scrollY % 8);
                    colorIdx = tile[scrollX % 8];
                }
                screenPixels[y * 160 + x] = applyPalette(colorIdx, BGP);
            }
        }

        nonZero = 0;
        auto packRGBA = [](const SDL_Color &color) -> uint32_t
        {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            return (static_cast<uint32_t>(color.r) << 24) |
                   (static_cast<uint32_t>(color.g) << 16) |
                   (static_cast<uint32_t>(color.b) << 8) |
                   static_cast<uint32_t>(color.a);
#else
            return (static_cast<uint32_t>(color.a) << 24) |
                   (static_cast<uint32_t>(color.b) << 16) |
                   (static_cast<uint32_t>(color.g) << 8) |
                   static_cast<uint32_t>(color.r);
#endif
        };

        for (int y = 0; y < 144; y++)
        {
            for (int x = 0; x < 160; x++)
            {
                uint8_t colorIndex = getPixel(x, y);
                SDL_Color color = palette[colorIndex];
                pixels[y * 160 + x] = packRGBA(color);
                if (colorIndex != 0)
                    nonZero++;
            }
        }

        g_logger.log("PPU: fallback render produced nonzero pixels = {}", nonZero);
        if (!dumped && nonZero > 0)
        {
            const char *path = "/tmp/ppu_frame_fallback.ppm";
            std::ofstream f(path, std::ios::binary);
            if (f)
            {
                f << "P6\n160 144\n255\n";
                for (int y = 0; y < 144; ++y)
                    for (int x = 0; x < 160; ++x)
                    {
                        uint8_t idx = getPixel(x, y);
                        SDL_Color c = palette[idx];
                        f.put(static_cast<char>(c.r));
                        f.put(static_cast<char>(c.g));
                        f.put(static_cast<char>(c.b));
                    }
                f.close();
                g_logger.log("PPU: dumped fallback framebuffer to {}", path);
                dumped = true;
            }
        }
    }
}

std::array<uint8, 8> PPU::decodeTile(uint16 addr, int row)
{
    std::array<uint8, 8> pixels{};
    uint16 base = addr + (row * 2);
    uint8 lo = mem.read8(base);
    uint8 hi = mem.read8(base + 1);

    for (int i = 0; i < 8; ++i)
    {
        int bit = 7 - i;
        pixels[i] = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
    }
    return pixels;
}

void PPU::renderSprites(int height)
{
    int drawn = 0;
    for (int i = 39; i >= 0; --i)
    {
        if (drawn >= 10)
            break;

        uint16 base = 0xFE00 + i * 4;
        int16_t sprY = mem.read8(base) - 16;
        int16_t sprX = mem.read8(base + 1) - 8;
        uint8 tileIdx = mem.read8(base + 2);
        uint8 attrs = mem.read8(base + 3);

        if (LY < sprY || LY >= sprY + height)
            continue;
        drawn++;

        bool flipX = attrs & 0x20;
        bool flipY = attrs & 0x40;
        bool bgPriority = attrs & 0x80;
        uint8 pal = (attrs & 0x10) ? OBP1 : OBP0;

        int spriteRow = LY - sprY;
        if (flipY)
            spriteRow = height - 1 - spriteRow;

        if (height == 16)
            tileIdx &= 0xFE;
        uint16 tileAddr = 0x8000 + (tileIdx * 16) + spriteRow * 2;

        uint8 lo = mem.read8(tileAddr);
        uint8 hi = mem.read8(tileAddr + 1);

        for (int px = 0; px < 8; ++px)
        {
            int screenX = sprX + px;
            if (screenX < 0 || screenX >= 160)
                continue;

            int bit = flipX ? px : 7 - px;
            uint8 color = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
            if (color == 0)
                continue;

            if (bgPriority && bgColorBuffer[screenX] != 0)
                continue;

            setPixel(screenX, LY, applyPalette(color, pal));
        }
    }
}