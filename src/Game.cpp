#include "Game.hpp"


Game::Game(int windowWidth, int windowHeight, int cellSize)
    : board(20, 10, cellSize, {0, 0, 255, 255}),
      currentShape(Shape::Type::O, board.getCols() / 2, 0, {255, 255, 255, 255}),
      canHold(true),
      shadowShape(currentShape),
      inputHandler(),
      running(true),
      lastMoveTime(SDL_GetTicks()),
      speed(800),
      cellSize(cellSize),
      lastHorizontalMoveTime(0),
      lastDownMoveTime(0),
      lastRotationTime(0),
      horizontalMoveDelay(50),
      downMoveDelay(100),
      rotationDelay(100),
      windowWidth(windowWidth),
      windowHeight(windowHeight),
      nextPieces(),
      level(1),
      totalLinesCleared(0),
      score(0),
      font(nullptr),
      heldShape(std::nullopt),
      ignoreNextMouseClick(false),
      isPaused(false)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error("SDL Initialization failed");
    }

    if (TTF_Init() == -1) {
        throw std::runtime_error("Failed to initialize SDL_ttf: " + std::string(TTF_GetError()));
    }

    font = TTF_OpenFont("fonts/DejaVuSans.ttf", 32);
    if (!font) {
        throw std::runtime_error("Failed to load font: " + std::string(TTF_GetError()));
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

    newGameButton.rect = {windowWidth / 2 - 100, windowHeight / 2 - 60, 200, 50};
    newGameButton.text = "New Game";
    newGameButton.color = {200, 200, 200, 255};

    quitButton.rect = {windowWidth / 2 - 100, windowHeight / 2 + 60, 200, 50};
    quitButton.text = "Quit";
    quitButton.color = {200, 200, 200, 255};

    resumeButton.rect = {windowWidth / 2 - 100, windowHeight / 2 - 120, 200, 50};
    resumeButton.text = "Resume";
    resumeButton.color = {200, 200, 200, 255};

    settingsButton.rect = {windowWidth / 2 - 100, windowHeight / 2, 200, 50};
    settingsButton.text = "Settings";
    settingsButton.color = {200, 200, 200, 255};

    spawnNewShape();
}

