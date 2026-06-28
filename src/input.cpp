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
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0)
    {
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
    bool isDown = false;
    int bit = -1;
    if (event.type == SDL_CONTROLLERDEVICEADDED)
    {
        int device_index = event.cdevice.which;
        if (SDL_IsGameController(device_index))
        {
            SDL_GameController *controller = SDL_GameControllerOpen(device_index);
            if (controller)
            {
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

        const char *keyName = SDL_GetKeyName(event.key.keysym.sym);
        std::cout << "[Input][KEY] " << (isDown ? "DOWN" : "UP  ")
                  << " name=" << keyName
                  << " sym=0x" << std::hex << event.key.keysym.sym << std::dec
                  << " scancode=" << event.key.keysym.scancode
                  << " repeat=" << (int)event.key.repeat
                  << "\n";

        auto it = keyMap.find(event.key.keysym.sym);
        if (it != keyMap.end())
        {
            bit = GetButtonBit(it->second);
            std::cout << "[Input][KEY] matched -> bit " << bit << "\n";
        }
        else
        {
            std::cout << "[Input][KEY] no mapping for this key\n";
        }
    }
    else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP)
    {
        isDown = (event.type == SDL_CONTROLLERBUTTONDOWN);

        std::cout << "[Input][PAD] " << (isDown ? "DOWN" : "UP  ")
                  << " button=" << (int)event.cbutton.button
                  << " which=" << event.cbutton.which
                  << "\n";

        switch (event.cbutton.button)
        {
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: bit = 0; break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  bit = 1; break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    bit = 2; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  bit = 3; break;
        
        case SDL_CONTROLLER_BUTTON_A:          bit = 4; break; // GB A
        case SDL_CONTROLLER_BUTTON_B:          bit = 5; break; // GB B
        case SDL_CONTROLLER_BUTTON_X:          bit = 4; break; // mirrors A
        case SDL_CONTROLLER_BUTTON_Y:          bit = 5; break; // mirrors B
        
        case SDL_CONTROLLER_BUTTON_BACK:       bit = 6; break;
        case SDL_CONTROLLER_BUTTON_START:      bit = 7; break;
        default:                               bit = -1; break;
        }

        std::cout << "[Input][PAD] -> bit " << bit << "\n";
    }

    if (bit != -1)
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
        keyMap[SDLK_w] = GBButton::Up;
        keyMap[SDLK_a] = GBButton::Left;
        keyMap[SDLK_s] = GBButton::Down;
        keyMap[SDLK_d] = GBButton::Right;

        keyMap[SDLK_j] = GBButton::A;
        keyMap[SDLK_k] = GBButton::B;

        // MENUS
        keyMap[SDLK_q] = GBButton::Select;
        keyMap[SDLK_e] = GBButton::Start;

        std::cout << "[Input] Switched to FIXED WASD layout (WASD + JK)\n";
    }
    else
    {
        keyMap[SDLK_RIGHT] = GBButton::Right;
        keyMap[SDLK_LEFT] = GBButton::Left;
        keyMap[SDLK_UP] = GBButton::Up;
        keyMap[SDLK_DOWN] = GBButton::Down;
        keyMap[SDLK_z] = GBButton::A;
        keyMap[SDLK_x] = GBButton::B;
        keyMap[SDLK_BACKSPACE] = GBButton::Select;
        keyMap[SDLK_RETURN] = GBButton::Start;
        std::cout << "[Input] Switched to DEFAULT layout (Arrows + Z/X/Enter)\n";
    }
}

uint8_t Input::GetButtonBit(GBButton button)
{
    switch (button)
    {
    case GBButton::Right:
        return 0;
    case GBButton::Left:
        return 1;
    case GBButton::Up:
        return 2;
    case GBButton::Down:
        return 3;
    case GBButton::A:
        return 4;
    case GBButton::B:
        return 5;
    case GBButton::Select:
        return 6;
    case GBButton::Start:
        return 7;
    }
    return 0xFF;
}