#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
#include <random>
#include <unordered_map>
#include <cstdint>

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

    Board(int rows, int cols, int cellSize, SDL_Color backgroundColor, uint32_t seed = std::random_device{}());
    ~Board();

    void initializeTexture(SDL_Renderer* renderer);
    void draw(SDL_Renderer* renderer, int offsetX, int offsetY, bool showPlacedBlocks) const;

    bool  isOccupied(const std::vector<std::pair<int, int>>& coords, int dx, int dy) const noexcept;
    void  placeShape(const Shape& shape);
    int   clearFullLines();
    void  finalizeLineClear();
    void  clearBoard();
    Uint8 landingAlpha(int x, int y, Uint32 now) const noexcept;
    bool  isCellReachable(int x, int y) const noexcept;

    void updateAnimations();
    void updateLandingAnimations();
    void updateHardDropAnimations();
    void updateBubbleParticles();
    void triggerHardDropAnim(const Shape& shape);

    void rebuildGridBackground(SDL_Renderer* renderer);

    void prewarm(SDL_Renderer* r);

    int  getRows() const noexcept;
    int  getCols() const noexcept;
    int  getCellSize() const noexcept;
    const std::vector<std::vector<int>>& getGrid() const noexcept;
    const std::vector<int>&              getLinesToClear() const noexcept;

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

    std::mt19937 rng;

    mutable SDL_Texture* gridBgTex = nullptr;

    mutable std::unordered_map<uint32_t, SDL_Texture*> tileTexByColor{};
    void clearTileTextures();
    SDL_Texture* getTileTexture(SDL_Renderer* r, SDL_Color base) const;
    static uint32_t packColor(SDL_Color c) noexcept {
        return (uint32_t(c.r) << 24) | (uint32_t(c.g) << 16) | (uint32_t(c.b) << 8) | uint32_t(c.a);
    }

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
