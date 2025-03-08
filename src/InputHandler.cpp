#include "InputHandler.hpp"

InputHandler::InputHandler() 
    : keyStates{}, quitRequested(false), mouseX(0), mouseY(0), mouseClicked(false) {}

void InputHandler::handleInput() {
    SDL_Event event;
    mouseClicked = false;

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
        } else if (event.type == SDL_MOUSEMOTION) {
            mouseX = event.motion.x;
            mouseY = event.motion.y;
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                mouseClicked = true;
            }
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
    auto keyIt = keyStates.find(key);
    auto repeatIt = keyRepeatStates.find(key);
    
    if (keyIt == keyStates.end() || repeatIt == keyRepeatStates.end()) {
        return false;
    }
    
    return keyIt->second && !repeatIt->second;
}

int InputHandler::getMouseX() const {
    return mouseX;
}

int InputHandler::getMouseY() const {
    return mouseY;
}

bool InputHandler::isMouseClicked() const {
    return mouseClicked;
}

void InputHandler::resetQuitRequested() {
    quitRequested = false;
}

void InputHandler::clearKeyState(SDL_Keycode key) {
    keyStates[key] = false;
    keyRepeatStates[key] = false;
}