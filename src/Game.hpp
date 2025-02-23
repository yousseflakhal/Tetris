#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <deque>
#include <string>
#include "Board.hpp"
#include "Shape.hpp"
#include "InputHandler.hpp"
#include <optional>
#include <cmath>

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

    Shape shadowShape;

    int windowWidth;
    int windowHeight;
    std::deque<Shape> nextPieces;

    int level;
    int totalLinesCleared;

    int score;
    TTF_Font* font;

    std::optional<Shape> heldShape;
    bool canHold;

    void processInput();
    void update();
    void render();
    void spawnNewShape();
    bool isGameOver() const;
    int evaluateLanding(const Shape &candidate);
    void autoRotateCurrentShape(int targetGridX);
    void renderNextPieces();
    void checkLevelUp();
    void updateScore(int clearedLines, int dropDistance, bool hardDrop);
    void renderText(const std::string& text, int x, int y, SDL_Color color);
    void holdPiece();
    void renderHoldPiece();

};
