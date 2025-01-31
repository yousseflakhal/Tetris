#include "Game.hpp"


Game::Game(int width, int height, int cellSize)
    : board(height / cellSize, width / cellSize, cellSize, {0, 0, 255, 255}),
      currentShape(Shape::Type::O, board.getCols() / 2, 0, {255, 255, 255, 255}),
      shadowShape(currentShape),
      running(true),
      lastMoveTime(SDL_GetTicks()),
      speed(500),
      cellSize(cellSize),
      lastHorizontalMoveTime(0),
      lastDownMoveTime(0),
      lastRotationTime(0),
      horizontalMoveDelay(50),
      downMoveDelay(100),
      rotationDelay(100)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error("SDL Initialization failed");
    }

    window = SDL_CreateWindow(
        "Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (!window) {
        throw std::runtime_error("Failed to create window");
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        throw std::runtime_error("Failed to create renderer");
    }

    srand(time(nullptr));

    spawnNewShape();
}

Game::~Game() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::run() {
    while (running) {
        processInput();
        update();
        render();
        SDL_Delay(16);  // Limit frame rate (~60 FPS)
    }
}

void Game::processInput() {
    inputHandler.handleInput();

    if (inputHandler.isQuitRequested()) {
        running = false;
        return;
    }

    if (inputHandler.isKeyPressed(SDLK_ESCAPE)) {
        running = false;
        return;
    }

    Uint32 currentTime = SDL_GetTicks();

    bool isKeyboardMoving = false;
    if (currentTime - lastHorizontalMoveTime >= horizontalMoveDelay) {
        if (inputHandler.isKeyPressed(SDLK_LEFT)) {
            if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                currentShape.moveLeft(board.getCols());
                lastHorizontalMoveTime = currentTime;
                isKeyboardMoving = true;
            }
        }
        if (inputHandler.isKeyPressed(SDLK_RIGHT)) {
            if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                currentShape.moveRight(board.getCols());
                lastHorizontalMoveTime = currentTime;
                isKeyboardMoving = true;
            }
        }
    }

    int mouseX = inputHandler.getMouseX();
    static int prevMouseX = -1;

    bool isMouseInsideBoard = (mouseX >= 0 && mouseX < board.getCols() * cellSize);
    bool isMouseMoving = (mouseX != prevMouseX);
    prevMouseX = mouseX;

    if (isMouseInsideBoard && isMouseMoving && !isKeyboardMoving) {
        int targetGridX = mouseX / cellSize;
        targetGridX = std::max(0, std::min(targetGridX, board.getCols() - 1));

        int currentX = currentShape.getCoords()[0].first;
        if (currentTime - lastHorizontalMoveTime >= horizontalMoveDelay) {
            if (targetGridX > currentX) {
                if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                    currentShape.moveRight(board.getCols());
                    lastHorizontalMoveTime = currentTime;
                }
            } else if (targetGridX < currentX) {
                if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                    currentShape.moveLeft(board.getCols());
                    lastHorizontalMoveTime = currentTime;
                }
            }
        }
    }

    if (inputHandler.isKeyPressed(SDLK_DOWN) && currentTime - lastDownMoveTime >= downMoveDelay) {
        if (!board.isOccupied(currentShape.getCoords(), 0, 1)) {
            currentShape.moveDown();
            lastDownMoveTime = currentTime;
        }
    }

    static bool rotationKeyHandled = false;
    if (inputHandler.isKeyJustPressed(SDLK_UP)) {
        if (!rotationKeyHandled) {
            currentShape.rotateClockwise(board.getGrid(), board.getCols(), board.getRows());
            rotationKeyHandled = true;
        }
    } else {
        rotationKeyHandled = false;
    }

    if (inputHandler.isMouseClicked()) {
        while (!board.isOccupied(currentShape.getCoords(), 0, 1)) {
            currentShape.moveDown();
        }
        board.placeShape(currentShape);
        board.clearFullLines();
        spawnNewShape();
    }
}


void Game::update() {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastMoveTime >= static_cast<Uint32>(speed)) {
        if (!board.isOccupied(currentShape.getCoords(), 0, 1)) {
            currentShape.moveDown();
        } else {
            board.placeShape(currentShape);
            board.clearFullLines();
            spawnNewShape();
        }
        lastMoveTime = currentTime;
    }

    shadowShape = currentShape;
    while (!board.isOccupied(shadowShape.getCoords(), 0, 1)) {
        shadowShape.moveDown();
    }
}



void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderClear(renderer);

    board.draw(renderer);
    shadowShape.draw(renderer, board.getCellSize(), true);
    currentShape.draw(renderer, board.getCellSize());
    SDL_RenderPresent(renderer);
}

void Game::spawnNewShape() {
    Shape::Type type = static_cast<Shape::Type>(rand() % 7);
    currentShape = Shape(type, board.getCols() / 2, 0, {0, 0, 0, 255});

    if (isGameOver()) {
        std::cout << "Game Over! Cannot place the new shape." << std::endl;
        running = false;
    }
}

bool Game::isGameOver() const {
    return board.isOccupied(currentShape.getCoords(), 0, 0);
}