#pragma once
#include <unordered_map>
#include <unordered_set>
#include <SDL2/SDL.h>

class InputHandler {
public:
    InputHandler();
    void resetQuitRequested();
    bool isKeyPressed(SDL_Keycode key) const;
    bool isQuitRequested() const;
    bool isKeyJustPressed(SDL_Keycode key) const;
    int getMouseX() const;
    int getMouseY() const;
    bool isMouseClicked() const;
    void clearKeyState(SDL_Keycode key);
    void handleEvent(const SDL_Event &event);
    void beginFrame();
    const std::unordered_map<SDL_Keycode, bool>& getKeyStates() const;

private:
    std::unordered_map<SDL_Keycode, bool> keyStates;
    std::unordered_map<SDL_Keycode, bool> keyRepeatStates;
    std::unordered_set<SDL_Keycode> keysJustPressed;

    bool quitRequested;
    int mouseX;
    int mouseY;
    bool mouseClicked;
};