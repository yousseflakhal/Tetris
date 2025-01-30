#pragma once
#include <vector>
#include <SDL2/SDL.h>
#include <unordered_map>

class Shape {
public:
    enum class Type { O, I, S, Z, L, J, T };

    Shape(Type type, int startX, int startY, SDL_Color color);

    void moveDown();
    void moveLeft(int boardWidth);
    void moveRight(int boardWidth);

    void rotateClockwise(const std::vector<std::vector<int>>& board, int boardWidth, int boardHeight);
    void rotateCounterClockwise(const std::vector<int>& board, int boardWidth, int boardHeight);

    const std::vector<std::pair<int, int>>& getCoords() const;
    SDL_Color getColor() const { return color; }

    void draw(SDL_Renderer* renderer, int cellSize) const;
    
    

private:
    Type type;
    std::vector<std::pair<int, int>> coords;
    SDL_Color color;
    int rotationState;
    void rotateShape(int direction);
    bool isValidPosition(const std::vector<std::vector<int>>& board, int boardWidth, int boardHeight) const;
};