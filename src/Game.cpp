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
      currentScreen(Screen::Main),
      isMusicPlaying(false),
      gameStartTime(0),
      startGameTimerAfterCountdown(true),
      mouseControlEnabled(true)
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

    if (soundEnabled) SoundManager::Load();

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

    backgroundTexture = IMG_LoadTexture(renderer, "assets/background.png");
    if (!backgroundTexture) {
        throw std::runtime_error("Failed to load background image");
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

    soundEnabled = false;
    lastSoundEnabled = soundEnabled;
    soundCheckbox = FormUI::Checkbox(
        "Enable Sound",
        windowWidth / 2 - 150,
        200,
        300,
        30,
        &soundEnabled,
        smallFont
    );
    soundCheckbox->visible = false;

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

    FormUI::Layout layout(windowWidth / 2 - 150, 250, 10);

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
        windowWidth / 2 + 50,
        windowHeight - 300,
        150,
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

    doneBtn = FormUI::Button(
        "Done",
        windowWidth / 2 - 100,
        windowHeight - 100,
        200,
        40,
        [this]() { 
            currentScreen = Screen::Main;
            resetControlsBtn->visible = false;
            doneBtn->visible = false;
        },
        smallFont
    );
    doneBtn->visible = false;

    spawnNewShape();
    resumeCountdownActive = true;
    countdownStartTime = SDL_GetTicks();
}

