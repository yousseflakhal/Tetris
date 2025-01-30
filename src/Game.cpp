#include "Game.hpp"


Game::Game(int width, int height, int cellSize)
    : board(height / cellSize, width / cellSize, cellSize,  {0, 0, 0, 255}),
      currentShape(Shape::Type::O, board.getCols() / 2, 0, {255, 255, 255, 255}),
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
    SDL_Event event;
    static std::unordered_map<SDL_Keycode, bool> keyState;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
            return;
        }

        if (event.type == SDL_KEYDOWN) {
            keyState[event.key.keysym.sym] = true;
        } else if (event.type == SDL_KEYUP) {
            keyState[event.key.keysym.sym] = false;
        }
    }

    Uint32 currentTime = SDL_GetTicks();

    if (keyState[SDLK_ESCAPE]) {
        running = false;
        return;
    }

    if (keyState[SDLK_LEFT] && currentTime - lastHorizontalMoveTime >= horizontalMoveDelay &&
        !board.isOccupied(currentShape.getCoords(), -1, 0)) {
        currentShape.moveLeft();
        lastHorizontalMoveTime = currentTime;
    }

    if (keyState[SDLK_RIGHT] && currentTime - lastHorizontalMoveTime >= horizontalMoveDelay &&
        !board.isOccupied(currentShape.getCoords(), 1, 0)) {
        currentShape.moveRight();
        lastHorizontalMoveTime = currentTime;
    }

    if (keyState[SDLK_DOWN] && currentTime - lastDownMoveTime >= downMoveDelay &&
        !board.isOccupied(currentShape.getCoords(), 0, 1)) {
        currentShape.moveDown();
        lastDownMoveTime = currentTime;
    }
    static bool rotationKeyHandled = false;

    if (keyState[SDLK_UP]) {
        if (!rotationKeyHandled) {
            currentShape.rotateClockwise(board.getGrid(), board.getCols(), board.getRows());
            rotationKeyHandled = true;
        }
    } else {
        rotationKeyHandled = false;
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