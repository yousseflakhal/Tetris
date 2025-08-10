#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

#include "Shape.hpp"

class Board {
public:
    static constexpr Uint32 FADE_OUT_MS             = 200;
    static constexpr Uint32 FADE_IN_MS              = 100;
    static constexpr Uint32 HARD_DROP_ANIM_DURATION = 300;

    struct LandingAnim {
        int    x;
        int    y;
        Uint32 startTime;
    };

    struct HardDropAnim {
        int    col;
        int    startRow;
        int    endRow;
        Uint32 startTime;
    };

    struct BubbleParticle {
        float  x, y;
        float  vx, vy;
        Uint8  alpha;
        Uint32 startTime;
    };

    struct ScorePopup {
        std::string text;
        SDL_Color   color;
        float       x;
        float       y0;
        float       rise;
        Uint32      start;
        Uint32      delay;
        Uint32      duration;
    };

    Board(int rows, int cols, int cellSize, SDL_Color backgroundColor);
    ~Board();

    void initializeTexture(SDL_Renderer* renderer) const;
    void draw(SDL_Renderer* renderer, int offsetX, int offsetY, bool showPlacedBlocks) const;

    bool  isOccupied(const std::vector<std::pair<int, int>>& coords, int dx, int dy) const;
    void  placeShape(const Shape& shape);
    int   clearFullLines();
    void  finalizeLineClear();
    void  clearBoard();
    Uint8 landingAlpha(int x, int y, Uint32 now) const;
    bool  isCellReachable(int x, int y) const;

    void updateAnimations();
    void updateLandingAnimations();
    void updateHardDropAnimations();
    void updateBubbleParticles();
    void triggerHardDropAnim(const Shape& shape);

    int  getRows() const;
    int  getCols() const;
    int  getCellSize() const;
    const std::vector<std::vector<int>>& getGrid() const;
    const std::vector<int>&              getLinesToClear() const;

    int  countFullLines() const;
    int  countHoles() const;
    std::pair<std::vector<std::pair<int, int>>, bool>
         getSurfaceCoordsAndFlatStatus(int x) const;

    bool                         isClearingLines      = false;
    int                          clearAnimationFrame  = 0;
    mutable Uint32               clearStartTime       = 0;
    mutable SDL_Texture*         whiteCellTexture     = nullptr;
    std::vector<LandingAnim>     landingAnims;

private:
    int       rows;
    int       cols;
    int       cellSize;
    SDL_Color backgroundColor;

    std::vector<std::vector<int>>       grid;
    std::vector<std::vector<SDL_Color>> colorGrid;
    std::vector<int>                    linesToClear;

    std::vector<HardDropAnim>   hardDropAnims;
    std::vector<BubbleParticle> bubbleParticles;
    std::vector<ScorePopup>     scorePopups;

    void triggerScorePopup(int clearedLines, int linePoints);
    void updateScorePopups();
    void renderScorePopups();

    static float easeOutCubic(float t) {
        return 1.0f - std::pow(1.0f - t, 3.0f);
    }
    static float easeInOutQuad(float t) {
        return (t < 0.5f)
                 ? (2.0f * t * t)
                 : (1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f);
    }
    static float clamp01(float v)     { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
    static float easeOutQuad(float t) { return 1.f - (1.f - t) * (1.f - t); }

    static float popupScale(float t) {
        const float grow   = 0.30f;
        const float hold   = 0.15f;
        const float settle = 0.55f;
        const float s0     = 0.65f;
        const float sOver  = 1.25f;
        const float sEnd   = 1.0f;

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
};
