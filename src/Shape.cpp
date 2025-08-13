#include "Shape.hpp"

Shape::Shape(Type type, int startX, int startY, SDL_Color color)
    : rotationState(0), type(type), color(color){
    coords = getDefaultCoordsForType(type);

    for (auto& coord : coords) {
        coord.first += startX;
        coord.second += startY;
    }

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
}

const std::vector<std::pair<int, int>>& Shape::getDefaultCoordsForType(Type type) {
    static const std::unordered_map<Type, std::vector<std::pair<int, int>>> coordsMap = {
        {Type::O, {{-1, 0}, {0, 0}, {-1, 1}, {0, 1}}},
        {Type::I, {{-1, 0}, {-2, 0}, {0, 0}, {1, 0}}},
        {Type::S, {{0, 0}, {-1, 0}, {0, 1}, {1, 1}}},
        {Type::Z, {{0, 0}, {1, 0}, {0, 1}, {-1, 1}}},
        {Type::L, {{-1, 1}, {-1, 0}, {-1, 2}, {0, 2}}},
        {Type::J, {{0, 1}, {0, 0}, {0, 2}, {-1, 2}}},
        {Type::T, {{0, 0}, {-1, 1}, {0, 1}, {1, 1}}}
    };
    return coordsMap.at(type);
}

const std::vector<std::pair<int, int>>& Shape::getCoords() const {
    return coords;
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
    if (type == Type::O) return;

    std::vector<std::pair<int, int>> preRotationCoords = coords;
    int oldRotationState = rotationState;

    rotateShape(1);

    if (isValidPosition(board, boardWidth, boardHeight)) return;

    std::vector<std::pair<int, int>> rotatedCoords = coords;
    std::vector<int> kickOffsets = {1, -1, 2, -2};

    for (int dx : kickOffsets) {
        for (size_t i = 0; i < coords.size(); ++i) {
            coords[i].first = rotatedCoords[i].first + dx;
        }
        if (isValidPosition(board, boardWidth, boardHeight)) {
            rotationState = (rotationState + 1) % 4;
            return;
        }
        coords = rotatedCoords;
    }

    coords = preRotationCoords;
    rotationState = oldRotationState;
}

void Shape::rotateCounterClockwise(const std::vector<std::vector<int>>& board, int boardWidth, int boardHeight) {
    if (type == Type::O) return;

    std::vector<std::pair<int, int>> preRotationCoords = coords;
    int oldRotationState = rotationState;

    rotateShape(-1);

    if (isValidPosition(board, boardWidth, boardHeight)) return;

    std::vector<std::pair<int, int>> rotatedCoords = coords;
    std::vector<int> kickOffsets = {1, -1, 2, -2};

    for (int dx : kickOffsets) {
        for (size_t i = 0; i < coords.size(); ++i) {
            coords[i].first = rotatedCoords[i].first + dx;
        }
        if (isValidPosition(board, boardWidth, boardHeight)) {
            rotationState = (rotationState + 3) % 4;
            return;
        }
        coords = rotatedCoords;
    }

    coords = preRotationCoords;
    rotationState = oldRotationState;
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

        if (x < 0 || x >= boardWidth || y < 0 || y >= boardHeight) return false;
        if (board[y][x] != 0) return false;
    }
    return true;
}

void Shape::draw(SDL_Renderer* renderer, int cellSize, int offsetX, int offsetY, bool isShadow) const {
    const int gap = 1;
    const int margin = 1;
    const int borderThickness = 2;
    const int radius = 6;

    SDL_Color mainColor = color;
    SDL_Color borderColor = darker(color, 0.55f);

    for (const auto& coord : coords) {
        int x = offsetX + coord.first * cellSize + gap;
        int y = offsetY + coord.second * cellSize + gap;
        int w = cellSize - 2 * gap;
        int h = cellSize - 2 * gap;

        if (isShadow) {
            draw_smooth_rounded_rect(renderer, x, y, w, h, radius, mainColor, false, 3);
        } else {
            draw_tetris_cell(renderer, x, y, w, h, radius, margin, borderThickness, mainColor, borderColor);
            draw_smooth_parabolic_highlight_arc(renderer, x, y, w, h, margin, borderThickness);
        }
    }
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
    const int pivotX = coords[0].first;
    const int pivotY = coords[0].second;

    const auto& defaultCoords = getDefaultCoordsForType(type);

    coords.clear();
    for (const auto& dc : defaultCoords) {
        coords.emplace_back(pivotX + dc.first, pivotY + dc.second);
    }

    rotationState = 0;
}

void Shape::getLocalCoords(std::vector<std::pair<int,int>>& out) const {
    out.clear();
    if (coords.empty()) return;

    int originX = coords[0].first;
    int originY = coords[0].second;

    for (const auto& c : coords) {
        out.emplace_back(c.first - originX, c.second - originY);
    }
}
