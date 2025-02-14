#include "Game.hpp"


Game::Game(int windowWidth, int windowHeight, int cellSize)
    : board(20, 10, cellSize, {0, 0, 255, 255}),
      currentShape(Shape::Type::O, board.getCols() / 2, 0, {255, 255, 255, 255}),
      shadowShape(currentShape),
      running(true),
      lastMoveTime(SDL_GetTicks()),
      speed(500),
      cellSize(cellSize),
      windowWidth(windowWidth),
      windowHeight(windowHeight),
      lastHorizontalMoveTime(0),
      lastDownMoveTime(0),
      lastRotationTime(0),
      horizontalMoveDelay(50),
      downMoveDelay(100),
      rotationDelay(100),
      level(1),
      totalLinesCleared(0)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error("SDL Initialization failed");
    }

    window = SDL_CreateWindow(
        "Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
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

    const Uint32 autoRepeatInitialDelay = 400;
    const Uint32 autoRepeatInterval     = 100;

    static bool leftKeyHandled = false;
    static Uint32 leftLastMoveTime = 0;
    static bool leftFirstRepeat = true;

    if (inputHandler.isKeyPressed(SDLK_LEFT)) {
        if (!leftKeyHandled) {
            if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                currentShape.moveLeft(board.getCols());
            }
            leftKeyHandled = true;
            leftLastMoveTime = currentTime;
            leftFirstRepeat = true;
        } else {
            if (leftFirstRepeat && (currentTime - leftLastMoveTime >= autoRepeatInitialDelay)) {
                if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                    currentShape.moveLeft(board.getCols());
                }
                leftLastMoveTime = currentTime;
                leftFirstRepeat = false;
            } else if (!leftFirstRepeat && (currentTime - leftLastMoveTime >= autoRepeatInterval)) {
                if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                    currentShape.moveLeft(board.getCols());
                }
                leftLastMoveTime = currentTime;
            }
        }
    } else {
        leftKeyHandled = false;
    }

    static bool rightKeyHandled = false;
    static Uint32 rightLastMoveTime = 0;
    static bool rightFirstRepeat = true;

    if (inputHandler.isKeyPressed(SDLK_RIGHT)) {
        if (!rightKeyHandled) {
            if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                currentShape.moveRight(board.getCols());
            }
            rightKeyHandled = true;
            rightLastMoveTime = currentTime;
            rightFirstRepeat = true;
        } else {
            if (rightFirstRepeat && (currentTime - rightLastMoveTime >= autoRepeatInitialDelay)) {
                if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                    currentShape.moveRight(board.getCols());
                }
                rightLastMoveTime = currentTime;
                rightFirstRepeat = false;
            } else if (!rightFirstRepeat && (currentTime - rightLastMoveTime >= autoRepeatInterval)) {
                if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                    currentShape.moveRight(board.getCols());
                }
                rightLastMoveTime = currentTime;
            }
        }
    } else {
        rightKeyHandled = false;
    }

    int boardOffsetX = 50;
    int mouseX = inputHandler.getMouseX() - boardOffsetX;
    static int prevMouseX = -1;
    bool isMouseInsideBoard = (mouseX >= 0 && mouseX < board.getCols() * cellSize);
    bool isMouseMoving = (mouseX != prevMouseX);
    prevMouseX = mouseX;

    if (isMouseInsideBoard && isMouseMoving) {
        int targetGridX = (mouseX >= 0) ? (mouseX / cellSize) : 0;
        targetGridX = std::max(0, std::min(targetGridX, board.getCols() - 1));

        int currentX = currentShape.getCoords()[0].first;
        if (targetGridX > currentX) {
            if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                currentShape.moveRight(board.getCols());
            }
        } else if (targetGridX < currentX) {
            if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                currentShape.moveLeft(board.getCols());
            }
        }

        if (currentTime - lastRotationTime >= rotationDelay) {
            autoRotateCurrentShape(targetGridX);
            lastRotationTime = currentTime;
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
        int clearedLines = board.clearFullLines();
    
        if (clearedLines > 0) {
            totalLinesCleared += clearedLines;
            std::cout << "Total Lines Cleared: " << totalLinesCleared << std::endl;
            checkLevelUp();
        }
    
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
            int clearedLines = board.clearFullLines();

            if (clearedLines > 0) {
                totalLinesCleared += clearedLines;
                std::cout << "Total Lines Cleared: " << totalLinesCleared << std::endl;
                checkLevelUp();
            }

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
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    int boardOffsetX = 50;
    int boardOffsetY = 10;

    board.draw(renderer, boardOffsetX, boardOffsetY);
    shadowShape.draw(renderer, board.getCellSize(), boardOffsetX, boardOffsetY, true);
    currentShape.draw(renderer, board.getCellSize(), boardOffsetX, boardOffsetY);

    renderNextPieces();

    SDL_RenderPresent(renderer);
}


