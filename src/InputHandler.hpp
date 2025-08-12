#pragma once
#include <unordered_map>
#include <unordered_set>
#include <SDL2/SDL.h>

class InputHandler {
public:
    InputHandler();
    void resetQuitRequested();
    bool isKeyPressed(SDL_Keycode key) const noexcept;
    bool isQuitRequested() const noexcept;
    bool isKeyJustPressed(SDL_Keycode key) const noexcept;
    int getMouseX() const noexcept;
    int getMouseY() const noexcept;
    bool isMouseClicked() const noexcept;
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