Game::~Game() {
    if (font) {
        TTF_CloseFont(font);
    }
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
    inputHandler.resetQuitRequested();
    inputHandler.handleInput();

    if (inputHandler.isQuitRequested()) {
        running = false;
        return;
    }

    if (isPaused) {
        int mouseX = inputHandler.getMouseX();
        int mouseY = inputHandler.getMouseY();

        bool hoverResume = mouseX >= resumeButton.rect.x && mouseX <= resumeButton.rect.x + resumeButton.rect.w &&
                           mouseY >= resumeButton.rect.y && mouseY <= resumeButton.rect.y + resumeButton.rect.h;

        bool hoverNewGame = mouseX >= newGameButton.rect.x && mouseX <= newGameButton.rect.x + newGameButton.rect.w &&
                            mouseY >= newGameButton.rect.y && mouseY <= newGameButton.rect.y + newGameButton.rect.h;

        bool hoverQuit = mouseX >= quitButton.rect.x && mouseX <= quitButton.rect.x + quitButton.rect.w &&
                         mouseY >= quitButton.rect.y && mouseY <= quitButton.rect.y + quitButton.rect.h;
        
        bool hoverSettings = mouseX >= settingsButton.rect.x && mouseX <= settingsButton.rect.x + settingsButton.rect.w &&
                         mouseY >= settingsButton.rect.y && mouseY <= settingsButton.rect.y + settingsButton.rect.h;

        resumeButton.color = hoverResume ? SDL_Color{255, 255, 255, 255} : SDL_Color{200, 200, 200, 255};
        newGameButton.color = hoverNewGame ? SDL_Color{255, 255, 255, 255} : SDL_Color{200, 200, 200, 255};
        quitButton.color = hoverQuit ? SDL_Color{255, 255, 255, 255} : SDL_Color{200, 200, 200, 255};
        settingsButton.color = hoverSettings ? SDL_Color{255, 255, 255, 255} : SDL_Color{200, 200, 200, 255};

        if (inputHandler.isMouseClicked()) {
            if (ignoreNextMouseClick) {
                ignoreNextMouseClick = false;
                return;
            }

            if (hoverResume) {
                isPaused = false;
            } else if (hoverNewGame) {
                resetGame();
                isPaused = false;
            } else if (hoverSettings) {
                // TODO: Open settings screen
            } else if (hoverQuit) {
                running = false;
            }
            return;
        }

        if (inputHandler.isKeyJustPressed(SDLK_ESCAPE)) {
            isPaused = !isPaused;
            inputHandler.clearKeyState(SDLK_ESCAPE);
            return;
        }
    }

    if (inputHandler.isKeyJustPressed(SDLK_ESCAPE)) {
        isPaused = !isPaused;
        inputHandler.clearKeyState(SDLK_ESCAPE);
        return;
    }

    if (isGameOver()) {
        int mouseX = inputHandler.getMouseX();
        int mouseY = inputHandler.getMouseY();

        bool hoverNewGame = mouseX >= newGameButton.rect.x && mouseX <= newGameButton.rect.x + newGameButton.rect.w &&
                            mouseY >= newGameButton.rect.y && mouseY <= newGameButton.rect.y + newGameButton.rect.h;

        bool hoverQuit = mouseX >= quitButton.rect.x && mouseX <= quitButton.rect.x + quitButton.rect.w &&
                         mouseY >= quitButton.rect.y && mouseY <= quitButton.rect.y + quitButton.rect.h;

        newGameButton.color = hoverNewGame ? SDL_Color{255, 255, 255, 255} : SDL_Color{200, 200, 200, 255};
        quitButton.color = hoverQuit ? SDL_Color{255, 255, 255, 255} : SDL_Color{200, 200, 200, 255};

        if (inputHandler.isMouseClicked()) {
            if (ignoreNextMouseClick) {
                ignoreNextMouseClick = false;
                return;
            }

            if (hoverNewGame) {
                resetGame();
            } else if (hoverQuit) {
                running = false;
            }
            return;
        }
        return;
    }

    Uint32 currentTime = SDL_GetTicks();
    const Uint32 autoRepeatInitialDelay = 400;
    const Uint32 autoRepeatInterval = 100;

    static bool leftKeyHandled = false;
    static Uint32 leftLastMoveTime = 0;
    static bool leftFirstRepeat = true;

    if (inputHandler.isKeyPressed(SDLK_LEFT)) {
        if (!leftKeyHandled) {
            if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                currentShape.moveLeft();
            }
            leftKeyHandled = true;
            leftLastMoveTime = currentTime;
            leftFirstRepeat = true;
        } else {
            if (leftFirstRepeat && (currentTime - leftLastMoveTime >= autoRepeatInitialDelay)) {
                if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                    currentShape.moveLeft();
                }
                leftLastMoveTime = currentTime;
                leftFirstRepeat = false;
            } else if (!leftFirstRepeat && (currentTime - leftLastMoveTime >= autoRepeatInterval)) {
                if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                    currentShape.moveLeft();
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

    int boardOffsetX = 200;
    int mouseX = inputHandler.getMouseX() - boardOffsetX;
    static int prevMouseX = -1;
    bool isMouseInsideBoard = (mouseX >= 0 && mouseX < board.getCols() * cellSize);
    bool isMouseMoving = (mouseX != prevMouseX);
    prevMouseX = mouseX;

    if (isMouseInsideBoard && isMouseMoving) {
        int targetGridX = std::round(static_cast<float>(mouseX) / cellSize);
        targetGridX = std::clamp(targetGridX, 0, board.getCols() - 1);

        int currentX = currentShape.getCoords()[0].first;
        if (targetGridX > currentX) {
            if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                currentShape.moveRight(board.getCols());
            }
        } else if (targetGridX < currentX) {
            if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                currentShape.moveLeft();
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
            updateScore(0, 1, false);
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
        if (ignoreNextMouseClick) {
            ignoreNextMouseClick = false;
            return;
        }

        int dropDistance = 0;
        while (!board.isOccupied(currentShape.getCoords(), 0, 1)) {
            currentShape.moveDown();
            dropDistance++;
        }
        board.placeShape(currentShape);
        int clearedLines = board.clearFullLines();
        updateScore(clearedLines, dropDistance, true);

        spawnNewShape();
    }

    if (inputHandler.isKeyJustPressed(SDLK_c) || inputHandler.isKeyJustPressed(SDLK_LSHIFT)) {
        holdPiece();
    }
}


void Game::update() {
    Uint32 currentTime = SDL_GetTicks();

    if (board.isClearingLines) {
        if (board.clearAnimationFrame < 10) {
            board.clearAnimationFrame++;
            return;
        } else {
            board.finalizeLineClear();
        }
    }

    if (currentTime - lastMoveTime >= static_cast<Uint32>(speed)) {
        if (!board.isOccupied(currentShape.getCoords(), 0, 1)) {
            currentShape.moveDown();
        } else {
            board.placeShape(currentShape);
            int clearedLines = board.clearFullLines();

            spawnNewShape();

            if (isGameOver()) {
                return;
            }
        }
        lastMoveTime = currentTime;
    }

    shadowShape = currentShape;
    while (!board.isOccupied(shadowShape.getCoords(), 0, 1)) {
        shadowShape.moveDown();
    }
}


void Game::render() {
    if (isPaused) {
        renderPauseMenu();
        return;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    int boardOffsetX = 200;
    int boardOffsetY = 10;

    board.draw(renderer, boardOffsetX, boardOffsetY);

    if (!isGameOver()) {
        shadowShape.draw(renderer, board.getCellSize(), boardOffsetX, boardOffsetY, true);
        currentShape.draw(renderer, board.getCellSize(), boardOffsetX, boardOffsetY);
    }

    renderNextPieces();
    renderHoldPiece();

    int sidebarX = 10;
    int textY = 250;
    SDL_Color textColor = {255, 255, 255, 255};
    renderText("Score: " + std::to_string(score), sidebarX, textY, textColor);
    textY += 50;
    renderText("Level: " + std::to_string(level), sidebarX, textY, textColor);
    textY += 50;
    renderText("Lines: " + std::to_string(totalLinesCleared), sidebarX, textY, textColor);

    if (isGameOver()) {
        renderGameOverScreen();
    }

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
    int sidebarX = board.getCols() * cellSize + 300;
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

    if (isGameOver()) {
        // running = false;
        return;
    }

    Shape::Type newType = static_cast<Shape::Type>(rand() % 7);
    nextPieces.push_back(Shape(newType, board.getCols() / 2, 0, {255, 255, 255, 255}));

    canHold = true;
}


void Game::checkLevelUp() {
    int newLevel = (totalLinesCleared / 10) + 1;

    if (newLevel > level) {
        level = newLevel;
        updateSpeed();
        std::cout << "Level Up New Level: " << level << std::endl;
    }
}

void Game::updateScore(int clearedLines, int dropDistance, bool hardDrop) {
    if (clearedLines > 0) {
        totalLinesCleared += clearedLines;
    }

    int points = 0;

    switch (clearedLines) {
        case 1: points += 40 * (level + 1); break;
        case 2: points += 100 * (level + 1); break;
        case 3: points += 300 * (level + 1); break;
        case 4: points += 1200 * (level + 1); break;
    }

    if (hardDrop) {
        points += dropDistance * 2;
    } else {
        points += dropDistance * 1;
    }

    score += points;

    checkLevelUp();
}


void Game::renderText(const std::string& text, int x, int y, SDL_Color color) {
    if (!font) {
        std::cerr << "Font not initialized!" << std::endl;
        return;
    }

    if (text.empty()) {
        std::cerr << "Skipping rendering empty text" << std::endl;
        return;
    }

    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!textSurface || textSurface->w == 0) {
        std::cerr << "Text rendering failed: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        return;
    }

    SDL_Rect textRect = { x, y, textSurface->w, textSurface->h };
    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}



void Game::holdPiece() {
    if (!canHold) return;

    if (heldShape.has_value()) {
        std::swap(currentShape, heldShape.value());
        currentShape.setPosition(board.getCols() / 2, 0);
        currentShape.resetRotation();
        heldShape->setPosition(0, 0);
    } else {
        heldShape = currentShape;
        heldShape->setPosition(0, 0);
        spawnNewShape();
    }

    canHold = false;
}

void Game::renderHoldPiece() {
    int holdBoxX = 20;
    int holdBoxY = 70;
    int holdBoxWidth = 120;
    int holdBoxHeight = 120;

    SDL_Rect holdBox = {holdBoxX - 10, holdBoxY - 10, holdBoxWidth, holdBoxHeight};
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &holdBox);

    SDL_Color textColor = {255, 255, 255, 255};
    renderText("HOLD", holdBoxX + 20, holdBoxY - 50, textColor);

    if (!heldShape.has_value()) return;

    int cellSize = board.getCellSize() * 0.75;
    
    auto localCoords = heldShape->getLocalCoords();
    
    int minX = 0, maxX = 0;
    int minY = 0, maxY = 0;
    for (const auto& coord : localCoords) {
        minX = std::min(minX, coord.first);
        maxX = std::max(maxX, coord.first);
        minY = std::min(minY, coord.second);
        maxY = std::max(maxY, coord.second);
    }
    
    int shapeWidth = maxX - minX + 1;
    int shapeHeight = maxY - minY + 1;
    
    int drawX = (holdBoxX - 10) + (holdBoxWidth - shapeWidth * cellSize) / 2;
    int drawY = (holdBoxY - 10) + (holdBoxHeight - shapeHeight * cellSize) / 2;

    heldShape->draw(renderer, cellSize, drawX, drawY, false);
}

