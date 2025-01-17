#include "Shape.hpp"
using namespace std;

Shape::Shape(Type type, int startX, int startY, SDL_Color color)
    : type(type), color(color), rotationState(0) {
    switch (type) {
        case Type::O:
            coords = {{startX, startY}, {startX + 1, startY}, {startX, startY + 1}, {startX + 1, startY + 1}};
            break;
        case Type::I:
            coords = {{startX, startY}, {startX - 1, startY}, {startX + 1, startY}, {startX + 2, startY}};
            break;
        case Type::S:
            coords = {{startX, startY}, {startX - 1, startY}, {startX, startY + 1}, {startX + 1, startY + 1}};
            break;
        case Type::Z:
            coords = {{startX, startY}, {startX + 1, startY}, {startX, startY + 1}, {startX - 1, startY + 1}};
            break;
        case Type::L:
            coords = {{startX, startY}, {startX, startY + 1}, {startX, startY + 2}, {startX + 1, startY + 2}};
            break;
        case Type::J:
            coords = {{startX, startY}, {startX, startY + 1}, {startX, startY + 2}, {startX - 1, startY + 2}};
            break;
        case Type::T:
            coords = {{startX, startY}, {startX - 1, startY + 1}, {startX, startY + 1}, {startX + 1, startY + 1}};
            break; 
    }
}

void Shape::moveDown() {
    for (auto& coord : coords) {
        coord.second += 1;
    }
}

void Shape::moveLeft() {
    for (auto& coord : coords) {
        coord.first -= 1;
    }
}

void Shape::moveRight() {
    for (auto& coord : coords) {
        coord.first += 1;
    }
}

void Shape::rotateClockwise(const std::vector<std::vector<int>>& board, int boardWidth, int boardHeight) {
    if (type == Type::O) {
        return;
    }

    rotateShape(1);
    if (!isValidPosition(board, boardWidth, boardHeight)) {
        rotateShape(-1);
    }
}

void Shape::rotateShape(int direction) {
    auto pivot = coords[0];
    for (auto& coord : coords) {
        int x = coord.first - pivot.first;
        int y = coord.second - pivot.second;
        coord.first = pivot.first - direction * y;
        coord.second = pivot.second + direction * x;
    }
    rotationState = (rotationState + direction + 4) % 4;
}

bool Shape::isValidPosition(const std::vector<std::vector<int>>& board, int boardWidth, int boardHeight) const {
    for (const auto& coord : coords) {
        int x = coord.first;
        int y = coord.second;
        if (x < 0 || x >= boardWidth || y >= boardHeight || board[y][x] != 0) {
            return false;
        }
    }
    return true;
}


void Shape::draw(SDL_Renderer* renderer, int cellSize) const {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (const auto& coord : coords) {
        SDL_Rect rect = {coord.first * cellSize, coord.second * cellSize, cellSize - 1, cellSize - 1};
        SDL_RenderFillRect(renderer, &rect);
        SDL_RenderDrawRect(renderer, &rect);
    }
}

const std::vector<std::pair<int, int>>& Shape::getCoords() const {
    return coords;
}