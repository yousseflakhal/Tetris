#include "DrawUtils.hpp"

void drawAACircle(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color color) {
    if (radius <= 0) return;

    SDL_BlendMode prev;
    SDL_GetRenderDrawBlendMode(renderer, &prev);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    const Uint8 baseR = color.r, baseG = color.g, baseB = color.b, baseA = color.a;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            float d = std::sqrt(float(dx*dx + dy*dy));
            if (d <= radius) {
                float edge = radius - d;
                float aFrac = std::clamp(edge, 0.0f, 1.0f);
                Uint8 a = static_cast<Uint8>(baseA * aFrac);
                if (a == 0) continue;
                SDL_SetRenderDrawColor(renderer, baseR, baseG, baseB, a);
                SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
            }
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, prev);
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
    draw_smooth_rounded_rect(
        renderer,
        x, y,
        w, h,
        radius,
        borderColor,
        false,
        borderThickness
    );

    draw_smooth_rounded_rect(
        renderer,
        x + borderThickness,
        y + borderThickness,
        w - 2 * borderThickness,
        h - 2 * borderThickness,
        std::max(0, radius - borderThickness),
        bgColor,
        true
    );
}

void draw_smooth_rounded_rect(SDL_Renderer* renderer,
                              int x, int y, int w, int h,
                              int radius, SDL_Color color,
                              bool filled, int borderThickness) 
{
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

SDL_Color darker(SDL_Color c, float factor) {
    SDL_Color out;
    out.r = Uint8(c.r * factor);
    out.g = Uint8(c.g * factor);
    out.b = Uint8(c.b * factor);
    out.a = c.a;
    return out;
}

template <typename PixelFunc>
void draw_rounded_corners(int x, int y, int w, int h, int radius, PixelFunc&& pixelFunc)
{
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
        int end_x = (corner % 2 == 0) ? x + radius : x + w;
        int start_y = (corner < 2) ? y : y + h - radius;
        int end_y = (corner < 2) ? y + radius : y + h;
        for (int py = start_y; py < end_y; py++) {
            for (int px = start_x; px < end_x; px++) {
                float dx = px - cx + 0.5f;
                float dy = py - cy + 0.5f;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist <= radius - 0.5f)
                    pixelFunc(px, py, 1.0f);
                else if (dist < radius + 0.5f)
                    pixelFunc(px, py, 1.0f - (dist - (radius - 0.5f)));
            }
        }
    }
}

void fill_solid_rounded_rect(SDL_Renderer* renderer, int x, int y, int w, int h, int radius, SDL_Color color)
{
    SDLBlendGuard _guard(renderer);
    SDL_Rect center = {x + radius, y, w - 2 * radius, h};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &center);
    SDL_Rect side = {x, y + radius, w, h - 2 * radius};
    SDL_RenderFillRect(renderer, &side);

    draw_rounded_corners(x, y, w, h, radius, [&](int px, int py, float alpha_frac) {
        Uint8 alpha = (Uint8)(color.a * alpha_frac);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
        SDL_RenderDrawPoint(renderer, px, py);
    });
}

inline void compute_gradient_color(int px, int py, int centerX, int centerY, float maxDistance,
                                  const SDL_Color& start, const SDL_Color& end,
                                  Uint8& r, Uint8& g, Uint8& b) {
    float dx = px - centerX;
    float dy = py - centerY;
    float distance = std::sqrt(dx * dx + dy * dy);
    float ratio = distance / maxDistance;
    if (ratio > 1.0f) ratio = 1.0f;
    r = static_cast<Uint8>(start.r * (1 - ratio) + end.r * ratio);
    g = static_cast<Uint8>(start.g * (1 - ratio) + end.g * ratio);
    b = static_cast<Uint8>(start.b * (1 - ratio) + end.b * ratio);
}

void fill_gradient_rounded_rect(SDL_Renderer* renderer, int x, int y, int w, int h, int radius, SDL_Color color)
{
    SDLBlendGuard _guard(renderer);

    SDL_Color centerColor = { 
        Uint8(255 - (255 - color.r) / 2), 
        Uint8(255 - (255 - color.g) / 2), 
        Uint8(255 - (255 - color.b) / 2), 
        color.a 
    };

    int centerX = x + w / 2;
    int centerY = y + h;
    float maxDistance = std::sqrt((w / 2.0f) * (w / 2.0f) + h * h);

    for (int py = y; py < y + h; py++)
        for (int px = x + radius; px < x + w - radius; px++) {
            Uint8 r, g, b;
            compute_gradient_color(px, py, centerX, centerY, maxDistance, centerColor, color, r, g, b);
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, px, py);
        }
    for (int py = y + radius; py < y + h - radius; py++) {
        for (int px = x; px < x + radius; px++) {
            Uint8 r, g, b;
            compute_gradient_color(px, py, centerX, centerY, maxDistance, centerColor, color, r, g, b);
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, px, py);
        }
        for (int px = x + w - radius; px < x + w; px++) {
            Uint8 r, g, b;
            compute_gradient_color(px, py, centerX, centerY, maxDistance, centerColor, color, r, g, b);
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, px, py);
        }
    }
    draw_rounded_corners(x, y, w, h, radius, [&](int px, int py, float alpha_frac) {
        Uint8 r, g, b;
        compute_gradient_color(px, py, centerX, centerY, maxDistance, centerColor, color, r, g, b);
        SDL_SetRenderDrawColor(renderer, r, g, b, (Uint8)(255 * alpha_frac));
        SDL_RenderDrawPoint(renderer, px, py);
    });
}

