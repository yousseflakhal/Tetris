#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <limits>
#include <stdexcept>
#include <random>

#include "Board.hpp"
#include "Shape.hpp"
#include "InputHandler.hpp"
#include "SDLFormUI.hpp"
#include "SoundManager.hpp"

class UILabel;
class UIButton;
class UICheckbox;

struct ScorePopup {
    std::string text;
    SDL_Color   color{255,255,255,255};
    float       x = 0.f, y0 = 0.f, rise = 40.f;
    Uint32      start = 0, delay = 0, duration = 900;
    float       scale = 1.0f;
    TTF_Font*   font = nullptr;

    SDL_Texture* tex = nullptr;
    SDL_Texture* shadowTex = nullptr;
    int texW = 0, texH = 0;

    ScorePopup() = default;
    ~ScorePopup() {
        if (tex) SDL_DestroyTexture(tex);
        if (shadowTex) SDL_DestroyTexture(shadowTex);
    }
    ScorePopup(const ScorePopup&) = delete;
    ScorePopup& operator=(const ScorePopup&) = delete;
    ScorePopup(ScorePopup&& o) noexcept
    : text(std::move(o.text)), color(o.color), x(o.x), y0(o.y0), rise(o.rise),
        start(o.start), delay(o.delay), duration(o.duration), scale(o.scale),
        font(o.font), tex(o.tex), shadowTex(o.shadowTex), texW(o.texW), texH(o.texH) {
        o.tex = nullptr; o.shadowTex = nullptr;
    }
    ScorePopup& operator=(ScorePopup&& o) noexcept {
        if (this != &o) {
            if (tex) SDL_DestroyTexture(tex);
            if (shadowTex) SDL_DestroyTexture(shadowTex);
            text = std::move(o.text);
            color = o.color; x = o.x; y0 = o.y0; rise = o.rise;
            start = o.start; delay = o.delay; duration = o.duration; scale = o.scale;
            font = o.font; tex = o.tex; shadowTex = o.shadowTex; texW = o.texW; texH = o.texH;
            o.tex = nullptr; o.shadowTex = nullptr;
        }
        return *this;
    }
};

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

    Game(int width, int height, int cellSize, std::optional<uint32_t> seed = std::nullopt);
    ~Game();

    void run();

