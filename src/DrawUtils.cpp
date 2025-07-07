#include "DrawUtils.hpp"

void drawAACircle(SDL_Renderer* renderer, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            float distance = sqrtf(dx*dx + dy*dy);
            if (distance <= radius) {
                float alpha = 1.0f - fmaxf(0.0f, fminf(1.0f, distance - (radius - 1)));
                Uint8 r, g, b, a;
                SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
                SDL_SetRenderDrawColor(renderer, r, g, b, static_cast<Uint8>(alpha * a));
                SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
            }
        }
    }
}

void drawRoundedRect(SDL_Renderer* renderer, int x, int y, int w, int h, int radius, SDL_Color color, Uint8 alpha) {
    if (w <= 0 || h <= 0) return;

    Uint8 orig_r, orig_g, orig_b, orig_a;
    SDL_GetRenderDrawColor(renderer, &orig_r, &orig_g, &orig_b, &orig_a);

    radius = std::min(radius, std::min(w, h) / 2);

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);

    if (w > 2 * radius && h > 2 * radius) {
        SDL_Rect center_rect = {x + radius, y + radius, w - 2 * radius, h - 2 * radius};
        SDL_RenderFillRect(renderer, &center_rect);
    }

    if (w > 2 * radius) {
        SDL_Rect top_rect = {x + radius, y, w - 2 * radius, radius};
        SDL_Rect bottom_rect = {x + radius, y + h - radius, w - 2 * radius, radius};
        SDL_RenderFillRect(renderer, &top_rect);
        SDL_RenderFillRect(renderer, &bottom_rect);
    }

    if (h > 2 * radius) {
        SDL_Rect left_rect = {x, y + radius, radius, h - 2 * radius};
        SDL_Rect right_rect = {x + w - radius, y + radius, radius, h - 2 * radius};
        SDL_RenderFillRect(renderer, &left_rect);
        SDL_RenderFillRect(renderer, &right_rect);
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    if (radius > 0) {
        drawAACircle(renderer, x + radius, y + radius, radius);
        drawAACircle(renderer, x + w - radius - 1, y + radius, radius);
        drawAACircle(renderer, x + radius, y + h - radius - 1, radius);
        drawAACircle(renderer, x + w - radius - 1, y + h - radius - 1, radius);
    }

    SDL_SetRenderDrawColor(renderer, orig_r, orig_g, orig_b, orig_a);
}