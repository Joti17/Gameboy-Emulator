#ifndef INPUT_H
#define INPUT_H
#include <cstdint>
#include "mmu.h"
#include <SDL2/SDL.h>
// #include <SDL_gamecontroller.h>
#include <SDL2/SDL_joystick.h>
#include "input.h"
#include "memory.h"
#include <unordered_map>
#include <SDL2/SDL.h>

enum class GBButton { 
    Right, Left, Up, Down, A, B, Select, Start 
};

struct Input{
	Memory& memory;
	std::unordered_map<SDL_Keycode, GBButton> keyMap;

	Input(Memory &mem);

	void HandleKey(const SDL_Event& event);
};



#endif