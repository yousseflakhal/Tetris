#include "Game.hpp"


Game::Game(int width, int height, int cellSize)
    : board(height / cellSize, width / cellSize, cellSize, {0, 0, 255, 255}),
      currentShape(Shape::Type::O, board.getCols() / 2, 0, {255, 0, 0, 255}),
      running(true),
      lastMoveTime(SDL_GetTicks()),
      speed(500)
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
    Uint32 currentTime = SDL_GetTicks();

    
    inputHandler.handleInput();

    
    if (inputHandler.isQuitRequested()) {
        running = false;
        return;
    }

    // Handle left movement
    if (inputHandler.isKeyPressed(SDLK_LEFT)) {
        if (currentTime - lastHorizontalMoveTime >= horizontalMoveDelay &&
            !board.isOccupied(currentShape.getCoords(), -1, 0)) {
            currentShape.moveLeft();
            lastHorizontalMoveTime = currentTime;
        }
    }

    
    if (inputHandler.isKeyPressed(SDLK_RIGHT)) {
        if (currentTime - lastHorizontalMoveTime >= horizontalMoveDelay &&
            !board.isOccupied(currentShape.getCoords(), 1, 0)) {
            currentShape.moveRight();
            lastHorizontalMoveTime = currentTime;
        }
    }

    
    if (inputHandler.isKeyPressed(SDLK_DOWN)) {
        if (currentTime - lastDownMoveTime >= downMoveDelay &&
            !board.isOccupied(currentShape.getCoords(), 0, 1)) {
            currentShape.moveDown();
            lastDownMoveTime = currentTime;
        }
    }

    if (inputHandler.isKeyPressed(SDLK_UP)) {
        if (currentTime - lastRotationTime >= rotationDelay) {
            currentShape.rotateClockwise(board.getGrid(), board.getCols(), board.getRows());
            lastRotationTime = currentTime;
        }
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
}



void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderClear(renderer);

    board.draw(renderer);
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