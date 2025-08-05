#pragma once

#include <SDL2/SDL.h>
#include <utility>
#include <vector>

#include "Shape.hpp"

class Board {
public:
    static constexpr Uint32 FADE_OUT_MS = 200;
    static constexpr Uint32 FADE_IN_MS  = 100;

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


    Board(int rows, int cols, int cellSize, SDL_Color backgroundColor);
    ~Board();

    void initializeTexture(SDL_Renderer* renderer);
    void draw(SDL_Renderer* renderer, int offsetX, int offsetY, bool showPlacedBlocks) const;

    bool isOccupied(const std::vector<std::pair<int, int>>& coords, int dx, int dy) const;
    void placeShape(const Shape& shape);
    int  clearFullLines();
    void finalizeLineClear();
    void clearBoard();
    void updateLandingAnimations();
    void triggerHardDropAnim(const Shape& shape);
    void updateHardDropAnimations();
    void updateAnimations();
    void updateBubbleParticles();

    Uint8 landingAlpha(int x, int y, Uint32 now) const;

    int  getRows() const;
    int  getCols() const;
    int  getCellSize() const;
    const std::vector<std::vector<int>>& getGrid() const;

    int  countFullLines() const;
    int  countHoles() const;
    std::pair<std::vector<std::pair<int, int>>, bool> getSurfaceCoordsAndFlatStatus(int x) const;

    bool             isClearingLines = false;
    int              clearAnimationFrame = 0;
    mutable Uint32   clearStartTime;
    SDL_Texture*     whiteCellTexture = nullptr;
    std::vector<LandingAnim> landingAnims;

private:
    int       rows;
    int       cols;
    int       cellSize;
    SDL_Color backgroundColor;
    std::vector<HardDropAnim> hardDropAnims;
    static constexpr Uint32   HARD_DROP_ANIM_DURATION = 300;

    std::vector<std::vector<int>>       grid;
    std::vector<std::vector<SDL_Color>> colorGrid;
    std::vector<int>                    linesToClear;
    std::vector<BubbleParticle>         bubbleParticles;

};
