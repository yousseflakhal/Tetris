#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <deque>
#include <string>
#include <optional>
#include <memory>
#include <cmath>
#include "Board.hpp"
#include "Shape.hpp"
#include "InputHandler.hpp"
#include "SDLFormUI.hpp"


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
    Uint32 countdownStartTime;
    bool mouseControlEnabled;
    bool waitingForKey = false;
    Action actionToRebind;
    Screen currentScreen;

    std::shared_ptr<UIButton> newGameBtn;
    std::shared_ptr<UIButton> quitBtn;
    std::shared_ptr<UIButton> resumeBtn;
    std::shared_ptr<UIButton> settingsBtn;
    std::shared_ptr<UIButton> gameOverNewGameBtn;
    std::shared_ptr<UIButton> gameOverQuitBtn;
    std::shared_ptr<UICheckbox> mouseControlCheckbox;
    std::vector<std::shared_ptr<UILabel>> controlLabels;
    std::vector<std::shared_ptr<UIButton>> controlButtons;
    std::vector<std::pair<std::string, Action>> controlMappings;
    std::unordered_map<Action, SDL_Keycode> keyBindings;



    void processInput();
    void update();
    void render();
    void spawnNewShape();
    bool isGameOver() const;
    int evaluateLanding(const Shape &candidate);
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
};
