#include <cstdint>
#include "mmu.h"
#include <SDL2/SDL.h>
// #include <SDL_gamecontroller.h>
#include <SDL_joystick.h>
#include "input.h"
#include "memory.h"

#define uint8 uint8_t
#define int8 int8_t
#define uint16 uint16_t
#define int16 int16_t

Input::Input(Memory& mem) : memory(mem) {}

void Input::HandleKey(bool down){
	// down means wether its a key down(true) or a key up event(false)
	uint8 result = 0xF; // all unpressed
	if (!P1 & (1 << ))
}