private:
    enum class Screen { Main, Settings };

    struct UI {
        static constexpr int BoardOffsetX = 200;
        static constexpr int BoardOffsetY = 10;
    };

    void processInput();
    void update();
    void render();

    void spawnNewShape();
    bool isGameOver() const noexcept;

    void autoRotateCurrentShape(int targetGridX, int targetGridY = -1);
    void snapShapeHorizontally(int targetGridX);
    int  countContactSegments(const Shape& shape, const Board& board) const;

    void renderNextPieces();
    void renderHoldPiece();
    void renderGameOverScreen();
    void renderPauseMenu();
    void renderSettingsScreen();
    void renderInfoCard(int x, int y, int width, int height, int radius,
                        const std::string& title, const std::string& value);
    void renderText(const std::string& text, int x, int y, SDL_Color color);
    void renderTextCenteredScaled(const std::string& text, int cx, int cy,
                                  SDL_Color color, float scale, TTF_Font* useFont);

    void   checkLevelUp();
    void   updateScore(int clearedLines, int dropDistance, bool hardDrop);
    void   updateSpeed();
    void   resetGame();
    void   holdPiece();
    Uint32 getElapsedGameTime() const noexcept;

    bool didWarmup = false;
    void warmupOnce();

    float       countdownScale(Uint32 msInSecond) const noexcept;
    static float easeOutCubic(float t) noexcept;
    static float easeInOutQuad(float t) noexcept;

    void triggerScorePopup(int clearedLines, int linePoints);
    void triggerLevelUpPopup();
    void updateScorePopups();
    void renderScorePopups();

    void triggerScorePopup(const std::string& msg, SDL_Color col, int cx, int cy);

    SDL_Window*   window            = nullptr;
    SDL_Renderer* renderer          = nullptr;
    SDL_Texture*  backgroundTexture = nullptr;

    TTF_Font* fontLarge   = nullptr;
    TTF_Font* fontMedium  = nullptr;
    TTF_Font* fontSmall   = nullptr;
    TTF_Font* fontDefault = nullptr;

    Board                   board;
    Shape                   currentShape;
    Shape                   shadowShape;
    std::optional<Shape>    heldShape  = std::nullopt;
    std::deque<Shape>       nextPieces;
    bool                    canHold    = true;

    int  score             = 0;
    int  level             = 1;
    int  totalLinesCleared = 0;

    int lastMouseTargetGridX = std::numeric_limits<int>::min();
    int lastMouseTargetGridY = std::numeric_limits<int>::min();

    int    cellSize;
    int    windowWidth;
    int    windowHeight;
    int    speed                 = 800;
    Uint32 lastMoveTime          = 0;
    Uint32 lastHorizontalMoveTime= 0;
    Uint32 lastDownMoveTime      = 0;
    Uint32 lastRotationTime      = 0;

    static constexpr Uint32 horizontalMoveDelay =  50;
    static constexpr Uint32 downMoveDelay       = 100;
    static constexpr Uint32 rotationDelay       = 100;

    Uint32 gameStartTime     = 0;
    Uint32 totalPausedTime   = 0;
    Uint32 pauseStartTime    = 0;
    Uint32 countdownStartTime= 0;

    std::vector<ScorePopup> scorePopups;

    bool   running                    = true;
    bool   ignoreNextMouseClick       = false;
    bool   isPaused                   = false;
    bool   resumeCountdownActive      = false;
    bool   isMusicPlaying             = false;
    bool   gameOverMusicPlayed        = false;
    bool   soundEnabled               = false;
    bool   lastSoundEnabled           = false;
    bool   startGameTimerAfterCountdown = true;
    bool   mouseControlEnabled        = true;
    Screen currentScreen              = Screen::Main;

    int   autoPlaceWindow     = 2;
    int   mouseMagnetRadius   = 0;
    float mouseFollowStrength = 0.35f;
    float mouseXAccumulator   = 0.0f;
    float autoPlaceAnchorW    = 2.0f;

    InputHandler inputHandler;

    std::unordered_map<Action, SDL_Keycode> keyBindings = {
        {Action::MoveRight,   SDLK_RIGHT },
        {Action::MoveLeft,    SDLK_LEFT  },
        {Action::RotateRight, SDLK_UP    },
        {Action::RotateLeft,  SDLK_z     },
        {Action::SoftDrop,    SDLK_DOWN  },
        {Action::HardDrop,    SDLK_SPACE },
        {Action::Hold,        SDLK_c     }
    };

    std::vector<std::pair<std::string, Action>> controlMappings = {
        {"MOVE RIGHT",   Action::MoveRight  },
        {"MOVE LEFT",    Action::MoveLeft   },
        {"ROTATE RIGHT", Action::RotateRight},
        {"ROTATE LEFT",  Action::RotateLeft },
        {"SOFT DROP",    Action::SoftDrop   },
        {"HARD DROP",    Action::HardDrop   },
        {"HOLD",         Action::Hold       }
    };

    std::vector<std::shared_ptr<UILabel>>   controlLabels;
    std::vector<std::shared_ptr<UIButton>>  controlButtons;
    mutable std::vector<std::pair<int,int>> tmpCoords;

    bool   waitingForKey   = false;
    Action actionToRebind  = Action::MoveRight;

    std::shared_ptr<UIButton>   newGameBtn;
    std::shared_ptr<UIButton>   quitBtn;
    std::shared_ptr<UIButton>   resumeBtn;
    std::shared_ptr<UIButton>   settingsBtn;
    std::shared_ptr<UIButton>   gameOverNewGameBtn;
    std::shared_ptr<UIButton>   gameOverQuitBtn;
    std::shared_ptr<UIButton>   resetControlsBtn;
    std::shared_ptr<UIButton>   doneBtn;
    std::shared_ptr<UICheckbox> mouseControlCheckbox;
    std::shared_ptr<UICheckbox> soundCheckbox;

    std::optional<Shape> plannedMouseLock;
    bool                 plannedCoversTarget = false;
    
    bool mouseMovedThisFrame = false;

    std::vector<Shape> computeReachableLocks(const Shape& start) const;
    void               planMousePlacement(int targetGridX, int targetGridY);
    int                scorePlacement(const Shape& locked, int targetGridX, int targetGridY) const;

    static int   minYOf(const Shape& s) noexcept;
    static bool  shapeCoversCell(const Shape& s, int gx, int gy) noexcept;
    void         alignToPlannedLock();
    void         performHardDrop();

    bool isCellReachable(int gridX, int gridY) const;

    static float clamp01(float v) noexcept     { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
    static float easeOutQuad(float t) noexcept { return 1.f - (1.f - t) * (1.f - t); }
    static float popupScale(float t) noexcept  {
        const float grow   = 0.30f, hold = 0.15f, settle = 0.55f;
        const float s0     = 0.65f, sOver = 1.25f, sEnd = 1.0f;
        if (t < grow) {
            const float p = t / grow;
            return s0 + (sOver - s0) * easeOutCubic(p);
        } else if (t < grow + hold) {
            return sOver;
        } else {
            float p = (t - (grow + hold)) / settle;
            if (p > 1.f) p = 1.f;
            return sOver + (sEnd - sOver) * easeInOutQuad(p);
        }
    }
    std::mt19937 rng;
};
