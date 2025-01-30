#pragma once
#include <SDL2/SDL.h>
#include "Board.hpp"
#include "Shape.hpp"
#include "InputHandler.hpp"

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
    int cellSize;
    Uint32 lastHorizontalMoveTime;
    Uint32 lastDownMoveTime;
    Uint32 lastRotationTime;
    const Uint32 horizontalMoveDelay;
    const Uint32 downMoveDelay;
    const Uint32 rotationDelay;

    void processInput();
    void update();
    void render();
    void spawnNewShape();
    bool isGameOver() const;
};
