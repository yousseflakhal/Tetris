#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <deque>
#include <string>
#include <optional>
#include <memory>
#include <cmath>
#include <limits>
#include <vector>
#include <unordered_map>
#include <iostream>

#include "Board.hpp"
#include "Shape.hpp"
#include "InputHandler.hpp"
#include "SDLFormUI.hpp"
#include "SoundManager.hpp"

class Game {
public:
    enum class Action {
        MoveRight,
        MoveLeft,
        RotateRight,
        RotateLeft,
        SoftDrop,
        HardDrop,
        Hold
    };

    Game(int width, int height, int cellSize);
    ~Game();

    void run();

private:
    enum class Screen { Main, Settings };

    void processInput();
    void update();
    void render();

    void spawnNewShape();
    bool isGameOver() const;
    void autoRotateCurrentShape(int targetGridX, int targetGridY = -1);
    void snapShapeHorizontally(int targetGridX);
    int  countContactSegments(const Shape& shape, const Board& board);

    void renderNextPieces();
    void renderHoldPiece();
    void renderGameOverScreen();
    void renderPauseMenu();
    void renderSettingsScreen();
    void renderInfoCard(int x, int y, int width, int height, int radius, 
                        const std::string& title, const std::string& value);
    void renderText(const std::string& text, int x, int y, SDL_Color color);
    void renderTextCenteredScaled(const std::string& text, int cx, int cy, SDL_Color color, float scale, TTF_Font* useFont);

    void checkLevelUp();
    void updateScore(int clearedLines, int dropDistance, bool hardDrop);
    void updateSpeed();
    void resetGame();
    void holdPiece();
    Uint32 getElapsedGameTime() const;

    float countdownScale(Uint32 msInSecond) const;
    static float easeOutCubic(float t);
    static float easeInOutQuad(float t);

    SDL_Window*   window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture*  backgroundTexture = nullptr;

    TTF_Font* fontLarge = nullptr;
    TTF_Font* fontMedium = nullptr;
    TTF_Font* fontSmall = nullptr;
    TTF_Font* fontDefault = nullptr;

    Board      board;
    Shape      currentShape;
    Shape      shadowShape;
    std::optional<Shape> heldShape = std::nullopt;
    std::deque<Shape>    nextPieces;
    bool       canHold = true;

    int    score = 0;
    int    level = 1;
    int    totalLinesCleared = 0;

    int lastMouseTargetGridX = std::numeric_limits<int>::min();
    int lastMouseTargetGridY = std::numeric_limits<int>::min();

    int    cellSize;
    int    windowWidth;
    int    windowHeight;
    int    speed = 800;
    Uint32 lastMoveTime = 0;
    Uint32 lastHorizontalMoveTime = 0;
    Uint32 lastDownMoveTime = 0;
    Uint32 lastRotationTime = 0;
    static constexpr Uint32 horizontalMoveDelay = 50;
    static constexpr Uint32 downMoveDelay = 100;
    static constexpr Uint32 rotationDelay = 100;
    Uint32 gameStartTime = 0;
    Uint32 totalPausedTime = 0;
    Uint32 pauseStartTime = 0;
    Uint32 countdownStartTime = 0;

    bool   running = true;
    bool   ignoreNextMouseClick = false;
    bool   isPaused = false;
    bool   resumeCountdownActive = false;
    bool   isMusicPlaying = false;
    bool   gameOverMusicPlayed = false;
    bool   soundEnabled = false;
    bool   lastSoundEnabled = false;
    bool   startGameTimerAfterCountdown = true;
    bool   mouseControlEnabled = true;
    Screen currentScreen = Screen::Main;

    int   autoPlaceWindow     = 3;
    int   mouseMagnetRadius   = 0;
    float mouseFollowStrength = 0.35f;
    float mouseXAccumulator   = 0.0f;
    float autoPlaceAnchorW    = 10.0f;

    InputHandler inputHandler;

    std::unordered_map<Action, SDL_Keycode> keyBindings = {
        {Action::MoveRight, SDLK_RIGHT},
        {Action::MoveLeft, SDLK_LEFT},
        {Action::RotateRight, SDLK_UP},
        {Action::RotateLeft, SDLK_z},
        {Action::SoftDrop, SDLK_DOWN},
        {Action::HardDrop, SDLK_SPACE},
        {Action::Hold, SDLK_c}
    };

    std::vector<std::pair<std::string, Action>> controlMappings = {
        {"MOVE RIGHT", Action::MoveRight},
        {"MOVE LEFT", Action::MoveLeft},
        {"ROTATE RIGHT", Action::RotateRight},
        {"ROTATE LEFT", Action::RotateLeft},
        {"SOFT DROP", Action::SoftDrop},
        {"HARD DROP", Action::HardDrop},
        {"HOLD", Action::Hold}
    };

    std::vector<std::shared_ptr<UILabel>> controlLabels;
    std::vector<std::shared_ptr<UIButton>> controlButtons;
    mutable std::vector<std::pair<int, int>> tmpCoords;
    bool   waitingForKey = false;
    Action actionToRebind = Action::MoveRight;

    std::shared_ptr<UIButton> newGameBtn;
    std::shared_ptr<UIButton> quitBtn;
    std::shared_ptr<UIButton> resumeBtn;
    std::shared_ptr<UIButton> settingsBtn;
    std::shared_ptr<UIButton> gameOverNewGameBtn;
    std::shared_ptr<UIButton> gameOverQuitBtn;
    std::shared_ptr<UIButton> resetControlsBtn;
    std::shared_ptr<UIButton> doneBtn;
    std::shared_ptr<UICheckbox> mouseControlCheckbox;
    std::shared_ptr<UICheckbox> soundCheckbox;
};
