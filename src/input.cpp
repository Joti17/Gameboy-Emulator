#include <cstdint>
#include "mmu.h"
#include <SDL2/SDL.h>
// #include <SDL_gamecontroller.h>
#include <SDL2/SDL_joystick.h>
#include "input.h"
#include "memory.h"
#include <iostream>

#define uint8 uint8_t
#define int8 int8_t
#define uint16 uint16_t
#define int16 int16_t

Input::Input(Memory &mem) : memory(mem)
{
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "SDL GameController init failed: " << SDL_GetError() << "\n";
    }

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
    uint8 bit = 0xFF;
    bool isDown = false;

    if (event.type == SDL_CONTROLLERDEVICEADDED)
    {
        int device_index = event.cdevice.which;
        if (SDL_IsGameController(device_index))
        {
            SDL_GameController* controller = SDL_GameControllerOpen(device_index);
            if (controller) {
                std::cout << "[Input] Controller connected: " << SDL_GameControllerName(controller) << "\n";
            }
        }
    }
    else if (event.type == SDL_CONTROLLERDEVICEREMOVED)
    {
        std::cout << "[Input] Controller disconnected.\n";
    }

    else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
    {
        isDown = (event.type == SDL_KEYDOWN);
        auto it = keyMap.find(event.key.keysym.sym);
        if (it != keyMap.end())
        {
            GBButton button = it->second;
            bit = GetButtonBit(button);
        }
    }
    else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP)
    {
        isDown = (event.type == SDL_CONTROLLERBUTTONDOWN);
        
        switch (event.cbutton.button)
        {
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: bit = 0; break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  bit = 1; break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:    bit = 2; break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  bit = 3; break;
            case SDL_CONTROLLER_BUTTON_A:          bit = 4; break; // Physical A
            case SDL_CONTROLLER_BUTTON_B:          bit = 5; break; // Physical B
            case SDL_CONTROLLER_BUTTON_BACK:       bit = 6; break; // Minus (-)
            case SDL_CONTROLLER_BUTTON_START:      bit = 7; break; // Plus (+)
        }
    }

    if (bit != 0xFF)
    {
        if (isDown)
        {
            memory.joypad_bits &= ~(1 << bit);
            memory.IF |= 0x10;
        }
        else
        {
            memory.joypad_bits |= (1 << bit);
        }
    }
}


void Input::switchLayout()
{
    alternativeLayout = !alternativeLayout;
    
    keyMap.clear();
    
    if (alternativeLayout)
    {
        keyMap[SDLK_w]         = GBButton::Up;
        keyMap[SDLK_a]         = GBButton::Left;
        keyMap[SDLK_s]         = GBButton::Down;
        keyMap[SDLK_d]         = GBButton::Right;
        
        keyMap[SDLK_j]         = GBButton::A;
        keyMap[SDLK_k]         = GBButton::B;
        
        // MENUS
        keyMap[SDLK_q]         = GBButton::Select;
        keyMap[SDLK_e]         = GBButton::Start;
        
        std::cout << "[Input] Switched to FIXED WASD layout (WASD + JK)\n";
    }
    else
    {
        keyMap[SDLK_RIGHT]     = GBButton::Right;
        keyMap[SDLK_LEFT]      = GBButton::Left;
        keyMap[SDLK_UP]        = GBButton::Up;
        keyMap[SDLK_DOWN]      = GBButton::Down;
        keyMap[SDLK_z]         = GBButton::A;
        keyMap[SDLK_x]         = GBButton::B;
        keyMap[SDLK_BACKSPACE] = GBButton::Select;
        keyMap[SDLK_RETURN]    = GBButton::Start;
        std::cout << "[Input] Switched to DEFAULT layout (Arrows + Z/X/Enter)\n";
    }
}

uint8_t Input::GetButtonBit(GBButton button) {
    switch (button) {
        case GBButton::Right:  return 0;
        case GBButton::Left:   return 1;
        case GBButton::Up:     return 2;
        case GBButton::Down:   return 3;
        case GBButton::A:      return 4;
        case GBButton::B:      return 5;
        case GBButton::Select: return 6;
        case GBButton::Start:  return 7;
    }
    return 0xFF;
}