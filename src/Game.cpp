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
      isPaused(false),
      resumeCountdownActive(false),
      countdownStartTime(0),
      mouseControlEnabled(true),
      currentScreen(Screen::Main),
      isMusicPlaying(false)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error("SDL Initialization failed");
    }
    if (TTF_Init() == -1) {
        throw std::runtime_error("Failed to initialize SDL_ttf: " + std::string(TTF_GetError()));
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        throw std::runtime_error("SDL_mixer initialization failed: " + std::string(Mix_GetError()));
    }

    SoundManager::Load();

    font = TTF_OpenFont("fonts/DejaVuSans.ttf", 32);
    if (!font) {
        throw std::runtime_error("Failed to load font: " + std::string(TTF_GetError()));
    }

    window = SDL_CreateWindow(
        "Tetris",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        throw std::runtime_error("Failed to create window");
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        throw std::runtime_error("Failed to create renderer");
    }

    srand(time(nullptr));

    FormUI::Init(font);

    TTF_Font* smallFont = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 10);

    newGameBtn = FormUI::Button(
        "New Game",
        windowWidth / 2 - 100,
        windowHeight / 2 - 60,
        200,
        50,
        [this]() {
            resetGame();
            isPaused = false;
        }
    );

    quitBtn = FormUI::Button(
        "Quit",
        windowWidth / 2 - 100,
        windowHeight / 2 + 60,
        200,
        50,
        [this]() {
            running = false;
        }
    );

    resumeBtn = FormUI::Button(
        "Resume",
        windowWidth / 2 - 100,
        windowHeight / 2 - 120,
        200,
        50,
        [this]() {
            isPaused = false;
            resumeCountdownActive = true;
            countdownStartTime = SDL_GetTicks();
        }
    );

    settingsBtn = FormUI::Button(
        "Settings",
        windowWidth / 2 - 100,
        windowHeight / 2,
        200,
        50,
        [this]() {
            currentScreen = Screen::Settings;
        }
    );

    mouseControlCheckbox = FormUI::Checkbox(
        "Enable Mouse Control",
        windowWidth / 2 - 150,
        150,
        300,
        30,
        &mouseControlEnabled,
        smallFont
    );
    mouseControlCheckbox->visible = false;

    gameOverNewGameBtn = FormUI::Button(
        "New Game",
        windowWidth / 2 - 100,
        windowHeight / 2 - 60,
        200,
        50,
        [this]() {
            resetGame();
        }
    );
    
    gameOverQuitBtn = FormUI::Button(
        "Quit",
        windowWidth / 2 - 100,
        windowHeight / 2 + 10,
        200,
        50,
        [this]() {
            running = false; 
        }
    );
    gameOverNewGameBtn->visible = false;
    gameOverQuitBtn->visible = false;

    FormUI::Layout layout(windowWidth / 2 - 150, 200, 10);

    keyBindings = {
        {Action::MoveRight, SDLK_RIGHT},
        {Action::MoveLeft, SDLK_LEFT},
        {Action::RotateRight, SDLK_UP},
        {Action::RotateLeft, SDLK_z},
        {Action::SoftDrop, SDLK_DOWN},
        {Action::HardDrop, SDLK_SPACE},
        {Action::Hold, SDLK_c}
    };

    auto keyToString = [](SDL_Keycode key) {
        return SDL_GetKeyName(key);
    };
    
    controlMappings = {
        {"MOVE RIGHT", Action::MoveRight},
        {"MOVE LEFT", Action::MoveLeft},
        {"ROTATE RIGHT", Action::RotateRight},
        {"ROTATE LEFT", Action::RotateLeft},
        {"SOFT DROP", Action::SoftDrop},
        {"HARD DROP", Action::HardDrop},
        {"HOLD", Action::Hold}
    };
    
    for (size_t i = 0; i < controlMappings.size(); ++i) {
        const auto& [labelText, action] = controlMappings[i];
        std::string keyLabel = keyToString(keyBindings[action]);
    
        auto buttonCallback = [this, action, i]() {
            waitingForKey = true;
            actionToRebind = action;
            controlButtons[i]->setText("Press a key...");
        };
    
        auto [label, button] = layout.addLabelButtonRow(labelText, keyLabel, buttonCallback, 200, 100, 30, smallFont, smallFont);
        label->visible = false;
        button->visible = false;
        controlLabels.push_back(label);
        controlButtons.push_back(button);
    }

    resetControlsBtn = FormUI::Button(
        "Reset Controls",
        windowWidth / 2 - 100,
        windowHeight - 100,
        200,
        40,
        [this]() {
            keyBindings = {
                {Action::MoveRight, SDLK_RIGHT},
                {Action::MoveLeft, SDLK_LEFT},
                {Action::RotateRight, SDLK_UP},
                {Action::RotateLeft, SDLK_z},
                {Action::SoftDrop, SDLK_DOWN},
                {Action::HardDrop, SDLK_SPACE},
                {Action::Hold, SDLK_c}
            };
            for (size_t i = 0; i < controlButtons.size(); ++i) {
                controlButtons[i]->setText(SDL_GetKeyName(keyBindings[controlMappings[i].second]));
            }
        },
        smallFont
    );
    resetControlsBtn->visible = false;
    


    spawnNewShape();
    resumeCountdownActive = true;
    countdownStartTime = SDL_GetTicks();
}

