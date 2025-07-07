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
        return true;
    }

    if (grid[y][x] != 0) {
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


int Board::clearFullLines() {
    linesToClear.clear();
    for (int y = rows - 1; y >= 0; --y) {
        if (std::all_of(grid[y].begin(), grid[y].end(), [](int cell) { return cell != 0; })) {
            linesToClear.push_back(y);
        }
    }
    if (!linesToClear.empty()) {
        isClearingLines = true;
        clearStartTime = SDL_GetTicks();
    }
    return linesToClear.size();
}

void Board::draw(SDL_Renderer* renderer, int offsetX, int offsetY, bool showPlacedBlocks) const {
    const int boardWidth = cols * cellSize;
    const int boardHeight = rows * cellSize;
    const int gridGap = 1;
    
    drawRoundedRect(renderer, offsetX, offsetY, boardWidth, boardHeight, 5, {50, 50, 50, 255}, 255);
    
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const int cellX = offsetX + x * cellSize + gridGap;
            const int cellY = offsetY + y * cellSize + gridGap;
            const int cellDrawSize = cellSize - 2 * gridGap;
            
            if (showPlacedBlocks && grid[y][x] != 0) {
                SDL_Color color = colorGrid[y][x];
                
                if (isClearingLines && std::find(linesToClear.begin(), linesToClear.end(), y) != linesToClear.end()) {
                    Uint32 currentTime = SDL_GetTicks();
                    float elapsed = static_cast<float>(currentTime - clearStartTime);
                    float progress = std::min(elapsed / 500.0f, 1.0f);
                    
                    float scale = 1.0f - progress * 0.5f;
                    int scaledSize = static_cast<int>(cellDrawSize * scale);
                    int offset = (cellDrawSize - scaledSize) / 2;
                    
                    SDL_Color white = {255, 255, 255, static_cast<Uint8>(255 * (1.0f - progress))};
                    drawRoundedRect(renderer, 
                                   cellX + offset, 
                                   cellY + offset, 
                                   scaledSize, 
                                   scaledSize, 
                                   static_cast<int>(2 * scale), 
                                   white, 
                                   white.a);
                } else {
                    drawRoundedRect(renderer, cellX, cellY, cellDrawSize, cellDrawSize, 2, color, 255);
                }
            } else {
                drawRoundedRect(renderer, cellX, cellY, cellDrawSize, cellDrawSize, 2, {0, 0, 0, 255}, 255);
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

int Board::countFullLines() const {
    int fullLineCount = 0;
    for (int y = 0; y < rows; ++y) {
        bool isFull = std::all_of(grid[y].begin(), grid[y].end(),
                                  [](int cell) { return cell != 0; });
        if (isFull) {
            fullLineCount++;
        }
    }
    return fullLineCount;
}

int Board::countHoles() const {
    int holes = 0;
    
    for (int x = 0; x < cols; ++x) {
        bool foundBlockInColumn = false;
        
        for (int y = 0; y < rows; ++y) {
            if (grid[y][x] != 0) {
                foundBlockInColumn = true;
            } else {
                if (foundBlockInColumn) {
                    holes++;
                }
            }
        }
    }
    return holes;
}

std::pair<std::vector<std::pair<int, int>>, bool> Board::getSurfaceCoordsAndFlatStatus(int x) const {
    std::vector<std::pair<int, int>> surfaceCoords;
    
    if (x < 1 || x >= cols - 1) {
        return {surfaceCoords, false};
    }

    int heightLeft = -1, heightMid = -1, heightRight = -1;

    for (int y = 0; y < rows; ++y) {
        if (heightLeft == -1 && grid[y][x - 1] != 0) heightLeft = y - 1;
        if (heightMid == -1 && grid[y][x] != 0) heightMid = y - 1;
        if (heightRight == -1 && grid[y][x + 1] != 0) heightRight = y - 1;
    }

    if (heightLeft == -1) heightLeft = rows - 1;
    if (heightMid == -1) heightMid = rows - 1;
    if (heightRight == -1) heightRight = rows - 1;

    surfaceCoords.push_back({x - 1, heightLeft});
    surfaceCoords.push_back({x, heightMid});
    surfaceCoords.push_back({x + 1, heightRight});

    bool isFlat = (heightLeft == heightMid && heightMid == heightRight);

    return {surfaceCoords, isFlat};
}

void Board::clearBoard() {
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            grid[y][x] = 0;
            colorGrid[y][x] = {0, 0, 0, 0};
        }
    }
}

void Board::finalizeLineClear() {
    if (!isClearingLines) return;

    // Remove full lines
    for (int line : linesToClear) {
        grid.erase(grid.begin() + line);
        colorGrid.erase(colorGrid.begin() + line);
    }

    int numLinesCleared = linesToClear.size();
    for (int i = 0; i < numLinesCleared; ++i) {
        grid.insert(grid.begin(), std::vector<int>(cols, 0));
        colorGrid.insert(colorGrid.begin(), std::vector<SDL_Color>(cols, {0, 0, 0, 0}));
    }

    isClearingLines = false;
    linesToClear.clear();
    clearStartTime = 0;
}