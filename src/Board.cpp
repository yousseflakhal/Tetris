#include "Board.hpp"


Board::Board(int rows, int cols, int cellSize, SDL_Color backgroundColor)
    : rows(rows), cols(cols), cellSize(cellSize), backgroundColor(backgroundColor),
      grid(rows, std::vector<int>(cols, 0)),
      colorGrid(rows, std::vector<SDL_Color>(cols, {0, 0, 0, 0}))
{
    whiteCellTexture = nullptr;
}

Board::~Board() {
    if (whiteCellTexture) {
        SDL_DestroyTexture(whiteCellTexture);
        whiteCellTexture = nullptr;
    }
}

void Board::initializeTexture(SDL_Renderer* renderer) {
    if (whiteCellTexture) SDL_DestroyTexture(whiteCellTexture);

    whiteCellTexture = SDL_CreateTexture(renderer,
                                         SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_TARGET,
                                         cellSize - 2, cellSize - 2);
    SDL_SetTextureBlendMode(whiteCellTexture, SDL_BLENDMODE_BLEND);

    SDL_SetRenderTarget(renderer, whiteCellTexture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    draw_smooth_rounded_rect(renderer, 0, 0, cellSize - 2, cellSize - 2, 2, {255, 255, 255, 255}, true);
    SDL_SetRenderTarget(renderer, nullptr);
}

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

            landingAnims.push_back({x, y, SDL_GetTicks()});
        }
    }
}

Uint8 Board::landingAlpha(int x, int y, Uint32 now) const {
    for (const auto& a : landingAnims) {
        if (a.x == x && a.y == y) {
            Uint32 t = now - a.startTime;

            if (t < FADE_OUT_MS) {
                float p = t / float(FADE_OUT_MS);
                return Uint8(255 - p * 200);
            } else if (t < FADE_OUT_MS + FADE_IN_MS) {
                float p = (t - FADE_OUT_MS) / float(FADE_IN_MS);
                return Uint8(55 + p * 200);
            }
        }
    }
    return 255;
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

    std::unordered_set<int> clearLinesSet(linesToClear.begin(), linesToClear.end());

    draw_smooth_rounded_rect(renderer, offsetX, offsetY, boardWidth, boardHeight, 5, {50, 50, 50, 255}, true);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const int cellX = offsetX + x * cellSize + gridGap;
            const int cellY = offsetY + y * cellSize + gridGap;
            const int cellDrawSize = cellSize - 2 * gridGap;

            bool isFullCell = grid[y][x] != 0;
            bool isLineClearing = isClearingLines && clearLinesSet.count(y) > 0;

            draw_smooth_rounded_rect(renderer, cellX, cellY, cellDrawSize, cellDrawSize,
                            2, {0, 0, 0, 255}, true);

            if (showPlacedBlocks && isFullCell) {
                SDL_Color color = colorGrid[y][x];

                if (isLineClearing) {
                    Uint32 currentTime = SDL_GetTicks();
                    float elapsed = static_cast<float>(currentTime - clearStartTime);
                    float progress = std::min(elapsed / 500.0f, 1.0f);

                    Uint8 alpha = static_cast<Uint8>(255 * (1.0f - progress));
                    float rotation = 360.0f * progress;

                    SDL_Rect destRect = {cellX, cellY, cellDrawSize, cellDrawSize};

                    if (!whiteCellTexture) {
                        SDL_Texture* tempTexture = SDL_CreateTexture(
                            renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                            cellDrawSize, cellDrawSize);
                        SDL_SetTextureBlendMode(tempTexture, SDL_BLENDMODE_BLEND);

                        SDL_SetRenderTarget(renderer, tempTexture);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                        SDL_RenderClear(renderer);
                        draw_smooth_rounded_rect(renderer, 0, 0, cellDrawSize, cellDrawSize, 2, color, true);
                        SDL_SetRenderTarget(renderer, nullptr);

                        SDL_RenderCopyEx(renderer, tempTexture, nullptr, &destRect, rotation, nullptr, SDL_FLIP_NONE);
                        SDL_DestroyTexture(tempTexture);
                    } else {
                        SDL_SetTextureColorMod(whiteCellTexture, color.r, color.g, color.b);
                        SDL_SetTextureAlphaMod(whiteCellTexture, alpha);
                        SDL_RenderCopyEx(renderer, whiteCellTexture, nullptr, &destRect, rotation, nullptr, SDL_FLIP_NONE);
                    }
                } else {
                    Uint32 now = SDL_GetTicks();
                    Uint8 alpha = landingAlpha(x, y, now);
                    SDL_Color borderColor = darker(color, 0.55f);

                    draw_tetris_cell(
                        renderer,
                        cellX, cellY, cellDrawSize, cellDrawSize,
                        6,
                        1,
                        2,
                        color,
                        borderColor
                    );
                    draw_smooth_parabolic_highlight_arc(
                        renderer,
                        cellX, cellY, cellDrawSize, cellDrawSize,
                        1, 2
                    );
                }
            }
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
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

    heightLeft = std::clamp(heightLeft, 0, rows - 1);
    heightMid = std::clamp(heightMid, 0, rows - 1);
    heightRight = std::clamp(heightRight, 0, rows - 1);

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

    std::sort(linesToClear.begin(), linesToClear.end(), std::greater<int>());

    for (int line : linesToClear) {
        grid.erase(grid.begin() + line);
        colorGrid.erase(colorGrid.begin() + line);
    }

    int numLinesCleared = static_cast<int>(linesToClear.size());
    for (int i = 0; i < numLinesCleared; ++i) {
        grid.insert(grid.begin(), std::vector<int>(cols, 0));
        colorGrid.insert(colorGrid.begin(), std::vector<SDL_Color>(cols, {0,0,0,0}));
    }

    isClearingLines = false;
    linesToClear.clear();
    clearStartTime = 0;
}


void Board::updateLandingAnimations() {
    Uint32 now = SDL_GetTicks();
    landingAnims.erase(
        std::remove_if(landingAnims.begin(), landingAnims.end(),
            [now](const LandingAnim& a) {
                return now - a.startTime > (FADE_OUT_MS + FADE_IN_MS);
            }),
        landingAnims.end());
}