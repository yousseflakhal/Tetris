#include "Shape.hpp"
using namespace std;

Shape::Shape(Type type, int startX, int startY, SDL_Color color)
    : type(type), color(color), rotationState(0) {
    static const std::unordered_map<Type, SDL_Color> shapeColors = {
    {Type::O, {255, 215, 0, 255}},
    {Type::I, {0, 255, 255, 255}},
    {Type::S, {0, 255, 0, 255}},
    {Type::Z, {255, 0, 0, 255}},
    {Type::L, {255, 140, 0, 255}},
    {Type::J, {0, 0, 255, 255}},
    {Type::T, {128, 0, 128, 255}}
    };
    this->color = shapeColors.at(type);
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
        coords = {{startX, startY + 1}, {startX, startY}, {startX, startY + 2}, {startX + 1, startY + 2}};
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
    for (const auto& coord : coords) {
        if (coord.first - 1 < 0) return;
    }
    for (auto& coord : coords) {
        coord.first -= 1;
    }
}

void Shape::moveRight(int boardWidth) {
    for (const auto& coord : coords) {
        if (coord.first + 1 >= boardWidth) return;
    }
    for (auto& coord : coords) {
        coord.first += 1;
    }
}

void Shape::rotateClockwise(const std::vector<std::vector<int>>& board, int boardWidth, int boardHeight) {
    if (type == Type::O) {
        return;
    }
    
    std::vector<std::pair<int, int>> preRotationCoords = coords;
    int oldRotationState = rotationState;
    
    rotateShape(1);
    
    if (isValidPosition(board, boardWidth, boardHeight))
        return;
    
    std::vector<std::pair<int, int>> rotatedCoords = coords;
    std::vector<int> kickOffsets = { 1, -1, 2, -2 };
    
    bool kicked = false;
    for (int dx : kickOffsets) {
        for (size_t i = 0; i < coords.size(); ++i) {
            coords[i].first = rotatedCoords[i].first + dx;
        }
        if (isValidPosition(board, boardWidth, boardHeight)) {
            kicked = true;
            break;
        }
        coords = rotatedCoords;
    }
    
    if (!kicked) {
        coords = preRotationCoords;
        rotationState = oldRotationState;
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

        if (x < 0 || x >= boardWidth || y < 0 || y >= boardHeight) {
            return false;
        }
        
        if (board[y][x] != 0) { 
            return false;
        }
    }
    return true;
}



void Shape::draw(SDL_Renderer* renderer, int cellSize, int offsetX, int offsetY, bool isShadow) const {
    SDL_Color drawColor = color;

    if (isShadow) {
        drawColor.a = 100;
        SDL_SetRenderDrawColor(renderer, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        
        for (const auto& coord : coords) {
            SDL_Rect rect = {offsetX + coord.first * cellSize + 1, offsetY + coord.second * cellSize + 1, cellSize - 1, cellSize - 1};
            SDL_RenderDrawRect(renderer, &rect);
        }
    } else {
        SDL_SetRenderDrawColor(renderer, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        
        for (const auto& coord : coords) {
            SDL_Rect rect = {offsetX + coord.first * cellSize + 1, offsetY + coord.second * cellSize + 1, cellSize - 1, cellSize - 1};
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

const std::vector<std::pair<int, int>>& Shape::getCoords() const {
    return coords;
}

Shape::Type Shape::getType() const {
    return type;
}

void Shape::setPosition(int x, int y) {
    int dx = x - coords[0].first;
    int dy = y - coords[0].second;

    for (auto& coord : coords) {
        coord.first += dx;
        coord.second += dy;
    }
}

void Shape::resetRotation() {
    rotationState = 0;
}

std::vector<std::pair<int, int>> Shape::getLocalCoords() const {
    if (coords.empty()) return {};
    
    int originX = coords[0].first;
    int originY = coords[0].second;
    
    std::vector<std::pair<int, int>> local;
    for (const auto& c : coords) {
        local.emplace_back(c.first - originX, c.second - originY);
    }
    return local;
}

void Shape::rotateCounterClockwise(const std::vector<std::vector<int>>& grid, int cols, int rows) {
    auto newCoords = coords;
    auto pivot = coords[1];

    for (auto& p : newCoords) {
        int x = p.first - pivot.first;
        int y = p.second - pivot.second;

        int rotatedX = y;
        int rotatedY = -x;

        p.first = pivot.first + rotatedX;
        p.second = pivot.second + rotatedY;
    }

    for (const auto& p : newCoords) {
        if (p.first < 0 || p.first >= cols || p.second < 0 || p.second >= rows) {
            return;
        }
        if (grid[p.second][p.first] != 0) {
            return;
        }
    }

    coords = newCoords;
}