Game::~Game() {
    if (font) {
        TTF_CloseFont(font);
    }
    SoundManager::CleanUp();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void Game::run() {
    while (running) {
        processInput();
        FormUI::Update();
        update();
        render();
        SDL_Delay(16);  // Limit frame rate (~60 FPS)
    }
}

void Game::processInput() {
    inputHandler.beginFrame();

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        inputHandler.handleEvent(e);
        FormUI::HandleEvent(e);
    
        if (inputHandler.isQuitRequested()) {
            running = false;
            return;
        }
    
        if (currentScreen == Screen::Settings && e.type == SDL_KEYDOWN && !e.key.repeat) {
            if (waitingForKey) {
                SDL_Keycode newKey = e.key.keysym.sym;
            
                if (newKey == SDLK_ESCAPE) {
                    waitingForKey = false;
                    for (size_t i = 0; i < controlButtons.size(); ++i) {
                        if (controlMappings[i].second == actionToRebind) {
                            controlButtons[i]->setText(SDL_GetKeyName(keyBindings[actionToRebind]));
                        }
                    }
                    return;
                }
            
                bool keyAlreadyUsed = false;
                for (const auto& [action, boundKey] : keyBindings) {
                    if (boundKey == newKey && action != actionToRebind) {
                        keyAlreadyUsed = true;
                        break;
                    }
                }
            
                if (keyAlreadyUsed) {
                    waitingForKey = false;
                    for (size_t i = 0; i < controlButtons.size(); ++i) {
                        if (controlMappings[i].second == actionToRebind) {
                            controlButtons[i]->setText(SDL_GetKeyName(keyBindings[actionToRebind]));
                        }
                    }
                    return;
                }
            
                keyBindings[actionToRebind] = newKey;
                waitingForKey = false;
            
                for (size_t i = 0; i < controlButtons.size(); ++i) {
                    if (controlMappings[i].second == actionToRebind) {
                        controlButtons[i]->setText(SDL_GetKeyName(newKey));
                    }
                }
            } else {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    currentScreen = Screen::Main;
                }
            }
        }
    }

    if (resumeCountdownActive) {
        return;
    }

    if (isPaused) {
        if (inputHandler.isKeyJustPressed(SDLK_ESCAPE)) {
            resumeCountdownActive = true;
            countdownStartTime = SDL_GetTicks();
            isPaused = false;
            inputHandler.clearKeyState(SDLK_ESCAPE);
    
            if (!isMusicPlaying) {
                SoundManager::ResumeBackgroundMusic();
                isMusicPlaying = true;
            }
        }
        return;
    }

    if (inputHandler.isKeyJustPressed(SDLK_ESCAPE)) {
        isPaused = true;
        inputHandler.clearKeyState(SDLK_ESCAPE);
        return;
    }

    if (isGameOver()) {
        if (inputHandler.isMouseClicked()) {
            if (ignoreNextMouseClick) {
                ignoreNextMouseClick = false;
            }
        }
        return;
    }

    Uint32 currentTime = SDL_GetTicks();
    const Uint32 autoRepeatInitialDelay = 400;
    const Uint32 autoRepeatInterval = 100;

    static bool leftKeyHandled = false;
    static Uint32 leftLastMoveTime = 0;
    static bool leftFirstRepeat = true;

    if (inputHandler.isKeyPressed(keyBindings[Action::MoveLeft])) {
        if (!leftKeyHandled) {
            if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                currentShape.moveLeft();
                SoundManager::PlayMoveSound();
            }
            leftKeyHandled = true;
            leftLastMoveTime = currentTime;
            leftFirstRepeat = true;
        } else {
            if (leftFirstRepeat && currentTime - leftLastMoveTime >= autoRepeatInitialDelay) {
                if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                    currentShape.moveLeft();
                    SoundManager::PlayMoveSound();
                }
                leftLastMoveTime = currentTime;
                leftFirstRepeat = false;
            } else if (!leftFirstRepeat && currentTime - leftLastMoveTime >= autoRepeatInterval) {
                if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                    currentShape.moveLeft();
                    SoundManager::PlayMoveSound();
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

    if (inputHandler.isKeyPressed(keyBindings[Action::MoveRight])) {
        if (!rightKeyHandled) {
            if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                currentShape.moveRight(board.getCols());
                SoundManager::PlayMoveSound();
            }
            rightKeyHandled = true;
            rightLastMoveTime = currentTime;
            rightFirstRepeat = true;
        } else {
            if (rightFirstRepeat && currentTime - rightLastMoveTime >= autoRepeatInitialDelay) {
                if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                    currentShape.moveRight(board.getCols());
                    SoundManager::PlayMoveSound();
                }
                rightLastMoveTime = currentTime;
                rightFirstRepeat = false;
            } else if (!rightFirstRepeat && currentTime - rightLastMoveTime >= autoRepeatInterval) {
                if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                    currentShape.moveRight(board.getCols());
                    SoundManager::PlayMoveSound();
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

    if (mouseControlEnabled && isMouseInsideBoard && isMouseMoving) {
        int targetGridX = std::round(static_cast<float>(mouseX) / cellSize);
        targetGridX = std::clamp(targetGridX, 0, board.getCols() - 1);

        int currentX = currentShape.getCoords()[0].first;
        if (targetGridX > currentX) {
            if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                currentShape.moveRight(board.getCols());
                SoundManager::PlayMoveSound();
            }
        } else if (targetGridX < currentX) {
            if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                currentShape.moveLeft();
                SoundManager::PlayMoveSound();
            }
        }

        if (currentTime - lastRotationTime >= rotationDelay) {
            autoRotateCurrentShape(targetGridX);
            lastRotationTime = currentTime;
        }
    }

    static bool rotationKeyHandled = false;
    if (inputHandler.isKeyJustPressed(keyBindings[Action::RotateRight])) {
        if (!rotationKeyHandled) {
            currentShape.rotateClockwise(board.getGrid(), board.getCols(), board.getRows());
            rotationKeyHandled = true;
        }
    } else {
        rotationKeyHandled = false;
    }

    if (inputHandler.isKeyJustPressed(keyBindings[Action::RotateLeft])) {
        currentShape.rotateCounterClockwise(board.getGrid(), board.getCols(), board.getRows());
    }

    if (inputHandler.isKeyPressed(keyBindings[Action::SoftDrop]) && currentTime - lastDownMoveTime >= downMoveDelay) {
        if (!board.isOccupied(currentShape.getCoords(), 0, 1)) {
            currentShape.moveDown();
            SoundManager::PlayMoveSound();
            updateScore(0, 1, false);
            lastDownMoveTime = currentTime;
        }
    }

    if (inputHandler.isKeyJustPressed(keyBindings[Action::HardDrop])) {
        int dropDistance = 0;
        while (!board.isOccupied(currentShape.getCoords(), 0, 1)) {
            currentShape.moveDown();
            dropDistance++;
        }
        board.placeShape(currentShape);
        SoundManager::PlayDropSound();
        int clearedLines = board.clearFullLines();
        updateScore(clearedLines, dropDistance, true);
        spawnNewShape();
        inputHandler.clearKeyState(SDLK_SPACE);
    }

    if (currentScreen == Screen::Settings) {
        if (inputHandler.isKeyJustPressed(SDLK_ESCAPE)) {
            currentScreen = Screen::Main;
            inputHandler.clearKeyState(SDLK_ESCAPE);
        }
        return;
    }

    if (mouseControlEnabled && inputHandler.isMouseClicked()) {
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
        SoundManager::PlayDropSound();
        int clearedLines = board.clearFullLines();
        updateScore(clearedLines, dropDistance, true);
        spawnNewShape();
    }

    if (inputHandler.isKeyJustPressed(keyBindings[Action::Hold])) {
        holdPiece();
        SoundManager::PlayHoldSound();
    }
}



