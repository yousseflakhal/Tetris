#include "Board.hpp"


Board::Board(int rows, int cols, int cellSize, SDL_Color backgroundColor)
    : rows(rows), cols(cols), cellSize(cellSize), backgroundColor(backgroundColor),
      grid(rows, std::vector<int>(cols, 0)),
      colorGrid(rows, std::vector<SDL_Color>(cols, {0, 0, 0, 0})) {}


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
        int x = coord.first;
        int y = coord.second;
        
        if (y >= 0 && y < rows && x >= 0 && x < cols) {
            grid[y][x] = 1;
            colorGrid[y][x] = shape.getColor();
        }
    }
}


void Board::clearFullLines() {
    for (int y = rows - 1; y >= 0; --y) {
        if (std::all_of(grid[y].begin(), grid[y].end(), [](int cell) { return cell != 0; })) {
            grid.erase(grid.begin() + y);
            grid.insert(grid.begin(), std::vector<int>(cols, 0));
            
            colorGrid.erase(colorGrid.begin() + y);
            colorGrid.insert(colorGrid.begin(), std::vector<SDL_Color>(cols, {0, 0, 0, 0}));
            ++y;
        }
    }
}

void Board::draw(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect rect = {0, 0, cols * cellSize, rows * cellSize};
    SDL_RenderFillRect(renderer, &rect);


    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    for (int x = 0; x <= cols; ++x) {
        SDL_RenderDrawLine(renderer, x * cellSize, 0, x * cellSize, rows * cellSize);
    }
    for (int y = 0; y <= rows; ++y) {
        SDL_RenderDrawLine(renderer, 0, y * cellSize, cols * cellSize, y * cellSize);
    }
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            if (grid[y][x] != 0) {
                SDL_SetRenderDrawColor(renderer, colorGrid[y][x].r, colorGrid[y][x].g, colorGrid[y][x].b, 255);
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
