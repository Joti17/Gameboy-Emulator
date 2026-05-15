#include <cstdint>
#include "mmu.h"
#include <SDL2/SDL.h>
// #include <SDL_gamecontroller.h>
#include <SDL2/SDL_joystick.h>
#include "input.h"
#include "memory.h"

#define uint8 uint8_t
#define int8 int8_t
#define uint16 uint16_t
#define int16 int16_t

Input::Input(Memory &mem) : memory(mem)
{
	// TODO: Add loading from settings
	keyMap[SDLK_RIGHT] = GBButton::Right;
	keyMap[SDLK_LEFT] = GBButton::Left;
	keyMap[SDLK_UP] = GBButton::Up;
	keyMap[SDLK_DOWN] = GBButton::Down;
	keyMap[SDLK_z] = GBButton::A;
	keyMap[SDLK_x] = GBButton::B;
	keyMap[SDLK_BACKSPACE] = GBButton::Select;
	keyMap[SDLK_RETURN] = GBButton::Start;
}

void Input::HandleKey(const SDL_Event &event)
{
	if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP)
		return;

	auto it = keyMap.find(event.key.keysym.sym);
	if (it == keyMap.end())
		return;

	GBButton button = it->second;
	uint8 bit = 0xFF;
	switch (button)
	{
	case GBButton::Right:
		bit = 0;
		break;
	case GBButton::Left:
		bit = 1;
		break;
	case GBButton::Up:
		bit = 2;
		break;
	case GBButton::Down:
		bit = 3;
		break;
	case GBButton::A:
		bit = 4;
		break;
	case GBButton::B:
		bit = 5;
		break;
	case GBButton::Select:
		bit = 6;
		break;
	case GBButton::Start:
		bit = 7;
		break;
	}

	if (bit != 0xFF)
	{
		if (event.type == SDL_KEYDOWN)
		{
			memory.joypad_bits &= ~(1 << bit);
			memory.TAC |= (1 << 4);
		}
		else
		{
			memory.joypad_bits |= (1 << bit);
		}
	}
}