void Game::update() {
    bool gameOver = isGameOver();

    if (resumeCountdownActive || isPaused || gameOver) {
        if (gameOver) {
            if (isMusicPlaying && !gameOverMusicPlayed) {
                SoundManager::PauseBackgroundMusic();
                SoundManager::PlayGameOverMusic();
                isMusicPlaying = false;
                gameOverMusicPlayed = true;
            }
        } else {
            if (isMusicPlaying) {
                SoundManager::PauseBackgroundMusic();
                isMusicPlaying = false;
            }
        }
    
        if (resumeCountdownActive) {
            Uint32 now = SDL_GetTicks();
            Uint32 elapsed = now - countdownStartTime;
            if (elapsed >= 3000) {
                resumeCountdownActive = false;
            }
        }
        return;
    }

    if (!isMusicPlaying) {
        SoundManager::ResumeBackgroundMusic();
        isMusicPlaying = true;
    }

    Uint32 currentTime = SDL_GetTicks();

    if (board.isClearingLines) {
        if (currentTime - board.clearStartTime >= 500) {
            board.finalizeLineClear();
        }
    }

    if (currentTime - lastMoveTime >= static_cast<Uint32>(speed)) {
        if (!board.isOccupied(currentShape.getCoords(), 0, 1)) {
            currentShape.moveDown();
            SoundManager::PlayMoveSound();
        } else {
            board.placeShape(currentShape);
            SoundManager::PlayDropSound();
            int clearedLines = board.clearFullLines();
            updateScore(clearedLines, 0, false);

            if (clearedLines > 0) {
                board.clearStartTime = currentTime;
            }

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
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    board.draw(renderer, 200, 10, !resumeCountdownActive);
    if (!resumeCountdownActive && !isGameOver()) {
        shadowShape.draw(renderer, board.getCellSize(), 200, 10, true);
        currentShape.draw(renderer, board.getCellSize(), 200, 10);
    }
    renderNextPieces();
    renderHoldPiece();

    if (isGameOver()) {
        renderGameOverScreen();
        gameOverNewGameBtn->visible = true;
        gameOverQuitBtn->visible    = true;
    } else {
        gameOverNewGameBtn->visible = false;
        gameOverQuitBtn->visible    = false;
    }
    
    if (isPaused) {
        renderPauseMenu();
    }
    if (currentScreen == Screen::Settings) {
        renderSettingsScreen();
    }

    if (!isPaused && currentScreen != Screen::Settings && !isGameOver()) {
        SDL_Color textColor = {255, 255, 255, 255};
        renderText("Score: " + std::to_string(score), 10, 250, textColor);
        renderText("Level: " + std::to_string(level), 10, 300, textColor);
        renderText("Lines: " + std::to_string(totalLinesCleared), 10, 350, textColor);
    }

    if (currentScreen == Screen::Settings) {
        mouseControlCheckbox->visible = true;
        for (auto& label : controlLabels) label->visible = true;
        for (auto& button : controlButtons) button->visible = true;
        resetControlsBtn->visible = true;
        resumeBtn->visible    = false;
        newGameBtn->visible   = false;
        quitBtn->visible      = false;
        settingsBtn->visible  = false;
    } 
    else if (isPaused) {
        resumeBtn->visible    = true;
        newGameBtn->visible   = true;
        quitBtn->visible      = true;
        settingsBtn->visible  = true;
        for (auto& label : controlLabels) label->visible = false;
        for (auto& button : controlButtons) button->visible = false;
        mouseControlCheckbox->visible = false;
    } 
    else {
        resumeBtn->visible    = false;
        newGameBtn->visible   = false;
        quitBtn->visible      = false;
        settingsBtn->visible  = false;
        for (auto& label : controlLabels) label->visible = false;
        for (auto& button : controlButtons) button->visible = false;
        mouseControlCheckbox->visible = false;
        resetControlsBtn->visible = false;
    }

    FormUI::Render(renderer);

    if (resumeCountdownActive) {
        Uint32 now = SDL_GetTicks();
        Uint32 elapsed = now - countdownStartTime;
        int countdownValue = 3 - (elapsed / 1000);

        if (countdownValue > 0) {
            SDL_Color white = {255, 255, 255, 255};
            renderText(std::to_string(countdownValue),
                       windowWidth / 2 - 40,
                       windowHeight / 2 - 40,
                       white);
        }
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

    SDL_Color textColor = {255, 255, 255, 255};
    renderText("NEXT", sidebarX + 20, sidebarY - 30, textColor);

    if (resumeCountdownActive) return;

    int previewCellSize = cellSize * 0.75;
    int spacing = 20;
    int slotHeight = 100;

    for (size_t i = 0; i < std::min(nextPieces.size(), size_t(3)); i++) {
        const auto& shape = nextPieces[i];
        auto localCoords = shape.getLocalCoords();
        SDL_Color color = shape.getColor();

        int minX = 0, maxX = 0;
        int minY = 0, maxY = 0;
        for (const auto& coord : localCoords) {
            minX = std::min(minX, coord.first);
            maxX = std::max(maxX, coord.first);
            minY = std::min(minY, coord.second);
            maxY = std::max(maxY, coord.second);
        }

        int shapePixelWidth = (maxX - minX + 1) * previewCellSize;
        int shapePixelHeight = (maxY - minY + 1) * previewCellSize;

        int drawX = sidebarX + (150 - shapePixelWidth) / 2;
        int drawY = sidebarY + spacing + i * (slotHeight + spacing) + (slotHeight - shapePixelHeight) / 2;

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        for (const auto& coord : localCoords) {
            int x = drawX + (coord.first - minX) * previewCellSize;
            int y = drawY + (coord.second - minY) * previewCellSize;
            SDL_Rect rect = {x + 1, y + 1, previewCellSize - 1, previewCellSize - 1};
            SDL_RenderFillRect(renderer, &rect);
        }
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
        SoundManager::PlayClearSound();
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

    Shape::Type currentType = currentShape.getType();
    SDL_Color currentColor = currentShape.getColor();

    if (heldShape.has_value()) {
        Shape::Type heldType = heldShape->getType();
        SDL_Color heldColor = heldShape->getColor();

        heldShape = Shape(currentType, 0, 0, currentColor);
        currentShape = Shape(heldType, board.getCols() / 2, 0, heldColor);
    } else {
        heldShape = Shape(currentType, 0, 0, currentColor);
        spawnNewShape();
    }

    canHold = false;
}

void Game::renderHoldPiece() {
    
    int holdBoxX = 20;
    int holdBoxY = 70;
    int holdBoxWidth = 120;
    int holdBoxHeight = 120;

    if (!resumeCountdownActive && heldShape.has_value()) {
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
    resumeCountdownActive = true;
    countdownStartTime = SDL_GetTicks();
    SoundManager::StopGameOverMusic();
    gameOverMusicPlayed = false;
    SoundManager::RestartBackgroundMusic();
    isMusicPlaying = true;
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
}

void Game::renderSettingsScreen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255, 255};
    renderText("SETTINGS", windowWidth / 2 - 60, 50, white);
}