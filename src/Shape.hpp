#pragma once
#include <vector>
#include <SDL2/SDL.h>
#include <unordered_map>
#include "DrawUtils.hpp"

class Shape {
public:
    enum class Type { O, I, S, Z, L, J, T };

    std::vector<std::pair<int, int>> coords;

    Shape(Type type, int startX, int startY, SDL_Color color);

    void moveDown();
    void moveLeft();
    void moveRight(int boardWidth);

    void rotateClockwise(const std::vector<std::vector<int>>& board, int boardWidth, int boardHeight);
    void rotateCounterClockwise(const std::vector<std::vector<int>>& board, int boardWidth, int boardHeight);

    const std::vector<std::pair<int, int>>& getCoords() const;
    SDL_Color getColor() const { return color; }

    void draw(SDL_Renderer* renderer, int cellSize, int offsetX = 0, int offsetY = 0, bool isShadow = false) const;
    Type getType() const;

    void setPosition(int x, int y);
    void getLocalCoords(std::vector<std::pair<int,int>>& out) const;

    int rotationState;

private:
    Type type;
    SDL_Color color;

    void rotateShape(int direction);
    bool isValidPosition(const std::vector<std::vector<int>>& board, int boardWidth, int boardHeight) const;
    static const std::vector<std::pair<int, int>>& getDefaultCoordsForType(Type type);
    void resetRotation();
};
