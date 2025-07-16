#include "DrawUtils.hpp"

void drawAACorner(SDL_Renderer* renderer, int cx, int cy, int radius, int corner, SDL_Color color, Uint8 alpha, bool filled) {
    int startX = 0, endX = 0, startY = 0, endY = 0;
    switch (corner) {
        case 0:
            startX = -radius; endX = 0; startY = -radius; endY = 0; break;
        case 1:
            startX = 0; endX = radius; startY = -radius; endY = 0; break;
        case 2:
            startX = -radius; endX = 0; startY = 0; endY = radius; break;
        case 3:
            startX = 0; endX = radius; startY = 0; endY = radius; break;
    }
    for (int dy = startY; dy < endY; ++dy) {
        for (int dx = startX; dx < endX; ++dx) {
            float distance = sqrtf(dx*dx + dy*dy);
            if (distance <= radius) {
                float a = 1.0f - fmaxf(0.0f, fminf(1.0f, distance - (radius - 1)));
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, Uint8(alpha * a));
                SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
            }
        }
    }
}

void drawRoundedRect(SDL_Renderer* renderer, int x, int y, int w, int h,
                     int radius, SDL_Color color, Uint8 alpha, bool filled)
{
    if (w <= 0 || h <= 0) return;
    radius = std::min(radius, std::min(w, h) / 2);

    if (filled) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
        SDL_Rect center = { x + radius, y + radius, w - 2 * radius, h - 2 * radius };
        SDL_RenderFillRect(renderer, &center);

        SDL_Rect top = { x + radius, y, w - 2 * radius, radius };
        SDL_Rect bottom = { x + radius, y + h - radius, w - 2 * radius, radius };
        SDL_RenderFillRect(renderer, &top);
        SDL_RenderFillRect(renderer, &bottom);

        SDL_Rect left = { x, y + radius, radius, h - 2 * radius };
        SDL_Rect right = { x + w - radius, y + radius, radius, h - 2 * radius };
        SDL_RenderFillRect(renderer, &left);
        SDL_RenderFillRect(renderer, &right);

        drawAACorner(renderer, x + radius, y + radius, radius, 0, color, alpha, true);
        drawAACorner(renderer, x + w - radius - 1, y + radius, radius, 1, color, alpha, true);
        drawAACorner(renderer, x + radius, y + h - radius - 1, radius, 2, color, alpha, true);
        drawAACorner(renderer, x + w - radius - 1, y + h - radius - 1, radius, 3, color, alpha, true);
    } else {
    }
}
