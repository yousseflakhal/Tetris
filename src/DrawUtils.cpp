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

void drawRoundedRect(SDL_Renderer* renderer, int x, int y, int w, int h, 
                     int radius, SDL_Color color, Uint8 alpha, bool filled) {
    if (w <= 0 || h <= 0) return;

    Uint8 orig_r, orig_g, orig_b, orig_a;
    SDL_GetRenderDrawColor(renderer, &orig_r, &orig_g, &orig_b, &orig_a);

    radius = std::min(radius, std::min(w, h) / 2);

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);

    if (filled) {
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
    }

    if (radius > 0) {
        drawAACircle(renderer, x + radius, y + radius, radius);
        drawAACircle(renderer, x + w - radius - 1, y + radius, radius);
        drawAACircle(renderer, x + radius, y + h - radius - 1, radius);
        drawAACircle(renderer, x + w - radius - 1, y + h - radius - 1, radius);
    }

    if (w > 2 * radius) {
        SDL_RenderDrawLine(renderer, x + radius, y, x + w - radius, y);
        SDL_RenderDrawLine(renderer, x + radius, y + h - 1, x + w - radius, y + h - 1);
    }

    if (h > 2 * radius) {
        SDL_RenderDrawLine(renderer, x, y + radius, x, y + h - radius);
        SDL_RenderDrawLine(renderer, x + w - 1, y + radius, x + w - 1, y + h - radius);
    }

    SDL_SetRenderDrawColor(renderer, orig_r, orig_g, orig_b, orig_a);
}

void drawUIMenuRoundedRect(SDL_Renderer* renderer, int x, int y, int w, int h,
                           int radius, SDL_Color color, Uint8 alpha) {
    if (w <= 0 || h <= 0) return;
    radius = std::min(radius, std::min(w, h) / 2);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);

    SDL_Rect center = { x + radius, y + radius, w - 2 * radius, h - 2 * radius };
    SDL_RenderFillRect(renderer, &center);

    SDL_Rect top    = { x + radius, y, w - 2 * radius, radius };
    SDL_Rect bottom = { x + radius, y + h - radius, w - 2 * radius, radius };
    SDL_Rect left   = { x, y + radius, radius, h - 2 * radius };
    SDL_Rect right  = { x + w - radius, y + radius, radius, h - 2 * radius };
    SDL_RenderFillRect(renderer, &top);
    SDL_RenderFillRect(renderer, &bottom);
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);

    for (int dy = 0; dy < radius; ++dy) {
        for (int dx = 0; dx < radius; ++dx) {
            if (dx*dx + dy*dy <= radius*radius) {
                SDL_RenderDrawPoint(renderer, x + radius - dx - 1, y + radius - dy - 1);
                SDL_RenderDrawPoint(renderer, x + w - radius + dx, y + radius - dy - 1);
                SDL_RenderDrawPoint(renderer, x + radius - dx - 1, y + h - radius + dy);
                SDL_RenderDrawPoint(renderer, x + w - radius + dx, y + h - radius + dy);
            }
        }
    }
}

void drawCardWithBorder(SDL_Renderer* renderer,
                        int x, int y, int w, int h,
                        int radius,
                        SDL_Color bgColor,
                        SDL_Color borderColor,
                        int borderThickness = 2)
{
    for (int i = 0; i < borderThickness; ++i) {
        drawRoundedRect(
            renderer,
            x + i, y + i,
            w - 2 * i, h - 2 * i,
            std::max(0, radius - i),
            borderColor,
            borderColor.a,
            false
        );
    }

    drawRoundedRect(
        renderer,
        x + borderThickness, y + borderThickness,
        w - 2 * borderThickness, h - 2 * borderThickness,
        std::max(0, radius - borderThickness),
        bgColor,
        bgColor.a,
        true
    );
}

void draw_smooth_rounded_rect(SDL_Renderer* renderer,
                              int x, int y, int w, int h,
                              int radius, SDL_Color color,
                              bool filled, int borderThickness) 
{
    // Set blend mode for anti-aliasing
    SDL_BlendMode original_mode;
    SDL_GetRenderDrawBlendMode(renderer, &original_mode);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    if (filled) {
        SDL_Rect center = {x + radius, y, w - 2 * radius, h};
        SDL_RenderFillRect(renderer, &center);

        SDL_Rect sides = {x, y + radius, w, h - 2 * radius};
        SDL_RenderFillRect(renderer, &sides);
    }

    const int centers[4][2] = {
        {x + radius, y + radius},
        {x + w - radius, y + radius},
        {x + radius, y + h - radius},
        {x + w - radius, y + h - radius}
    };

    for (int corner = 0; corner < 4; corner++) {
        int cx = centers[corner][0];
        int cy = centers[corner][1];

        int start_x = (corner % 2 == 0) ? x : x + w - radius;
        int end_x   = (corner % 2 == 0) ? x + radius : x + w;

        int start_y = (corner < 2) ? y : y + h - radius;
        int end_y   = (corner < 2) ? y + radius : y + h;

        for (int py = start_y; py < end_y; py++) {
            for (int px = start_x; px < end_x; px++) {
                float dx = px - cx + 0.5f;
                float dy = py - cy + 0.5f;
                float distance = sqrtf(dx * dx + dy * dy);

                if (filled) {
                    if (distance <= radius - 0.5f) {
                        SDL_RenderDrawPoint(renderer, px, py);
                    } else if (distance < radius + 0.5f) {
                        Uint8 alpha = (Uint8)(color.a * (1.0f - (distance - (radius - 0.5f))));
                        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
                        SDL_RenderDrawPoint(renderer, px, py);
                        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                    }
                } else {
                    if (distance >= radius - borderThickness && distance <= radius + 0.5f) {
                        SDL_RenderDrawPoint(renderer, px, py);
                    }
                }
            }
        }
    }

    if (!filled) {
        for (int i = 0; i < borderThickness; i++) {
            SDL_RenderDrawLine(renderer, x + radius, y + i, x + w - radius - 1, y + i);
            SDL_RenderDrawLine(renderer, x + radius, y + h - 1 - i, x + w - radius - 1, y + h - 1 - i);

            SDL_RenderDrawLine(renderer, x + i, y + radius, x + i, y + h - radius - 1);
            SDL_RenderDrawLine(renderer, x + w - 1 - i, y + radius, x + w - 1 - i, y + h - radius - 1);
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, original_mode);
}
