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

    SDL_Window* window;
    SDL_Renderer* renderer;
    Board board;
    Shape currentShape;
    bool canHold;
    Shape shadowShape;
    InputHandler inputHandler;
    bool running;
    Uint32 lastMoveTime;
    int speed;
    int cellSize;
    Uint32 lastHorizontalMoveTime;
    Uint32 lastDownMoveTime;
    Uint32 lastRotationTime;
    const Uint32 horizontalMoveDelay;
    const Uint32 downMoveDelay;
    const Uint32 rotationDelay;
    int windowWidth;
    int windowHeight;
    std::deque<Shape> nextPieces;
    int level;
    int totalLinesCleared;
    int score;
    TTF_Font* font;
    std::optional<Shape> heldShape;
    bool ignoreNextMouseClick;
    bool isPaused;
    bool resumeCountdownActive;
    bool waitingForKey = false;
    Action actionToRebind;
    Screen currentScreen;
    bool isMusicPlaying;
    bool gameOverMusicPlayed = false;
    bool soundEnabled;
    bool lastSoundEnabled;
    SDL_Texture* backgroundTexture = nullptr;
    Uint32 gameStartTime = 0;
    Uint32 totalPausedTime = 0;
    Uint32 pauseStartTime = 0;
    bool isTimingPaused = false;
    bool startGameTimerAfterCountdown = false;
    bool mouseControlEnabled;
    Uint32 countdownStartTime = 0;
    TTF_Font* countdownFont = nullptr;

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
    std::vector<std::shared_ptr<UILabel>> controlLabels;
    std::vector<std::shared_ptr<UIButton>> controlButtons;
    std::vector<std::pair<std::string, Action>> controlMappings;
    std::unordered_map<Action, SDL_Keycode> keyBindings;



    void processInput();
    void update();
    void render();
    void spawnNewShape();
    bool isGameOver() const;
    void autoRotateCurrentShape(int targetGridX);
    void renderNextPieces();
    void checkLevelUp();
    void updateScore(int clearedLines, int dropDistance, bool hardDrop);
    void renderText(const std::string& text, int x, int y, SDL_Color color);
    void holdPiece();
    void renderHoldPiece();
    void renderGameOverScreen();
    void resetGame();
    void updateSpeed();
    void renderPauseMenu();
    void renderSettingsScreen();
    Uint32 getElapsedGameTime() const;
    void renderInfoCard(int x, int y, int width, int height, int radius, const std::string& title, const std::string& value);
    int countContactSegments(const Shape& shape, const Board& board);
    float countdownScale(Uint32 msInSecond) const;
    static float easeOutCubic(float t);
    static float easeInOutQuad(float t);
    void renderTextCenteredScaled(const std::string& text, int cx, int cy, SDL_Color color, float scale, TTF_Font* useFont);
};
