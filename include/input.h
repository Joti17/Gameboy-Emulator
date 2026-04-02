#ifndef INPUT_H
#define INPUT_H
#include <cstdint>
#include "mmu.h"
#include <SDL2/SDL.h>
// #include <SDL_gamecontroller.h>
#include <SDL_joystick.h>
#include "input.h"
#include "memory.h"

struct Input{
	Memory& memory;

	Input(Memory &mem);

	void HandleKey(bool down);
};

#endif