void draw_rounded_rect_border(SDL_Renderer* renderer, int x, int y, int w, int h,
                              int radius, int borderThickness, SDL_Color color)
{
    SDLBlendGuard _guard(renderer);
    int innerRadius = radius - borderThickness;
    draw_rounded_corners(x, y, w, h, radius, [&](int px, int py, float alpha_frac) {
        float dx, dy, dist;
        int cx = (px < x + w/2) ? x + radius : x + w - radius;
        int cy = (py < y + h/2) ? y + radius : y + h - radius;
        dx = px - cx + 0.5f; dy = py - cy + 0.5f;
        dist = std::sqrt(dx*dx + dy*dy);
        if (dist < innerRadius - 0.5f)
            return;
        float blend = alpha_frac;
        if (dist < innerRadius + 0.5f)
            blend *= 1.0f - ((innerRadius + 0.5f) - dist);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, (Uint8)(color.a * blend));
        SDL_RenderDrawPoint(renderer, px, py);
    });
    for (int py = y + radius; py < y + h - radius; py++) {
        for (int i = 0; i < borderThickness; i++) {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderDrawPoint(renderer, x + i, py);
            SDL_RenderDrawPoint(renderer, x + w - 1 - i, py);
        }
    }
    for (int px = x + radius; px < x + w - radius; px++) {
        for (int i = 0; i < borderThickness; i++) {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderDrawPoint(renderer, px, y + i);
            SDL_RenderDrawPoint(renderer, px, y + h - 1 - i);
        }
    }
}

void draw_tetris_cell(SDL_Renderer* renderer,
                      int x, int y, int w, int h,
                      int radius, int margin, int borderThickness,
                      SDL_Color outerColor, SDL_Color borderColor)
{
    fill_solid_rounded_rect(renderer, x, y, w, h, radius, outerColor);

    int borderX = x + margin;
    int borderY = y + margin;
    int borderW = w - 2 * margin;
    int borderH = h - 2 * margin;
    int borderRadius = radius - margin;

    int gradX = borderX + borderThickness;
    int gradY = borderY + borderThickness;
    int gradW = borderW - 2 * borderThickness;
    int gradH = borderH - 2 * borderThickness;
    int gradRadius = borderRadius - borderThickness;

    if (borderW > 0 && borderH > 0 && borderRadius > 0 && borderThickness > 0)
        draw_rounded_rect_border(renderer, borderX, borderY, borderW, borderH, borderRadius, borderThickness, borderColor);

    if (gradW > 0 && gradH > 0 && gradRadius > 0)
        fill_gradient_rounded_rect(renderer, gradX, gradY, gradW, gradH, gradRadius, outerColor);
}

void draw_smooth_parabolic_highlight_arc(SDL_Renderer* renderer,
                                         int x, int y, int w, int h,
                                         int margin, int borderThickness)
{
    int gradX = x + margin + borderThickness;
    int gradY = y + margin + borderThickness;
    int gradW = w - 2 * (margin + borderThickness);
    int gradH = h - 2 * (margin + borderThickness);

    if (gradW <= 0 || gradH <= 0) return;

    int topY = gradY;
    int arcBottomY = gradY + gradH / 4;
    int arcCenterX = gradX + gradW / 2;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    int thickness = std::max(2, gradH / 18);

    float fadeTopY = static_cast<float>(topY);
    float fadeBottomY = static_cast<float>(arcBottomY);

    for (int py = topY; py <= arcBottomY; py++) {
        float t = static_cast<float>(py - topY) / (arcBottomY - topY);
        float halfWidth = (gradW / 2.0f) * (1.0f - t * t);
        
        float vFade = 1.0f;
        float pixelCenterY = py + 0.5f;
        float dTop = pixelCenterY - fadeTopY;
        float dBottom = fadeBottomY - pixelCenterY;
        vFade = std::min(dTop, dBottom);
        vFade = std::clamp(vFade, 0.0f, 1.0f);
        
        int leftBound = static_cast<int>(arcCenterX - halfWidth - thickness);
        int rightBound = static_cast<int>(arcCenterX + halfWidth + thickness);
        
        for (int px = leftBound; px <= rightBound; px++) {
            float dx = static_cast<float>(px) + 0.5f - arcCenterX;
            float horizontalDist = std::abs(dx) - halfWidth;
            float dist = std::max(0.0f, horizontalDist);
            
            float alpha_factor = 0.0f;
            if (dist < thickness) {
                alpha_factor = 1.0f;
                if (dist > thickness - 1.0f) {
                    alpha_factor = 1.0f - (dist - (thickness - 1.0f));
                }
                alpha_factor *= vFade;
                
                Uint8 alpha = static_cast<Uint8>(128 * alpha_factor);
                if (alpha > 0) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
                    SDL_RenderDrawPoint(renderer, px, py);
                }
            }
        }
    }
}

void draw_preview_block(SDL_Renderer* r,
                               int x, int y, int w, int h,
                               SDL_Color baseCol)
{
    SDL_Color borderCol = darker(baseCol, 0.55f);
    constexpr int radius          = 4;
    constexpr int margin          = 1;
    constexpr int borderThickness = 2;

    draw_tetris_cell(r, x, y, w, h,
                     radius, margin, borderThickness,
                     baseCol, borderCol);

    draw_smooth_parabolic_highlight_arc(r, x, y, w, h,
                                        margin, borderThickness);
}