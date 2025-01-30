#define SDL_MAIN_HANDLED
#include "Game.hpp"
#include <iostream>

int main() {
    try {
        const int windowWidth = 400;
        const int windowHeight = 800;
        const int cellSize = 40;

        Game tetrisGame(windowWidth, windowHeight, cellSize);
        tetrisGame.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}