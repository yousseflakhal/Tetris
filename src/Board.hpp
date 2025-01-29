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

    bool isOccupied(const std::vector<std::pair<int, int>>& coords, int dx, int dy) const;
    void placeShape(const Shape& shape);
    void clearFullLines();

    void draw(SDL_Renderer* renderer) const;

    int getRows() const;
    int getCols() const;
    int getCellSize() const;
    const std::vector<std::vector<int>>& getGrid() const;

private:
    int rows, cols, cellSize;
    SDL_Color backgroundColor;
    std::vector<std::vector<int>> grid;
    std::vector<std::vector<SDL_Color>> colorGrid;
};