bool Game::isGameOver() const {
    return board.isOccupied(currentShape.getCoords(), 0, 0);
}

int Game::evaluateLanding(const Shape &candidate) {
    int contact = 0;
    const auto &grid = board.getGrid();
    int rows = board.getRows();

    for (const auto &coord : candidate.getCoords()) {
        int x = coord.first;
        int y = coord.second;
        if (y == rows - 1 || grid[y + 1][x] != 0) {
            contact++;
        }
    }
    
    int landingHeight = 0;
    for (const auto &coord : candidate.getCoords()) {
        landingHeight += coord.second;
    }

    return contact * 10 - landingHeight;
}

void Game::autoRotateCurrentShape(int targetGridX) {
    if (currentShape.getType() == Shape::Type::I) {
        auto isHorizontal = [&](const Shape &s) -> bool {
            const auto &coords = s.getCoords();
            int y0 = coords[0].second;
            for (const auto &p : coords) {
                if (p.second != y0)
                    return false;
            }
            return true;
        };

        auto [surfaceCoords, isFlat] = board.getSurfaceCoordsAndFlatStatus(targetGridX);

        if (isFlat) {
            Shape horizontalCandidate = currentShape;
            for (int i = 0; i < 4; ++i) {
                if (isHorizontal(horizontalCandidate))
                    break;
                horizontalCandidate.rotateClockwise(board.getGrid(), board.getCols(), board.getRows());
            }

            Shape dropHorizontal = horizontalCandidate;
            while (!board.isOccupied(dropHorizontal.getCoords(), 0, 1)) {
                dropHorizontal.moveDown();
            }
            for (auto &p : dropHorizontal.coords) {
                p.second -= 1;
            }

            bool horizontalFits = true;
            for (const auto &p : dropHorizontal.getCoords()) {
                if (p.second != board.getRows() - 1) {
                    horizontalFits = false;
                    break;
                }
            }

            if (horizontalFits) {
                currentShape = horizontalCandidate;
                return;
            }
        } 

        auto isVertical = [&](const Shape &s) -> bool {
            const auto &coords = s.getCoords();
            int x0 = coords[0].first;
            for (const auto &p : coords) {
                if (p.first != x0)
                    return false;
            }
            return true;
        };

        for (int i = 0; i < 4; ++i) {
            if (isVertical(currentShape))
                break;
            currentShape.rotateClockwise(board.getGrid(), board.getCols(), board.getRows());
        }
    }

    int bestScore = -100000;
    Shape bestOrientation = currentShape;
    Shape original = currentShape;

    for (int rotation = 0; rotation < 4; ++rotation) {
        Shape candidate = original;

        for (int r = 0; r < rotation; r++) {
            candidate.rotateClockwise(board.getGrid(), board.getCols(), board.getRows());
        }

        Shape dropped = candidate;
        Board tempBoard = board;

        while (!tempBoard.isOccupied(dropped.getCoords(), 0, 1)) {
            dropped.moveDown();
        }
        tempBoard.placeShape(dropped);

        int linesCleared = tempBoard.countFullLines();
        int sumY = 0;
        for (auto &p : dropped.getCoords()) {
            sumY += p.second;
        }
        int holes = tempBoard.countHoles();

        int score = linesCleared * 100 + sumY - holes * 10;

        if (score > bestScore) {
            bestScore = score;
            bestOrientation = candidate;
        }
    }

    currentShape = bestOrientation;
}

void Game::renderNextPieces() {
    int sidebarX = board.getCols() * cellSize + 100;
    int sidebarY = 50;

    SDL_Rect sidebarRect = {sidebarX, sidebarY, 150, 400};
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &sidebarRect);

    int previewX = sidebarX - (160 - cellSize);
    int previewY = sidebarY + 20;
    int previewCellSize = cellSize * 0.75;

    int spacing = 120;

    for (size_t i = 0; i < std::min(nextPieces.size(), size_t(3)); i++) {
        nextPieces[i].draw(renderer, previewCellSize, previewX, previewY + i * spacing, false);
    }
}

void Game::spawnNewShape() {
    if (nextPieces.empty()) {
        for (int i = 0; i < 3; i++) {
            Shape::Type newType = static_cast<Shape::Type>(rand() % 7);
            nextPieces.push_back(Shape(newType, board.getCols() / 2, 0, {255, 255, 255, 255}));
        }
    }

    currentShape = nextPieces.front();
    nextPieces.pop_front();

    Shape::Type newType = static_cast<Shape::Type>(rand() % 7);
    nextPieces.push_back(Shape(newType, board.getCols() / 2, 0, {255, 255, 255, 255}));
}

void Game::checkLevelUp() {
    int newLevel = (totalLinesCleared / 10) + 1;

    if (newLevel > level) {
        level = newLevel;
        //TODO updateSpeed();
        std::cout << "Level Up New Level: " << level << std::endl;
    }
}