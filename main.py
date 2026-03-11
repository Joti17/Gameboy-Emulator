"""
Will make in Cpp
import pygame

# Game Boy(DMG)

# Master clock
master_clock = 4194304  # 4.194304 MHz

# Full 64 KiB memory map
memory = bytearray(0x10000)  # 64 KiB CPU address space

# Cartridge ROM (32 KiB typical, can be banked)
rom = memory[0x0000:0x7FFF]

# VRAM (8 KiB, tiles + background/window maps)
vram = memory[0x8000:0x9FFF]

# External Cartridge RAM (8 KiB)
cartridge_ram = memory[0xA000:0xBFFF]

# Work RAM (8 KiB)
wram = memory[0xC000:0xDFFF]

# Echo RAM (mirrors Work RAM, optional handling)
echo_ram = memory[0xE000:0xFDFF]

# Object Attribute Memory (OAM, 40 sprites × 4 bytes)
oam = memory[0xFE00:0xFE9F]

# Unusable memory region (FEA0–FEFF)
unusable = memory[0xFEA0:0xFEFF]

# IO Registers (timers, PPU, sound, joypad)
io_registers = memory[0xFF00:0xFF7F]

# High RAM (fast internal memory)
hram = memory[0xFF80:0xFFFE]

# Interrupt Enable Register
interrupt_enable = memory[0xFFFF]

# Framebuffer for rendered pixels (final display, 160x144)
framebuffer = bytearray(160 * 144)  # Each entry = color index 0–3

# DMG has 4 shades of gray, represented as RGB tuples
COLORS = [(0, 0, 0), (85, 85, 85), (170, 170, 170), (255, 255, 255)]
"""