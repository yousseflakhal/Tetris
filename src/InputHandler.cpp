#include "InputHandler.hpp"

InputHandler::InputHandler() : keyStates{}, quitRequested(false) {}

void InputHandler::handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            if (!event.key.repeat) {
                keyStates[event.key.keysym.sym] = true;
                keyRepeatStates[event.key.keysym.sym] = false;
            } else {
                keyRepeatStates[event.key.keysym.sym] = true;
            }
        } else if (event.type == SDL_KEYUP) {
            keyStates[event.key.keysym.sym] = false;
            keyRepeatStates[event.key.keysym.sym] = false;
        } else if (event.type == SDL_QUIT) {
            quitRequested = true;
        }
    }
}

bool InputHandler::isKeyPressed(SDL_Keycode key) const {
    auto it = keyStates.find(key);
    return it != keyStates.end() && it->second;
}

bool InputHandler::isQuitRequested() const {
    return quitRequested;
}

bool InputHandler::isKeyJustPressed(SDL_Keycode key) const {
    auto it = keyStates.find(key);
    return it != keyStates.end() && it->second && !keyRepeatStates.at(key);
}