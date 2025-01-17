#pragma once
#include <unordered_map>
#include <SDL2/SDL.h>

class InputHandler {
public:
    InputHandler();

    void handleInput();
    bool isKeyPressed(SDL_Keycode key) const;
    bool isQuitRequested() const;
    bool isKeyJustPressed(SDL_Keycode key) const;

private:
    std::unordered_map<SDL_Keycode, bool> keyStates;
    bool quitRequested;
    std::unordered_map<SDL_Keycode, bool> keyRepeatStates;
};