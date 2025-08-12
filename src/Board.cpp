#include "Board.hpp"


Board::Board(int rows, int cols, int cellSize, SDL_Color backgroundColor, uint32_t seed)
    : rows(rows), cols(cols), cellSize(cellSize), backgroundColor(backgroundColor),
      grid(rows, std::vector<int>(cols, 0)),
      colorGrid(rows, std::vector<SDL_Color>(cols, {0, 0, 0, 0})),
      rng(seed) {}

Board::~Board() {
    if (whiteCellTexture) {
        SDL_DestroyTexture(whiteCellTexture);
        whiteCellTexture = nullptr;
    }
}

void Board::initializeTexture(SDL_Renderer* renderer) const {
    if (cellSize <= 2) return;
    if (whiteCellTexture) {
        SDL_DestroyTexture(whiteCellTexture);
        whiteCellTexture = nullptr;
    }

    whiteCellTexture = SDL_CreateTexture(renderer,
                                         SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_TARGET,
                                         cellSize - 2, cellSize - 2);

    if (!whiteCellTexture) {
        SDL_Log("Failed to create whiteCellTexture: %s", SDL_GetError());
        return;
    }

    if (SDL_SetTextureBlendMode(whiteCellTexture, SDL_BLENDMODE_BLEND) != 0) {
        SDL_Log("Failed to set blend mode for whiteCellTexture: %s", SDL_GetError());
        SDL_DestroyTexture(whiteCellTexture);
        whiteCellTexture = nullptr;
        return;
    }

    if (SDL_SetRenderTarget(renderer, whiteCellTexture) != 0) {
        SDL_Log("Failed to set render target: %s", SDL_GetError());
        SDL_DestroyTexture(whiteCellTexture);
        whiteCellTexture = nullptr;
        return;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    draw_smooth_rounded_rect(renderer, 0, 0, cellSize - 2, cellSize - 2, 2,
                             {255, 255, 255, 255}, true);

    SDL_SetRenderTarget(renderer, nullptr);
}

bool Board::isOccupied(const std::vector<std::pair<int, int>>& coords, int dx, int dy) const {
    for (const auto& coord : coords) {
        int x = coord.first + dx;
        int y = coord.second + dy;

        if (x < 0 || x >= cols || y >= rows) {
            return true;
        }

        if (y < 0) {
            continue;
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
    if (!whiteCellTexture) {
        initializeTexture(renderer);
    }
    const int boardWidth = cols * cellSize;
    const int boardHeight = rows * cellSize;
    const int gridGap = 1;

    auto isRowClearing = [this](int y) {
        for (int r : linesToClear) if (r == y) return true;
        return false;
    };

    draw_smooth_rounded_rect(renderer, offsetX, offsetY, boardWidth, boardHeight, 5, {50, 50, 50, 255}, true);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const int cellX = offsetX + x * cellSize + gridGap;
            const int cellY = offsetY + y * cellSize + gridGap;
            const int cellDrawSize = cellSize - 2 * gridGap;
            bool isFullCell = grid[y][x] != 0;
            bool isLineClearing = isClearingLines && isRowClearing(y);
            draw_smooth_rounded_rect(renderer, cellX, cellY, cellDrawSize, cellDrawSize, 2, {0, 0, 0, 255}, true);

            if (showPlacedBlocks && isFullCell) {
                SDL_Color color = colorGrid[y][x];
                if (isLineClearing) {
                    Uint32 currentTime = SDL_GetTicks();
                    float elapsed = static_cast<float>(currentTime - clearStartTime);
                    float progress = std::min(elapsed / 500.0f, 1.0f);
                    Uint8 alpha = static_cast<Uint8>(255 * (1.0f - progress));
                    float rotation = 360.0f * progress;
                    SDL_Rect destRect = {cellX, cellY, cellDrawSize, cellDrawSize};

                    SDL_SetTextureColorMod(whiteCellTexture, color.r, color.g, color.b);
                    SDL_SetTextureAlphaMod(whiteCellTexture, alpha);
                    SDL_RenderCopyEx(renderer, whiteCellTexture, nullptr, &destRect, rotation, nullptr, SDL_FLIP_NONE);
                } else {
                    SDL_Color borderColor = darker(color, 0.55f);
                    draw_tetris_cell(renderer, cellX, cellY, cellDrawSize, cellDrawSize, 6, 1, 2, color, borderColor);
                    draw_smooth_parabolic_highlight_arc(renderer, cellX, cellY, cellDrawSize, cellDrawSize, 1, 2);
                }
            }
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    Uint32 now = SDL_GetTicks();
    for (const auto& anim : hardDropAnims) {
        Uint32 elapsed = now - anim.startTime;
        if (elapsed > HARD_DROP_ANIM_DURATION) continue;
        float progress = elapsed / (float)HARD_DROP_ANIM_DURATION;
        Uint8 baseAlpha = static_cast<Uint8>(180 * (1.0f - progress * progress));
        int cellX = offsetX + anim.col * cellSize;
        int span = std::max(1, anim.endRow - anim.startRow);

        for (int row = anim.startRow; row < anim.endRow; ++row) {
            float rowT = (row - anim.startRow) / float(span);
            float falloff = rowT * rowT;
            Uint8 rowAlpha = static_cast<Uint8>(baseAlpha * falloff);
            int cellY = offsetY + row * cellSize;
            SDL_Rect flashRect = { cellX + 1, cellY + 1, cellSize - 2, cellSize - 2 };
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, rowAlpha);
            SDL_RenderFillRect(renderer, &flashRect);
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (const auto& p : bubbleParticles) {
        int px = offsetX + static_cast<int>(p.x * cellSize);
        int py = offsetY + static_cast<int>(p.y * cellSize);
        int radius = std::max(1, cellSize / 16);

        SDL_Color col{255, 255, 255, p.alpha};
        drawAACircle(renderer, px, py, radius, col);
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

const std::vector<int>& Board::getLinesToClear() const {
    return linesToClear;
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

    std::vector<char> clear(rows, 0);
    for (int y : linesToClear) clear[y] = 1;

    int write = rows - 1;
    for (int read = rows - 1; read >= 0; --read) {
        if (!clear[read]) {
            if (write != read) {
                grid[write]      = std::move(grid[read]);
                colorGrid[write] = std::move(colorGrid[read]);
            }
            --write;
        }
    }
    for (; write >= 0; --write) {
        grid[write].assign(cols, 0);
        colorGrid[write].assign(cols, SDL_Color{0,0,0,0});
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

void Board::triggerHardDropAnim(const Shape& shape) {
    Uint32 now = SDL_GetTicks();
    std::unordered_map<int, int> topRows;
    
    for (const auto& coord : shape.getCoords()) {
        int col = coord.first;
        if (topRows.find(col) == topRows.end() || coord.second < topRows[col]) {
            topRows[col] = coord.second;
        }
    }
    
    for (const auto& [col, row] : topRows) {
        hardDropAnims.push_back({col, 0, row, now});

        const int maxSpan = std::max(row, 1);
        const int denom   = maxSpan * 100;

        if (denom > 0) {
            std::uniform_int_distribution<int> fyInt(0, denom - 1);
            std::uniform_int_distribution<int> vyInt(0, 29);
            for (int i = 0; i < 5; ++i) {
                float fx = col + 0.5f;
                float fy = fyInt(rng) / 100.0f;
                float vx = 0.0f;
                float vy = -0.05f - (vyInt(rng) / 300.0f);
                bubbleParticles.push_back({fx, fy, vx, vy, 255, now});
            }
        }
    }
}

void Board::updateHardDropAnimations() {
    Uint32 now = SDL_GetTicks();
    auto it = hardDropAnims.begin();
    while (it != hardDropAnims.end()) {
        if (now - it->startTime > HARD_DROP_ANIM_DURATION) {
            it = hardDropAnims.erase(it);
        } else {
            ++it;
        }
    }
}

void Board::updateBubbleParticles() {
    Uint32 now = SDL_GetTicks();
    bubbleParticles.erase(
        std::remove_if(bubbleParticles.begin(), bubbleParticles.end(),
            [now](const BubbleParticle& p) {
                return now - p.startTime > 600;
            }),
        bubbleParticles.end()
    );

    for (auto& p : bubbleParticles) {
        p.x += p.vx;
        p.y += p.vy;
        float lifeRatio = (now - p.startTime) / 600.0f;
        p.alpha = Uint8(255 * (1.0f - lifeRatio * lifeRatio));
    }
}

void Board::updateAnimations() {
    updateLandingAnimations();
    updateHardDropAnimations();
    updateBubbleParticles();
}

bool Board::isCellReachable(int x, int y) const {
    if (y < 0) return true;
    
    for (int i = y - 1; i >= 0; i--) {
        if (grid[i][x] != 0) {
            return false;
        }
    }
    return true;
}