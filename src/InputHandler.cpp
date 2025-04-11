#include "InputHandler.hpp"

InputHandler::InputHandler() 
    : keyStates{}, quitRequested(false), mouseX(0), mouseY(0), mouseClicked(false) {}

void InputHandler::beginFrame() {
    mouseClicked = false;
    keysJustPressed.clear();
}
void InputHandler::handleEvent(const SDL_Event &event) {
    if (event.type == SDL_KEYDOWN && !event.key.repeat) {
        keyStates[event.key.keysym.sym] = true;
        keysJustPressed.insert(event.key.keysym.sym);
    }
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

bool InputHandler::isKeyPressed(SDL_Keycode key) const {
    auto it = keyStates.find(key);
    return it != keyStates.end() && it->second;
}

bool InputHandler::isQuitRequested() const {
    return quitRequested;
}

bool InputHandler::isKeyJustPressed(SDL_Keycode key) const {
    return keysJustPressed.find(key) != keysJustPressed.end();
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

const std::unordered_map<SDL_Keycode, bool>& InputHandler::getKeyStates() const {
    return keyStates;
}