Game::~Game() {
    if (backgroundTexture) {
        SDL_DestroyTexture(backgroundTexture);
        backgroundTexture = nullptr;
    }
    if (font) {
        TTF_CloseFont(font);
    }
    if (soundEnabled) SoundManager::CleanUp();
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
    if (soundEnabled != lastSoundEnabled) {
        if (soundEnabled) {
            SoundManager::Load();
            SoundManager::RestartBackgroundMusic();
            isMusicPlaying = true;
        } else {
            SoundManager::PauseBackgroundMusic();
            isMusicPlaying = false;
        }
        lastSoundEnabled = soundEnabled;
    }
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

            totalPausedTime += SDL_GetTicks() - pauseStartTime;
            resumeCountdownActive = true;
            countdownStartTime = SDL_GetTicks();
    
            if (!isMusicPlaying) {
                if (soundEnabled) SoundManager::ResumeBackgroundMusic();
                isMusicPlaying = true;
            }
        }
        return;
    }

    if (inputHandler.isKeyJustPressed(SDLK_ESCAPE)) {
        isPaused = true;
        pauseStartTime = SDL_GetTicks();
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
                if (soundEnabled) SoundManager::PlayMoveSound();
            }
            leftKeyHandled = true;
            leftLastMoveTime = currentTime;
            leftFirstRepeat = true;
        } else {
            if (leftFirstRepeat && currentTime - leftLastMoveTime >= autoRepeatInitialDelay) {
                if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                    currentShape.moveLeft();
                    if (soundEnabled) SoundManager::PlayMoveSound();
                }
                leftLastMoveTime = currentTime;
                leftFirstRepeat = false;
            } else if (!leftFirstRepeat && currentTime - leftLastMoveTime >= autoRepeatInterval) {
                if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                    currentShape.moveLeft();
                    if (soundEnabled) SoundManager::PlayMoveSound();
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
                if (soundEnabled) SoundManager::PlayMoveSound();
            }
            rightKeyHandled = true;
            rightLastMoveTime = currentTime;
            rightFirstRepeat = true;
        } else {
            if (rightFirstRepeat && currentTime - rightLastMoveTime >= autoRepeatInitialDelay) {
                if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                    currentShape.moveRight(board.getCols());
                    if (soundEnabled) SoundManager::PlayMoveSound();
                }
                rightLastMoveTime = currentTime;
                rightFirstRepeat = false;
            } else if (!rightFirstRepeat && currentTime - rightLastMoveTime >= autoRepeatInterval) {
                if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                    currentShape.moveRight(board.getCols());
                    if (soundEnabled) SoundManager::PlayMoveSound();
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
        if (currentShape.getType() == Shape::Type::O) {
            targetGridX -= 1;
        }
        targetGridX = std::clamp(targetGridX, 0, board.getCols() - 1);

        int currentX = currentShape.getCoords()[0].first;
        if (targetGridX > currentX) {
            if (!board.isOccupied(currentShape.getCoords(), 1, 0)) {
                currentShape.moveRight(board.getCols());
                if (soundEnabled) SoundManager::PlayMoveSound();
            }
        } else if (targetGridX < currentX) {
            if (!board.isOccupied(currentShape.getCoords(), -1, 0)) {
                currentShape.moveLeft();
                if (soundEnabled) SoundManager::PlayMoveSound();
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
            if (soundEnabled) SoundManager::PlayMoveSound();
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
        if (soundEnabled) SoundManager::PlayDropSound();
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
        if (soundEnabled) SoundManager::PlayDropSound();
        int clearedLines = board.clearFullLines();
        updateScore(clearedLines, dropDistance, true);
        spawnNewShape();
    }

    if (inputHandler.isKeyJustPressed(keyBindings[Action::Hold])) {
        holdPiece();
        if (soundEnabled) SoundManager::PlayHoldSound();
    }
}



void Game::update() {
    bool gameOver = isGameOver();

    if (resumeCountdownActive || isPaused || gameOver) {
        if (gameOver) {
            if (isMusicPlaying && !gameOverMusicPlayed) {
                if (soundEnabled) SoundManager::PauseBackgroundMusic();
                if (soundEnabled) SoundManager::PlayGameOverMusic();
                isMusicPlaying = false;
                gameOverMusicPlayed = true;
            }
        } else {
            if (isMusicPlaying) {
                if (soundEnabled) SoundManager::PauseBackgroundMusic();
                isMusicPlaying = false;
            }
        }

        if (resumeCountdownActive) {
            Uint32 now = SDL_GetTicks();
            Uint32 elapsed = now - countdownStartTime;
            if (elapsed >= 3000) {
                resumeCountdownActive = false;
                if (!startGameTimerAfterCountdown) {
                    totalPausedTime += elapsed;
                }
                if (startGameTimerAfterCountdown) {
                    gameStartTime = now;
                    startGameTimerAfterCountdown = false;
                }
            }
            return;
        }
        return;
    }

    if (!isMusicPlaying) {
        if (soundEnabled) SoundManager::ResumeBackgroundMusic();
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
            if (soundEnabled) SoundManager::PlayMoveSound();
        } else {
            board.placeShape(currentShape);
            if (soundEnabled) SoundManager::PlayDropSound();
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

    if (backgroundTexture) {
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
    }

    if (isPaused || currentScreen == Screen::Settings) {
        board.draw(renderer, 200, 10, false);
    } else {
        board.draw(renderer, 200, 10, !resumeCountdownActive);
        if (!resumeCountdownActive && !isGameOver()) {
            shadowShape.draw(renderer, board.getCellSize(), 200, 10, true);
            currentShape.draw(renderer, board.getCellSize(), 200, 10);
        }
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

    const int cardWidth = 150;
    const int cardHeight = 80;
    const int cardMargin = 10;
    const int cardsStartY = 550;
    const int cornerRadius = 8;
    
    const int cardsX = 20;
    const int scoreCardY = cardsStartY;
    const int levelCardY = cardsStartY + cardHeight + cardMargin;
    const int linesCardY = cardsStartY + 2*(cardHeight + cardMargin);
    
    renderInfoCard(cardsX, scoreCardY, cardWidth, cardHeight, cornerRadius,
                    "SCORE", std::to_string(score));
    
    renderInfoCard(cardsX, levelCardY, cardWidth, cardHeight, cornerRadius,
                    "LEVEL", std::to_string(level));
    
    renderInfoCard(cardsX, linesCardY, cardWidth, cardHeight, cornerRadius,
                    "LINES", std::to_string(totalLinesCleared));

    if (currentScreen == Screen::Settings) {
        mouseControlCheckbox->visible = true;
        soundCheckbox->visible = true;
        for (auto& label : controlLabels) label->visible = true;
        for (auto& button : controlButtons) button->visible = true;
        resetControlsBtn->visible = true;
        doneBtn->visible = true;
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
        soundCheckbox->visible = false;
    } 
    else {
        resumeBtn->visible    = false;
        newGameBtn->visible   = false;
        quitBtn->visible      = false;
        settingsBtn->visible  = false;
        for (auto& label : controlLabels) label->visible = false;
        for (auto& button : controlButtons) button->visible = false;
        mouseControlCheckbox->visible = false;
        soundCheckbox->visible = false;
        resetControlsBtn->visible = false;
        doneBtn->visible = false;
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

    if ( (!isPaused && !resumeCountdownActive && !isGameOver() && currentScreen == Screen::Main) || resumeCountdownActive ) {
        Uint32 ms = getElapsedGameTime();
        int seconds = (ms / 1000) % 60;
        int minutes = (ms / 1000) / 60;
        char buffer[32];
        sprintf(buffer, "Time: %02d:%02d", minutes, seconds);

        int textX = windowWidth - 220;
        int textY = windowHeight - 40;
        SDL_Color textColor = {255, 255, 255, 255};
        renderText(buffer, textX, textY, textColor);
    }

    SDL_RenderPresent(renderer);
}


bool Game::isGameOver() const {
    return board.isOccupied(currentShape.getCoords(), 0, 0);
}

void Game::autoRotateCurrentShape(int targetGridX) {
    (void)targetGridX;

    int bestScore = std::numeric_limits<int>::min();
    Shape bestOrientation = currentShape;
    Shape original = currentShape;

    for (int rotation = 0; rotation < 4; ++rotation) {
        Shape candidate = original;

        for (int r = 0; r < rotation; r++) {
            candidate.rotateClockwise(board.getGrid(), board.getCols(), board.getRows());
        }

        Board tempBoard = board;
        Shape dropped = candidate;
        
        while (!tempBoard.isOccupied(dropped.getCoords(), 0, 1)) {
            dropped.moveDown();
        }
        
        tempBoard.placeShape(dropped);
        int linesCleared = tempBoard.clearFullLines();

        const int cols = tempBoard.getCols();
        const int rows = tempBoard.getRows();
        const auto& grid = tempBoard.getGrid();
        
        int aggregateHeight = 0;
        int holes = 0;
        int bumpiness = 0;
        int potentialHoles = 0;
        std::vector<int> heights(cols, 0);
        std::vector<int> maxHeights(cols, 0);

        for (int col = 0; col < cols; col++) {
            int columnHeight = 0;
            for (int row = 0; row < rows; row++) {
                if (grid[row][col] != 0) {
                    columnHeight = rows - row;
                    break;
                }
            }
            heights[col] = columnHeight;
            aggregateHeight += columnHeight;
            maxHeights[col] = columnHeight;
        }

        for (int col = 0; col < cols; col++) {
            bool foundBlock = false;
            for (int row = 0; row < rows; row++) {
                if (grid[row][col] != 0) {
                    foundBlock = true;
                } else if (foundBlock) {
                    holes++;
                }
                
                if (row < rows - 1 && grid[row][col] == 0) {
                    bool covered = false;
                    if ((col > 0 && maxHeights[col-1] > rows - row) ||
                        (col < cols - 1 && maxHeights[col+1] > rows - row)) {
                        covered = true;
                    }
                    if (covered) potentialHoles++;
                }
            }
        }

        for (int i = 0; i < cols - 1; i++) {
            bumpiness += std::abs(heights[i] - heights[i + 1]);
        }

        int contacts = countContactSegments(dropped, tempBoard);

        // Adjustable weights
        int score =
            linesCleared     * 10000 +
            aggregateHeight  *   -7  +
            holes            * -100  +
            potentialHoles   *  -25  +
            bumpiness        *   -3  +
            contacts         *   500;

        if (score > bestScore) {
            bestScore = score;
            bestOrientation = candidate;
        }
    }

    currentShape = bestOrientation;
}


void Game::renderNextPieces() {
    const int sidebarX = board.getCols() * cellSize + 300;
    const int sidebarY = 70;
    const int sidebarWidth = 150;
    const int sidebarHeight = 400;
    const int cornerRadius = 10;
    const int margin = 5;
    const int titleAreaHeight = 40;
    
    drawRoundedRect(renderer, sidebarX, sidebarY, sidebarWidth, sidebarHeight, 
                   cornerRadius, {255, 255, 255, 255}, 255, true);
    
    SDL_Rect innerRect = {
        sidebarX + margin,
        sidebarY + margin + titleAreaHeight,
        sidebarWidth - 2 * margin,
        sidebarHeight - 2 * margin - titleAreaHeight
    };
    drawRoundedRect(renderer, innerRect.x, innerRect.y, innerRect.w, innerRect.h, 
                   cornerRadius - 1, {20, 25, 51, 255}, 255, true);

    SDL_Color titleColor = {20, 25, 51, 255};
    TTF_Font* titleFont = TTF_OpenFont("fonts/DejaVuSans-Bold.ttf", 24);
    if (!titleFont) titleFont = font;
    
    SDL_Surface* textSurface = TTF_RenderText_Blended(titleFont, "NEXT", titleColor);
    if (textSurface) {
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture) {
            int textX = sidebarX + (sidebarWidth - textSurface->w) / 2;
            int textY = sidebarY + (titleAreaHeight - textSurface->h) / 2;
            SDL_Rect textRect = {textX, textY, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    if (titleFont != font) {
        TTF_CloseFont(titleFont);
    }

    bool showNextPieces = (!resumeCountdownActive && !isPaused && currentScreen != Screen::Settings && !isGameOver());
    if (!showNextPieces) return;

    int previewCellSize = cellSize * 0.75;
    int spacing = 20;
    int slotHeight = 80;

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

        int drawX = sidebarX + margin + (innerRect.w - shapePixelWidth) / 2;
        int drawY = innerRect.y + spacing + i * (slotHeight + spacing) + 
                   (slotHeight - shapePixelHeight) / 2;

        const int gap = 1;
        const int previewCellDrawSize = previewCellSize - 2 * gap;
        const int radius = 1;

        for (const auto& coord : localCoords) {
            int x = drawX + (coord.first - minX) * previewCellSize + gap;
            int y = drawY + (coord.second - minY) * previewCellSize + gap;
            drawRoundedRect(renderer, 
                           x, y,
                           previewCellDrawSize, previewCellDrawSize,
                           radius,
                           color,
                           color.a,
                           true);
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
        if (soundEnabled) SoundManager::PlayClearSound();
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
    const int holdBoxX = 20;
    const int holdBoxY = 70;
    const int holdBoxWidth = 150;
    const int holdBoxHeight = 180;
    const int cornerRadius = 10;
    const int margin = 5;
    const int titleAreaHeight = 40;

    drawRoundedRect(renderer, holdBoxX, holdBoxY, holdBoxWidth, holdBoxHeight, 
                   cornerRadius, {255, 255, 255, 255}, 255, true);
    
    SDL_Rect innerRect = {
        holdBoxX + margin,
        holdBoxY + margin + titleAreaHeight,
        holdBoxWidth - 2 * margin,
        holdBoxHeight - 2 * margin - titleAreaHeight
    };
    drawRoundedRect(renderer, innerRect.x, innerRect.y, innerRect.w, innerRect.h, 
                   cornerRadius - 1, {20, 25, 51, 255}, 255, true);

    SDL_Color titleColor = {20, 25, 51, 255};
    TTF_Font* titleFont = TTF_OpenFont("fonts/DejaVuSans-Bold.ttf", 24);
    if (!titleFont) titleFont = font;
    
    SDL_Surface* textSurface = TTF_RenderText_Blended(titleFont, "HOLD", titleColor);
    if (textSurface) {
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture) {
            int textX = holdBoxX + (holdBoxWidth - textSurface->w) / 2;
            int textY = holdBoxY + (titleAreaHeight - textSurface->h) / 2;
            SDL_Rect textRect = {textX, textY, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    if (titleFont != font) {
        TTF_CloseFont(titleFont);
    }

    bool showHeldPiece = (!resumeCountdownActive && !isPaused && currentScreen != Screen::Settings && !isGameOver());
    if (showHeldPiece && heldShape.has_value()) {
        auto localCoords = heldShape->getLocalCoords();
        SDL_Color color = heldShape->getColor();

        int minX = 0, maxX = 0;
        int minY = 0, maxY = 0;
        for (const auto& coord : localCoords) {
            minX = std::min(minX, coord.first);
            maxX = std::max(maxX, coord.first);
            minY = std::min(minY, coord.second);
            maxY = std::max(maxY, coord.second);
        }

        int previewCellSize = cellSize * 0.75;
        int shapePixelWidth = (maxX - minX + 1) * previewCellSize;
        int shapePixelHeight = (maxY - minY + 1) * previewCellSize;

        int drawX = holdBoxX + margin + (innerRect.w - shapePixelWidth) / 2;
        int drawY = holdBoxY + margin + titleAreaHeight + 
                   (innerRect.h - shapePixelHeight) / 2;

        const int gap = 1;
        const int previewCellDrawSize = previewCellSize - 2 * gap;
        const int radius = 1;

        for (const auto& coord : localCoords) {
            int x = drawX + (coord.first - minX) * previewCellSize + gap;
            int y = drawY + (coord.second - minY) * previewCellSize + gap;
            drawRoundedRect(renderer, 
                           x, y,
                           previewCellDrawSize, previewCellDrawSize,
                           radius,
                           color,
                           color.a,
                           true);
        }
    }
}


void Game::renderGameOverScreen() {
    const int cardWidth = 400;
    const int cardHeight = 400;
    const int cardX = (windowWidth - cardWidth) / 2;
    const int cardY = (windowHeight - cardHeight) / 2;
    const int cornerRadius = 18;

    drawRoundedRect(renderer, cardX, cardY, cardWidth, cardHeight, cornerRadius, {20, 25, 51, 240}, 240);

    SDL_Color textColor = {255, 255, 255, 255};
    renderText("GAME OVER", cardX + 90, cardY + 40, textColor);

    renderText("Score:",  cardX + 60, cardY + 130, textColor);
    renderText(std::to_string(score),  cardX + 200, cardY + 130, textColor);

    renderText("Lines:",  cardX + 60, cardY + 180, textColor);
    renderText(std::to_string(totalLinesCleared), cardX + 200, cardY + 180, textColor);

    renderText("Level:",  cardX + 60, cardY + 230, textColor);
    renderText(std::to_string(level), cardX + 200, cardY + 230, textColor);

    const int buttonWidth = 180;
    const int buttonHeight = 40;
    const int buttonSpacing = 20;

    const int buttonX = cardX + (cardWidth - buttonWidth) / 2;
    const int newGameY = cardY + 280;
    const int quitY = newGameY + buttonHeight + buttonSpacing;

    gameOverNewGameBtn->bounds.x = buttonX;
    gameOverNewGameBtn->bounds.y = newGameY;
    gameOverNewGameBtn->bounds.w = buttonWidth;
    gameOverNewGameBtn->bounds.h = buttonHeight;

    gameOverQuitBtn->bounds.x = buttonX;
    gameOverQuitBtn->bounds.y = quitY;
    gameOverQuitBtn->bounds.w = buttonWidth;
    gameOverQuitBtn->bounds.h = buttonHeight;
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
    totalPausedTime = 0;
    pauseStartTime = 0;
    startGameTimerAfterCountdown = true;
    gameStartTime = 0;
    if (soundEnabled) SoundManager::StopGameOverMusic();
    gameOverMusicPlayed = false;
    if (soundEnabled) SoundManager::RestartBackgroundMusic();
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
    const int cardWidth = 400;
    const int cardHeight = 400;
    const int cardX = (windowWidth - cardWidth) / 2;
    const int cardY = (windowHeight - cardHeight) / 2;
    const int cornerRadius = 15;

    drawCardWithBorder(renderer, cardX, cardY, cardWidth, cardHeight,cornerRadius, {20, 25, 51, 180}, {255, 255, 255, 255}, 2);
    
    SDL_Color textColor = {255, 255, 255, 255};
    renderText("PAUSED", windowWidth / 2 - 60, cardY + 30, textColor);
    
    resumeBtn->bounds.x = windowWidth / 2 - 100;
    resumeBtn->bounds.y = cardY + 80;
    
    newGameBtn->bounds.x = windowWidth / 2 - 100;
    newGameBtn->bounds.y = cardY + 150;
    
    settingsBtn->bounds.x = windowWidth / 2 - 100;
    settingsBtn->bounds.y = cardY + 220;
    
    quitBtn->bounds.x = windowWidth / 2 - 100;
    quitBtn->bounds.y = cardY + 290;
}

void Game::renderSettingsScreen() {
    const int cardWidth = 500;
    const int cardHeight = 800;
    const int cardX = (windowWidth - cardWidth) / 2;
    const int cardY = (windowHeight - cardHeight) / 2;
    const int cornerRadius = 18;

    drawCardWithBorder(renderer, cardX, cardY, cardWidth, cardHeight, cornerRadius, {20, 25, 51, 230}, {255, 255, 255, 255}, 2);  
    SDL_Color white = {255, 255, 255, 255};
    renderText("SETTINGS", cardX + (cardWidth - 180) / 2, cardY + 32, white);

    mouseControlCheckbox->bounds.x = cardX + (cardWidth - mouseControlCheckbox->bounds.w) / 2;
    mouseControlCheckbox->bounds.y = cardY + 100;

    soundCheckbox->bounds.x = cardX + (cardWidth - soundCheckbox->bounds.w) / 2;
    soundCheckbox->bounds.y = cardY + 150;

    int controlStartY = cardY + 210;
    for (size_t i = 0; i < controlLabels.size(); ++i) {
        controlLabels[i]->bounds.x = cardX + 40;
        controlLabels[i]->bounds.y = controlStartY + int(i) * 40;

        controlButtons[i]->bounds.x = cardX + cardWidth - 240;
        controlButtons[i]->bounds.y = controlStartY + int(i) * 40;
    }
}


Uint32 Game::getElapsedGameTime() const {
    if (gameStartTime == 0) {
        return 0;
    }
    Uint32 now = SDL_GetTicks();
    Uint32 pausedFor = totalPausedTime;
    if (isPaused) {
        pausedFor += now - pauseStartTime;
    }
    if (resumeCountdownActive) {
        pausedFor += now - countdownStartTime;
    }
    
    return now - gameStartTime - pausedFor;
}

void Game::renderInfoCard(int x, int y, int width, int height, int radius, 
                         const std::string& title, const std::string& value) {
    const int margin = 5;
    const int titleAreaHeight = 30;

    drawRoundedRect(renderer, x, y, width, height, radius, 
                   {255, 255, 255, 255}, 255, true);
    
    SDL_Rect innerRect = {
        x + margin,
        y + margin + titleAreaHeight,
        width - 2 * margin,
        height - 2 * margin - titleAreaHeight
    };
    drawRoundedRect(renderer, innerRect.x, innerRect.y, innerRect.w, innerRect.h, 
                   radius - 2, {20, 25, 51, 255}, 255, true);

    SDL_Color titleColor = {20, 25, 51, 255};
    TTF_Font* titleFont = TTF_OpenFont("fonts/DejaVuSans-Bold.ttf", 20);
    if (!titleFont) titleFont = font;
    
    SDL_Surface* titleSurface = TTF_RenderText_Blended(titleFont, title.c_str(), titleColor);
    if (titleSurface) {
        SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
        if (titleTexture) {
            int textX = x + (width - titleSurface->w) / 2;
            int textY = y + (titleAreaHeight - titleSurface->h) / 2;
            SDL_Rect textRect = {textX, textY, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(renderer, titleTexture, nullptr, &textRect);
            SDL_DestroyTexture(titleTexture);
        }
        SDL_FreeSurface(titleSurface);
    }
    
    SDL_Color valueColor = {255, 255, 255, 255};
    TTF_Font* valueFont = TTF_OpenFont("fonts/DejaVuSans.ttf", 24);
    if (!valueFont) valueFont = font;
    
    SDL_Surface* valueSurface = TTF_RenderText_Blended(valueFont, value.c_str(), valueColor);
    if (valueSurface) {
        SDL_Texture* valueTexture = SDL_CreateTextureFromSurface(renderer, valueSurface);
        if (valueTexture) {
            int textX = innerRect.x + (innerRect.w - valueSurface->w) / 2;
            int textY = innerRect.y + (innerRect.h - valueSurface->h) / 2;
            SDL_Rect textRect = {textX, textY, valueSurface->w, valueSurface->h};
            SDL_RenderCopy(renderer, valueTexture, nullptr, &textRect);
            SDL_DestroyTexture(valueTexture);
        }
        SDL_FreeSurface(valueSurface);
    }
    
    if (titleFont != font) TTF_CloseFont(titleFont);
    if (valueFont != font) TTF_CloseFont(valueFont);
}

int Game::countContactSegments(const Shape& shape, const Board& board) {
    const auto& coords = shape.getCoords();
    const auto& grid = board.getGrid();
    int rows = board.getRows();
    int cols = board.getCols();

    int contactCount = 0;

    for (const auto& [x, y] : coords) {
        if (y == rows - 1 || (y + 1 < rows && grid[y + 1][x] != 0)) contactCount++;
        if (y == 0 || (y - 1 >= 0 && grid[y - 1][x] != 0)) contactCount++;
        if (x == 0 || (x - 1 >= 0 && grid[y][x - 1] != 0)) contactCount++;
        if (x == cols - 1 || (x + 1 < cols && grid[y][x + 1] != 0)) contactCount++;
    }

    return contactCount;
}