void Game::renderGameOverScreen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = {0, 0, windowWidth, windowHeight};
    SDL_RenderFillRect(renderer, &overlay);

    SDL_Color textColor = {255, 255, 255, 255};
    int centerX = windowWidth / 2;

    renderText("GAME OVER", centerX - 100, windowHeight / 2 - 100, textColor);

    renderButton(newGameButton);
    renderButton(quitButton);

    SDL_RenderPresent(renderer);
}


void Game::resetGame() {
    board.clearBoard();
    score = 0;
    totalLinesCleared = 0;
    level = 1;
    nextPieces.clear();
    canHold = true;
    heldShape.reset();
    spawnNewShape();
    running = true;
    ignoreNextMouseClick = true;
}

void Game::renderButton(const Button &button) {
    SDL_SetRenderDrawColor(renderer, button.color.r, button.color.g, button.color.b, button.color.a);
    SDL_RenderFillRect(renderer, &button.rect);

    SDL_Color textColor = {0, 0, 0, 255};
    renderText(button.text, button.rect.x + 20, button.rect.y + 10, textColor);
}

void Game::updateSpeed() {
    if (level == 0) speed = 800;
    else if (level == 1) speed = 717;
    else if (level == 2) speed = 633;
    else if (level == 3) speed = 550;
    else if (level == 4) speed = 467;
    else if (level == 5) speed = 383;
    else if (level == 6) speed = 300;
    else if (level == 7) speed = 217;
    else if (level == 8) speed = 133;
    else if (level == 9) speed = 100;
    else if (level >= 10 && level <= 12) speed = 83;
    else if (level >= 13 && level <= 15) speed = 67;
    else if (level >= 16 && level <= 18) speed = 50;
    else if (level >= 19 && level <= 28) speed = 33;
    else speed = 16;
}

void Game::renderPauseMenu() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = {0, 0, windowWidth, windowHeight};
    SDL_RenderFillRect(renderer, &overlay);

    SDL_Color textColor = {255, 255, 255, 255};
    renderText("PAUSED", windowWidth / 2 - 60, windowHeight / 2 - 180, textColor);

    renderButton(resumeButton);
    renderButton(newGameButton);
    renderButton(settingsButton);
    renderButton(quitButton);

    SDL_RenderPresent(renderer);
}