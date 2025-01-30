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
    int getMouseX() const;
    int getMouseY() const;
    bool isMouseClicked() const;

private:
    std::unordered_map<SDL_Keycode, bool> keyStates;
    std::unordered_map<SDL_Keycode, bool> keyRepeatStates;
    bool quitRequested;
    int mouseX;
    int mouseY;
    bool mouseClicked;
};