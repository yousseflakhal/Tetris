#include "Board.hpp"


Board::Board(int rows, int cols, int cellSize, SDL_Color backgroundColor)
    : rows(rows), cols(cols), cellSize(cellSize), backgroundColor(backgroundColor) {
    grid = std::vector<std::vector<int>>(rows, std::vector<int>(cols, 0));
}

bool Board::isOccupied(const std::vector<std::pair<int, int>>& coords, int dx, int dy) const {
    for (const auto& coord : coords) {
        int x = coord.first + dx;
        int y = coord.second + dy;

        
        if (x < 0 || x >= cols || y < 0 || y >= rows) {
            std::cout << "Out-of-bounds detected at (" << x << ", " << y << ")\n";
            return true;
        }

        
        if (grid[y][x] != 0) {
            std::cout << "Collision detected at (" << x << ", " << y << ")\n";
            return true;
        }
    }
    return false;
}


void Board::placeShape(const Shape& shape) {
    for (const auto& coord : shape.getCoords()) {
        if (coord.second >= rows || coord.first < 0 || coord.first >= cols) {
            continue;
        }
        grid[coord.second][coord.first] = 1;
    }
}


void Board::clearFullLines() {
    for (int y = rows - 1; y >= 0; --y) {
        if (std::all_of(grid[y].begin(), grid[y].end(), [](int cell) { return cell != 0; })) {
            grid.erase(grid.begin() + y);
            grid.insert(grid.begin(), std::vector<int>(cols, 0));
            ++y;
        }
    }
}

void Board::draw(SDL_Renderer* renderer) const {
    
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_Rect rect = {0, 0, cols * cellSize, rows * cellSize};
    SDL_RenderFillRect(renderer, &rect);

    
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            if (grid[y][x] != 0) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_Rect cellRect = {x * cellSize, y * cellSize, cellSize - 1, cellSize - 1};
                SDL_RenderFillRect(renderer, &cellRect);
                SDL_RenderDrawRect(renderer, &cellRect);
            }
        }
    }
}

int Board::getRows() const {
    return rows;
}

int Board::getCols() const {
    return cols;
}

int Board::getCellSize() const {
    return cellSize;
}

const std::vector<std::vector<int>>& Board::getGrid() const {
    return grid;
}
