#pragma once
#include <vector>
#include <SDL2/SDL.h>
#include "Shape.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream>

class Board {
public:
    Board(int rows, int cols, int cellSize, SDL_Color backgroundColor);

    bool isClearingLines = false;
    int clearAnimationFrame = 0;
    mutable Uint32 clearStartTime;
    SDL_Texture* whiteCellTexture;

    bool isOccupied(const std::vector<std::pair<int, int>>& coords, int dx, int dy) const;
    void placeShape(const Shape& shape);
    int clearFullLines();

    void draw(SDL_Renderer* renderer, int offsetX, int offsetY, bool showPlacedBlocks) const;

    int getRows() const;
    int getCols() const;
    int getCellSize() const;
    const std::vector<std::vector<int>>& getGrid() const;
    int countFullLines() const;
    int countHoles() const;
    std::pair<std::vector<std::pair<int, int>>, bool> getSurfaceCoordsAndFlatStatus(int x) const;
    void clearBoard();
    void finalizeLineClear();

private:
    int rows, cols, cellSize;
    SDL_Color backgroundColor;
    std::vector<std::vector<int>> grid;
    std::vector<std::vector<SDL_Color>> colorGrid;
    std::vector<int> linesToClear;
    
};
