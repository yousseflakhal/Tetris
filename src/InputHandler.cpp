#include "InputHandler.hpp"

InputHandler::InputHandler() 
    : keyStates{}, quitRequested(false), mouseX(0), mouseY(0), mouseClicked(false) {}

void InputHandler::beginFrame() {
    mouseClicked = false;
    keysJustPressed.clear();
}
void InputHandler::handleEvent(const SDL_Event &event) {
    if (event.type == SDL_KEYDOWN) {
        const auto key = event.key.keysym.sym;
        keyStates[key] = true;
        keyRepeatStates[key] = event.key.repeat;
        if (!event.key.repeat) keysJustPressed.insert(key);
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

bool InputHandler::isKeyPressed(SDL_Keycode key) const noexcept {
    auto it = keyStates.find(key);
    return it != keyStates.end() && it->second;
}

bool InputHandler::isQuitRequested() const noexcept {
    return quitRequested;
}

bool InputHandler::isKeyJustPressed(SDL_Keycode key) const noexcept {
    return keysJustPressed.find(key) != keysJustPressed.end();
}

int InputHandler::getMouseX() const noexcept {
    return mouseX;
}

int InputHandler::getMouseY() const noexcept {
    return mouseY;
}

bool InputHandler::isMouseClicked() const noexcept {
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