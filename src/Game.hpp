#pragma once
#include <SDL2/SDL.h>
#include "Board.hpp"
#include "Shape.hpp"
#include "InputHandler.hpp"

Uint32 lastHorizontalMoveTime = 0;
const Uint32 horizontalMoveDelay = 150;
Uint32 lastDownMoveTime = 0;
Uint32 lastRotationTime = 0;
const Uint32 downMoveDelay = 100;
const Uint32 rotationDelay = 100;

class Game {
public:
    Game(int width, int height, int cellSize);
    ~Game();
    void run();

private:

    SDL_Window* window;
    SDL_Renderer* renderer;


    Board board;
    Shape currentShape;
    InputHandler inputHandler;
    bool running;
    Uint32 lastMoveTime;
    const int speed;

    void processInput();
    void update();
    void render();
    void spawnNewShape();
    bool isGameOver() const;
};
