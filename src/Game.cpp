#include "Game.hpp"

Game::Game(int windowWidth, int windowHeight, int cellSize)
    : board(20, 10, cellSize, {0, 0, 255, 255}),
      currentShape(Shape::Type::O, board.getCols() / 2, 0, {255, 255, 255, 255}),
      shadowShape(currentShape),
      cellSize(cellSize),
      windowWidth(windowWidth),
      windowHeight(windowHeight)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        throw std::runtime_error("SDL Initialization failed");

    int imgFlags = IMG_INIT_PNG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags)
        throw std::runtime_error(std::string("IMG_Init failed: ") + IMG_GetError());

    if (TTF_Init() == -1)
        throw std::runtime_error("Failed to initialize SDL_ttf: " + std::string(TTF_GetError()));

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
        throw std::runtime_error("SDL_mixer initialization failed: " + std::string(Mix_GetError()));

    fontDefault = TTF_OpenFont("fonts/DejaVuSans.ttf", 32);
    if (!fontDefault) throw std::runtime_error("Failed to load font: " + std::string(TTF_GetError()));

    fontLarge = TTF_OpenFont("fonts/DejaVuSans-Bold.ttf", 80);
    if (!fontLarge) fontLarge = fontDefault;

    fontMedium = TTF_OpenFont("fonts/DejaVuSans-Bold.ttf", 24);
    if (!fontMedium) fontMedium = fontDefault;

    fontSmall = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 10);
    if (!fontSmall) fontSmall = fontDefault;

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

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        throw std::runtime_error("Failed to create renderer");
    }

    backgroundTexture = IMG_LoadTexture(renderer, "assets/background.png");
    if (!backgroundTexture) {
        throw std::runtime_error("Failed to load background image");
    }

    srand(time(nullptr));

    FormUI::Init(fontDefault);

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
        fontSmall
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
        fontSmall
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
    
        auto [label, button] = layout.addLabelButtonRow(labelText, keyLabel, buttonCallback, 200, 100, 30, fontSmall, fontSmall);
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
        fontSmall
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
        fontSmall
    );
    doneBtn->visible = false;

    spawnNewShape();
    resumeCountdownActive = true;
    countdownStartTime = SDL_GetTicks();
}

Game::~Game() {
    if (soundEnabled && Mix_PlayingMusic()) {
        SoundManager::StopBackgroundMusic();
        SoundManager::StopGameOverMusic();
    }

    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);

    SoundManager::CleanUp();
    Mix_CloseAudio();

    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    
    if (fontLarge && fontLarge != fontDefault) {
        TTF_CloseFont(fontLarge);
    }
    if (fontMedium && fontMedium != fontDefault) {
        TTF_CloseFont(fontMedium);
    }
    if (fontSmall && fontSmall != fontDefault) {
        TTF_CloseFont(fontSmall);
    }
    if (fontDefault) {
        TTF_CloseFont(fontDefault);
    }

    IMG_Quit();
    TTF_Quit();
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
            SoundManager::StopBackgroundMusic();
            isMusicPlaying = false;
        }
        lastSoundEnabled = soundEnabled;
    }
    inputHandler.beginFrame();

    if (board.isClearingLines) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }
        return;
    }

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

    {
        const int boardOffsetX = 200;
        const int boardOffsetY = 10;
        int mouseX = inputHandler.getMouseX() - boardOffsetX;
        int mouseY = inputHandler.getMouseY() - boardOffsetY;

        const bool insideBoard =
            mouseX >= 0 && mouseX < board.getCols()*cellSize &&
            mouseY >= 0 && mouseY < board.getRows()*cellSize;

        if (mouseControlEnabled && insideBoard) {
            int targetGridX = std::clamp(mouseX / cellSize, 0, board.getCols()-1);
            int targetGridY = std::clamp(mouseY / cellSize, 0, board.getRows()-1);

            snapShapeHorizontally(targetGridX);

            static Uint32 lastAutoPlace = 0;
            const Uint32 intervalMs = 16;
            Uint32 now = SDL_GetTicks();
            if (now - lastAutoPlace >= intervalMs) {
                autoRotateCurrentShape(targetGridX, targetGridY);
                lastAutoPlace = now;
            }
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
        board.triggerHardDropAnim(currentShape);
        if (soundEnabled) SoundManager::PlayDropSound();

        int clearedLines = board.clearFullLines();
        updateScore(clearedLines, dropDistance, true);

        Uint32 now = SDL_GetTicks();
        if (clearedLines > 0) {
            board.clearStartTime = now;
        } else {
            spawnNewShape();
            if (isGameOver()) return;
        }

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
        board.triggerHardDropAnim(currentShape);
        if (soundEnabled) SoundManager::PlayDropSound();

        int clearedLines = board.clearFullLines();
        updateScore(clearedLines, dropDistance, true);

        int basePoints = 0;
        switch (clearedLines) {
            case 1: basePoints = 40 * (level + 1); break;
            case 2: basePoints = 100 * (level + 1); break;
            case 3: basePoints = 300 * (level + 1); break;
            case 4: basePoints = 1200 * (level + 1); break;
            default: break;
        }
        if (clearedLines > 0) {
            triggerScorePopup(clearedLines, basePoints);
        }

        Uint32 now = SDL_GetTicks();
        if (clearedLines > 0) {
            board.clearStartTime = now;
        } else {
            spawnNewShape();
            if (isGameOver()) return;
        }
    }

    if (inputHandler.isKeyJustPressed(keyBindings[Action::Hold])) {
        holdPiece();
        if (soundEnabled) SoundManager::PlayHoldSound();
    }
}


