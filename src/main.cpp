#define SDL_MAIN_HANDLED
#define SDLFORMUI_IMPLEMENTATION
#include "SDLFormUI.hpp"
#include "Game.hpp"
#include <iostream>

int main() {
    try {
        const int boardWidth = 600;
        const int boardHeight = 800;
        const int sidebarWidth = 400;
        const int windowWidth = boardWidth + sidebarWidth;
        const int windowHeight = boardHeight + 100;

        const int cellSize = 40;

        Game tetrisGame(windowWidth, windowHeight, cellSize);
        tetrisGame.run();
        SDL_Quit();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}