void Game::update() {
    updateScorePopups();
    bool gameOver = isGameOver();

    if (resumeCountdownActive || isPaused || gameOver) {
        if (gameOver && !gameOverMusicPlayed) {
            if (soundEnabled) {
                SoundManager::StopBackgroundMusic();
                SoundManager::PlayGameOverMusic();
            }
            isMusicPlaying = false;
            gameOverMusicPlayed = true;
        } 
        else if (isMusicPlaying) {
            if (soundEnabled) SoundManager::PauseBackgroundMusic();
            isMusicPlaying = false;
        }

        if (resumeCountdownActive) {
            Uint32 now = SDL_GetTicks();
            if (now - countdownStartTime >= 3000) {
                resumeCountdownActive = false;
                if (startGameTimerAfterCountdown) {
                    gameStartTime = now;
                    startGameTimerAfterCountdown = false;
                } else {
                    totalPausedTime += now - countdownStartTime;
                }
            }
        }
        return;
    }

    if (!isMusicPlaying && soundEnabled) {
        SoundManager::ResumeBackgroundMusic();
        isMusicPlaying = true;
    }

    board.updateAnimations();


    Uint32 currentTime = SDL_GetTicks();

    if (board.isClearingLines) {
        if (currentTime - board.clearStartTime >= 500) {
            board.finalizeLineClear();
            spawnNewShape();
            if (isGameOver()) return;
        }
        return;
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

            int basePoints = 0;
            switch (clearedLines) {
                case 1: basePoints = 40 * (level + 1); break;
                case 2: basePoints = 100 * (level + 1); break;
                case 3: basePoints = 300 * (level + 1); break;
                case 4: basePoints = 1200 * (level + 1); break;
                default: break;
            }
            if (clearedLines > 0) {
                triggerScorePopup(clearedLines, basePoints);
            }

            if (clearedLines > 0) {
                board.clearStartTime = currentTime;
            } else {
                spawnNewShape();
                if (isGameOver()) return;
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
        if (!resumeCountdownActive && !isGameOver() && !board.isClearingLines) {
            shadowShape.draw(renderer, board.getCellSize(), 200, 10, true);
            currentShape.draw(renderer, board.getCellSize(), 200, 10);
        }
    }

    renderScorePopups();

    renderNextPieces();
    renderHoldPiece();

    bool settingsScreen = (currentScreen == Screen::Settings);
    bool paused = isPaused && !settingsScreen;
    bool gameOver = isGameOver();

    mouseControlCheckbox->visible = settingsScreen;
    soundCheckbox->visible = settingsScreen;
    resetControlsBtn->visible = settingsScreen;
    doneBtn->visible = settingsScreen;

    for (auto& label : controlLabels) label->visible = settingsScreen;
    for (auto& button : controlButtons) button->visible = settingsScreen;

    resumeBtn->visible = paused;
    newGameBtn->visible = paused;
    settingsBtn->visible = paused;
    quitBtn->visible = paused;

    gameOverNewGameBtn->visible = gameOver;
    gameOverQuitBtn->visible = gameOver;

    if (gameOver) {
        renderGameOverScreen();
    }

    if (paused) {
        renderPauseMenu();
    }

    if (settingsScreen) {
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
    const int linesCardY = cardsStartY + 2 * (cardHeight + cardMargin);

    renderInfoCard(cardsX, scoreCardY, cardWidth, cardHeight, cornerRadius,
                   "SCORE", std::to_string(score));
    renderInfoCard(cardsX, levelCardY, cardWidth, cardHeight, cornerRadius,
                   "LEVEL", std::to_string(level));
    renderInfoCard(cardsX, linesCardY, cardWidth, cardHeight, cornerRadius,
                   "LINES", std::to_string(totalLinesCleared));

    FormUI::Render(renderer);

    if (resumeCountdownActive) {
        Uint32 now = SDL_GetTicks();
        Uint32 elapsed = now - countdownStartTime;
        int countdownValue = 3 - static_cast<int>(elapsed / 1000);

        if (countdownValue > 0) {
            Uint32 msInSecond = elapsed % 1000;
            float scale = countdownScale(msInSecond);
            SDL_Color white = {255, 255, 255, 255};
            renderTextCenteredScaled(
                std::to_string(countdownValue),
                windowWidth / 2,
                windowHeight / 2,
                white,
                scale,
                fontLarge ? fontLarge : fontDefault
            );
        }
    }

    if ((!paused && !resumeCountdownActive && !gameOver && currentScreen == Screen::Main) || resumeCountdownActive) {
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
    if (board.isClearingLines) return false;
    return board.isOccupied(currentShape.getCoords(), 0, 0);
}


void Game::autoRotateCurrentShape(int targetGridX, int targetGridY)
{
    constexpr int CONTACT_W  = 20;
    const int ANCHOR_W   = autoPlaceAnchorW;
    constexpr int STAB_W     = 15;
    constexpr int ANCHOR_CAP = 2;
    constexpr int FILL_BONUS = 1'000'000;

    const int rows = board.getRows();
    const int cols = board.getCols();
    const auto& grid = board.getGrid();
    const Shape original = currentShape;

    static std::vector<std::vector<int>> scratch;
    scratch.assign(rows, std::vector<int>(cols, 0));
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            scratch[r][c] = grid[r][c] ? 1 : 0;

    struct RotInfo {
        Shape shape;
        int   minX, maxX, width;
        explicit RotInfo(const Shape& s) : shape(s), minX(0), maxX(0), width(0) {}
    };

    auto computeBounds = [&](const Shape& s, int& mn, int& mx) {
        mn = cols; mx = -1;
        for (const auto& c : s.getCoords()) { mn = std::min(mn, c.first); mx = std::max(mx, c.first); }
    };

    std::vector<RotInfo> rots; rots.reserve(4);

    RotInfo r0(original);
    computeBounds(r0.shape, r0.minX, r0.maxX);
    r0.width = r0.maxX - r0.minX + 1;
    rots.push_back(std::move(r0));

    for (int i = 1; i < 4; ++i) {
        RotInfo ri(rots.back().shape);
        ri.shape.rotateClockwise(grid, cols, rows);
        computeBounds(ri.shape, ri.minX, ri.maxX);
        ri.width = ri.maxX - ri.minX + 1;
        rots.push_back(std::move(ri));
    }

    int bestScore = std::numeric_limits<int>::min();
    int bestTie   = std::numeric_limits<int>::max();
    Shape best    = currentShape;
    bool foundAny = false;

    auto evalRange = [&](const RotInfo& base, int startX, int endX) {
        bool localFound = false;
        for (int xLeft = startX; xLeft <= endX; ++xLeft)
        {
            Shape cand = base.shape;
            int dxShift = xLeft - base.minX;
            for (auto& p : cand.coords) p.first += dxShift;
            if (board.isOccupied(cand.getCoords(), 0, 0)) continue;

            Shape dropped = cand;
            while (!board.isOccupied(dropped.getCoords(), 0, 1))
                dropped.moveDown();

            for (const auto& p : dropped.getCoords())
                if (p.second >= 0 && p.second < rows && p.first >= 0 && p.first < cols)
                    scratch[p.second][p.first] = 1;

            int cleared = 0;
            for (int r = 0; r < rows; ++r) {
                bool full = true;
                for (int c = 0; c < cols; ++c) { if (!scratch[r][c]) { full = false; break; } }
                if (full) ++cleared;
            }

            int aggregate = 0, holes = 0, bumpiness = 0;
            std::vector<int> heights(cols, 0);
            for (int c = 0; c < cols; ++c) {
                for (int r = 0; r < rows; ++r) {
                    if (scratch[r][c]) { heights[c] = rows - r; aggregate += heights[c]; break; }
                }
            }
            for (int c = 0; c < cols; ++c) {
                bool seen = false;
                for (int r = 0; r < rows; ++r) { if (scratch[r][c]) seen = true; else if (seen) ++holes; }
            }
            for (int c = 0; c < cols - 1; ++c) bumpiness += std::abs(heights[c] - heights[c+1]);

            int contacts = countContactSegments(dropped, board);

            int fmin = cols, fmax = -1, centreX = 0, minY = rows;
            for (const auto& p : dropped.getCoords()) {
                fmin = std::min(fmin, p.first);
                fmax = std::max(fmax, p.first);
                centreX += p.first;
                minY = std::min(minY, p.second);
            }
            centreX /= (int)dropped.getCoords().size();

            int rawDist = 0;
            if (targetGridX < fmin) rawDist = fmin - targetGridX;
            else if (targetGridX > fmax) rawDist = targetGridX - fmax;
            int anchorDist = std::max(0, rawDist - 1);
            anchorDist     = std::min(anchorDist, ANCHOR_CAP);
            int anchorPen  = -ANCHOR_W * anchorDist * anchorDist;

            int dxPivot = std::abs(cand.coords[0].first - original.coords[0].first);

            bool fillsTarget = false;
            if (targetGridY >= 0 && targetGridY < rows)
                for (const auto& p : dropped.getCoords())
                    if (p.first == targetGridX && p.second == targetGridY) { fillsTarget = true; break; }

            int yAlignBonus = 0;
            if (targetGridY >= 0 && targetGridY < rows)
                yAlignBonus = -std::abs(minY - targetGridY) * 5;

            int score =
                  cleared      * 1000
                + aggregate    *   -7
                + holes        * -120
                + bumpiness    *   -4
                + contacts     *  CONTACT_W
                + anchorPen
                + dxPivot      *  -STAB_W
                + (fillsTarget ? FILL_BONUS : 0)
                + yAlignBonus;

            int tie = std::max(0, std::abs(centreX - targetGridX) - 1);

            if (score > bestScore || (score == bestScore && tie < bestTie)) {
                bestScore = score; bestTie = tie; best = cand; localFound = true;
            }

            for (const auto& p : dropped.getCoords())
                if (p.second >= 0 && p.second < rows && p.first >= 0 && p.first < cols)
                    scratch[p.second][p.first] = grid[p.second][p.first] ? 1 : 0;
        }
        return localFound;
    };

    for (const auto& base : rots)
    {
        int W = autoPlaceWindow;
        int startX = std::max(0, targetGridX - W);
        int endX   = std::min(cols - base.width, targetGridX + W);
        if (startX <= endX) foundAny |= evalRange(base, startX, endX);
        if (!foundAny && cols - base.width >= 0) foundAny |= evalRange(base, 0, cols - base.width);
    }

    currentShape = best;
}


void Game::snapShapeHorizontally(int targetX)
{
    int minX = INT_MAX, maxX = INT_MIN;
    for (auto &c : currentShape.coords) {
        minX = std::min(minX, c.first);
        maxX = std::max(maxX, c.first);
    }

    const int leftBias = (currentShape.getType() == Shape::Type::O) ? 1 : 0;
    int desiredMinX = std::clamp(targetX - leftBias, 0, board.getCols() - (maxX - minX + 1));
    int dx = desiredMinX - minX;

    if (std::abs(dx) <= mouseMagnetRadius) return;

    mouseXAccumulator += dx * mouseFollowStrength;

    int steps = static_cast<int>(std::floor(std::abs(mouseXAccumulator)));
    if (steps <= 0) return;

    int dir = (mouseXAccumulator > 0.0f) ? 1 : -1;
    for (int i = 0; i < steps; ++i) {
        if (!board.isOccupied(currentShape.getCoords(), dir, 0)) {
            for (auto &c : currentShape.coords) c.first += dir;
            mouseXAccumulator -= dir;
        } else {
            mouseXAccumulator = 0.0f;
            break;
        }
    }
}



void Game::renderNextPieces() {
    const int sidebarX = board.getCols() * cellSize + 300;
    const int sidebarY = 70;
    const int sidebarWidth = 150;
    const int sidebarHeight = 400;
    const int cornerRadius = 10;
    const int margin = 5;
    const int titleAreaHeight = 40;
    
    draw_smooth_rounded_rect(renderer, sidebarX, sidebarY, sidebarWidth, sidebarHeight, 
                   cornerRadius, {255, 255, 255, 255}, true);
    
    SDL_Rect innerRect = {
        sidebarX + margin,
        sidebarY + margin + titleAreaHeight,
        sidebarWidth - 2 * margin,
        sidebarHeight - 2 * margin - titleAreaHeight
    };
    draw_smooth_rounded_rect(renderer, innerRect.x, innerRect.y, innerRect.w, innerRect.h, 
                   cornerRadius - 1, {20, 25, 51, 255}, true);

    SDL_Color titleColor = {20, 25, 51, 255};
    
    SDL_Surface* textSurface = TTF_RenderText_Blended(fontMedium, "NEXT", titleColor);
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

    bool showNextPieces = (!resumeCountdownActive && !isPaused && currentScreen != Screen::Settings && !isGameOver());
    if (!showNextPieces) return;

    int previewCellSize = cellSize * 0.75;
    int spacing = 20;
    int slotHeight = 80;

    for (size_t i = 0; i < std::min(nextPieces.size(), size_t(3)); i++) {
        const auto& shape = nextPieces[i];
        tmpCoords.clear();
        shape.getLocalCoords(tmpCoords);
        SDL_Color color = shape.getColor();

        int minX = 0, maxX = 0;
        int minY = 0, maxY = 0;
        for (const auto& coord : tmpCoords) {
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

        for (const auto& coord : tmpCoords) {
            int x = drawX + (coord.first - minX) * previewCellSize + gap;
            int y = drawY + (coord.second - minY) * previewCellSize + gap;
            draw_preview_block(renderer, x, y,
                       previewCellDrawSize, previewCellDrawSize,
                       color);
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
        return;
    }

    Shape::Type newType = static_cast<Shape::Type>(rand() % 7);
    nextPieces.push_back(Shape(newType, board.getCols() / 2, 0, {255, 255, 255, 255}));

    canHold = true;
    lastMouseTargetGridX = std::numeric_limits<int>::min();
}



void Game::checkLevelUp() {
    int newLevel = (totalLinesCleared / 10) + 1;

    if (newLevel > level) {
        level = newLevel;
        updateSpeed();
        triggerLevelUpPopup();
    }
}

void Game::updateScore(int clearedLines,int dropDistance, bool hardDrop)
{
    if (clearedLines > 0) {
        totalLinesCleared += clearedLines;
        if (soundEnabled) SoundManager::PlayClearSound();
    }

    int points = 0;
    switch (clearedLines) {
        case 1: points += 40;   break;
        case 2: points += 100;  break;
        case 3: points += 300;  break;
        case 4: points += 1200; break;
    }
    points *= (level + 1);
    points += dropDistance * (hardDrop ? 2 : 1);
    score  += points;

    if (clearedLines > 0) {
        triggerScorePopup(clearedLines, points);
    }

    checkLevelUp();
}


void Game::renderText(const std::string& text, int x, int y, SDL_Color color) {
    if (!fontDefault) {
        std::cerr << "Font not initialized!" << std::endl;
        return;
    }

    if (text.empty()) {
        std::cerr << "Skipping rendering empty text" << std::endl;
        return;
    }

    SDL_Surface* textSurface = TTF_RenderText_Blended(fontDefault, text.c_str(), color);
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

    draw_smooth_rounded_rect(renderer, holdBoxX, holdBoxY, holdBoxWidth, holdBoxHeight, 
                   cornerRadius, {255, 255, 255, 255}, true);
    
    SDL_Rect innerRect = {
        holdBoxX + margin,
        holdBoxY + margin + titleAreaHeight,
        holdBoxWidth - 2 * margin,
        holdBoxHeight - 2 * margin - titleAreaHeight
    };
    draw_smooth_rounded_rect(renderer, innerRect.x, innerRect.y, innerRect.w, innerRect.h, 
                   cornerRadius - 1, {20, 25, 51, 255}, true);

    SDL_Color titleColor = {20, 25, 51, 255};
    SDL_Surface* textSurface = TTF_RenderText_Blended(fontMedium, "HOLD", titleColor);
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

    bool showHeldPiece = (!resumeCountdownActive && !isPaused && currentScreen != Screen::Settings && !isGameOver());
    if (showHeldPiece && heldShape.has_value()) {
        tmpCoords.clear();
        heldShape->getLocalCoords(tmpCoords);
        SDL_Color color = heldShape->getColor();

        int minX = 0, maxX = 0;
        int minY = 0, maxY = 0;
        for (const auto& coord : tmpCoords) {
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

        for (const auto& coord : tmpCoords) {
            int x = drawX + (coord.first - minX) * previewCellSize + gap;
            int y = drawY + (coord.second - minY) * previewCellSize + gap;
            draw_preview_block(renderer, x, y,
                       previewCellDrawSize, previewCellDrawSize,
                       color);
        }
    }
}



void Game::renderGameOverScreen() {
    const int cardWidth = 400;
    const int cardHeight = 400;
    const int cardX = (windowWidth - cardWidth) / 2;
    const int cardY = (windowHeight - cardHeight) / 2;
    const int cornerRadius = 18;

    draw_smooth_rounded_rect(renderer, cardX, cardY, cardWidth, cardHeight, cornerRadius, {20, 25, 51, 240}, true);

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
    if (soundEnabled) {
        SoundManager::StopBackgroundMusic();
        SoundManager::StopGameOverMusic();
    }

    board.clearBoard();
    score = totalLinesCleared = 0;
    level = 1;
    nextPieces.clear();
    canHold = true;
    heldShape.reset();
    spawnNewShape();

    running = true;
    ignoreNextMouseClick = true;
    resumeCountdownActive = true;
    countdownStartTime = SDL_GetTicks();
    totalPausedTime = pauseStartTime = 0;
    startGameTimerAfterCountdown = true;
    gameStartTime = 0;
    gameOverMusicPlayed = false;

    if (soundEnabled) {
        SoundManager::RestartBackgroundMusic();
        isMusicPlaying = true;
    } else {
        isMusicPlaying = false;
    }
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

    draw_smooth_rounded_rect(renderer, x, y, width, height, radius, 
                   {255, 255, 255, 255}, true);

    SDL_Rect innerRect = {
        x + margin,
        y + margin + titleAreaHeight,
        width - 2 * margin,
        height - 2 * margin - titleAreaHeight
    };
    draw_smooth_rounded_rect(renderer, innerRect.x, innerRect.y, innerRect.w, innerRect.h, 
                   radius - 2, {20, 25, 51, 255}, true);

    SDL_Color titleColor = {20, 25, 51, 255};
    SDL_Surface* titleSurface = TTF_RenderText_Blended(fontMedium, title.c_str(), titleColor);
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
    SDL_Surface* valueSurface = TTF_RenderText_Blended(fontDefault, value.c_str(), valueColor);
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
}


int Game::countContactSegments(const Shape& shape, const Board& board)
{
    const auto& coords = shape.getCoords();
    const auto& grid   = board.getGrid();
    const int   rows   = board.getRows();
    const int   cols   = board.getCols();

    struct PairHash {
        size_t operator()(const std::pair<int,int>& p) const noexcept
        { return (static_cast<size_t>(p.first) << 16) ^ static_cast<size_t>(p.second); }
    };
    std::unordered_set<std::pair<int,int>, PairHash> shapeCells(coords.begin(), coords.end());

    const int dx[4] = { 0,  0, -1,  1 };
    const int dy[4] = { -1, 1,  0,  0 };

    int contacts = 0;

    for (auto [x, y] : coords)
    {
        for (int k = 0; k < 4; ++k)
        {
            const int nx = x + dx[k];
            const int ny = y + dy[k];

            if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) {
                ++contacts;
                continue;
            }

            if (shapeCells.find({nx, ny}) != shapeCells.end())
                continue;

            if (grid[ny][nx] != 0)
                ++contacts;
        }
    }
    return contacts;
}


float Game::easeOutCubic(float t) {
    return 1.0f - std::pow(1.0f - t, 3.0f);
}

float Game::easeInOutQuad(float t) {
    return (t < 0.5f) ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
}

float Game::countdownScale(Uint32 msInSecond) const {
    const float growDur   = 550.0f;
    const float holdDur   = 200.0f;
    const float settleDur = 250.0f;

    const float startScale     = 0.65f;
    const float overshootScale = 1.25f;
    const float finalScale     = 1.00f;

    float t = static_cast<float>(msInSecond);

    if (t < growDur) {
        float p = t / growDur;
        return startScale + (overshootScale - startScale) * easeOutCubic(p);
    } else if (t < growDur + holdDur) {
        return overshootScale;
    } else {
        float p = (t - (growDur + holdDur)) / settleDur;
        if (p > 1.0f) p = 1.0f;
        return overshootScale + (finalScale - overshootScale) * easeInOutQuad(p);
    }
}

void Game::renderTextCenteredScaled(const std::string& text, int cx, int cy,
                                    SDL_Color color, float scale, TTF_Font* useFont)
{
    if (!useFont || text.empty()) return;

    SDL_Surface* surf = TTF_RenderText_Blended(useFont, text.c_str(), color);
    if (!surf || surf->w == 0 || surf->h == 0) {
        if (surf) SDL_FreeSurface(surf);
        return;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (!tex) {
        SDL_FreeSurface(surf);
        return;
    }

    int w = surf->w;
    int h = surf->h;

    int dstW = static_cast<int>(w * scale);
    int dstH = static_cast<int>(h * scale);
    SDL_Rect dst { cx - dstW / 2, cy - dstH / 2, dstW, dstH };

    SDL_Color shadowCol { 0, 0, 0, 160 };
    SDL_Surface* shadowSurf = TTF_RenderText_Blended(useFont, text.c_str(), shadowCol);
    if (shadowSurf) {
        SDL_Texture* shadowTex = SDL_CreateTextureFromSurface(renderer, shadowSurf);
        if (shadowTex) {
            int sw = shadowSurf->w, sh = shadowSurf->h;
            int sdw = static_cast<int>(sw * scale);
            int sdh = static_cast<int>(sh * scale);
            SDL_Rect shadowDst { cx - sdw / 2 + 4, cy - sdh / 2 + 4, sdw, sdh };
            SDL_RenderCopy(renderer, shadowTex, nullptr, &shadowDst);
            SDL_DestroyTexture(shadowTex);
        }
        SDL_FreeSurface(shadowSurf);
    }

    SDL_RenderCopy(renderer, tex, nullptr, &dst);

    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void Game::triggerScorePopup(int clearedLines, int linePoints) {
    if (clearedLines <= 0) return;

    const int boardOffsetX = 200;
    const int boardOffsetY = 10;
    float cx = boardOffsetX + board.getCols() * board.getCellSize() * 0.5f;

    float avgRow = 0.f;
    const auto& rows = board.getLinesToClear();
    if (!rows.empty()) {
        for (int r : rows) avgRow += r;
        avgRow /= rows.size();
    }
    float cy = boardOffsetY + (avgRow + 0.5f) * board.getCellSize();

    const Uint32 dur  = 800;
    const float  rise = 40.f;

    const int labelYOffset  = -12;
    const int pointsYOffset = +6;

    const SDL_Color white = {255, 255, 255, 255};

    std::string label;
    switch (clearedLines) {
        case 1: label = "Single"; break;
        case 2: label = "Double"; break;
        case 3: label = "Triple"; break;
        default: label = "Tetris"; break;
    }

    Uint32 now = SDL_GetTicks();

    scorePopups.push_back(ScorePopup{
        label, white, cx, cy + labelYOffset, rise, now, 0, dur
    });

    scorePopups.push_back(ScorePopup{
        std::string("+") + std::to_string(linePoints),
        white, cx, cy + pointsYOffset, rise, now, 60, dur
    });

}


void Game::updateScorePopups() {
    Uint32 now = SDL_GetTicks();
    scorePopups.erase(
        std::remove_if(scorePopups.begin(), scorePopups.end(),
            [now](const ScorePopup& p) {
                if (now < p.start + p.delay) return false;
                return now >= p.start + p.delay + p.duration;
            }),
        scorePopups.end()
    );
}

void Game::renderScorePopups() {
    if (scorePopups.empty()) return;

    Uint32 now = SDL_GetTicks();
    for (const auto& p : scorePopups) {
        if (now < p.start + p.delay) continue;

        float t = (now - (p.start + p.delay)) / float(p.duration);
        t = clamp01(t);

        float dy = p.rise * easeOutQuad(t);
        float y  = p.y0 - dy;

        float aIn  = clamp01(t / 0.12f);
        float aOut = (t > 0.75f) ? clamp01(1.f - (t - 0.75f) / 0.25f) : 1.f;
        Uint8 alpha = Uint8(255 * aIn * aOut);

        TTF_Font* useFont = (fontMedium ? fontMedium : fontDefault);
        float sAnim = 0.92f + 0.08f * easeOutQuad(std::min(t / 0.2f, 1.f));
        float s = sAnim;

        if (p.text == "Level up!") {
            if (fontLarge) useFont = fontLarge;
            s *= 1.8f;
        }

        SDL_Color col = p.color; 
        col.a = alpha;

        useFont = (fontMedium ? fontMedium : fontDefault);
        renderTextCenteredScaled(p.text, int(p.x), int(y), col, s, useFont);
    }
}

void Game::triggerLevelUpPopup() {
    const int boardOffsetX = 200;
    const int boardOffsetY = 10;

    const float cx = boardOffsetX + board.getCols() * board.getCellSize() * 0.5f;
    const float cy = boardOffsetY + board.getCellSize() * 5.5f;

    Uint32 now = SDL_GetTicks();

    scorePopups.push_back(ScorePopup{
        "Level up!",
        SDL_Color{255,255,255,255},
        cx,
        cy,
        80.0f,
        SDL_GetTicks(),
        0u,
        1200u
    });
}