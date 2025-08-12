#pragma once
#include <SDL2/SDL.h>
#include <cmath>
#include <algorithm>
#include <functional>

struct SDLBlendGuard {
    SDL_Renderer* renderer;
    SDL_BlendMode oldMode;
    SDLBlendGuard(SDL_Renderer* r) : renderer(r) {
        SDL_GetRenderDrawBlendMode(renderer, &oldMode);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }
    ~SDLBlendGuard() {
        SDL_SetRenderDrawBlendMode(renderer, oldMode);
    }
};

void drawAACircle(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color color);
void drawUIMenuRoundedRect(SDL_Renderer* renderer, int x, int y, int w, int h, int radius, SDL_Color color, Uint8 alpha);
void drawCardWithBorder(SDL_Renderer* renderer,int x, int y, int w, int h, int radius, SDL_Color bgColor, SDL_Color borderColor, int borderThickness);
void draw_smooth_rounded_rect(SDL_Renderer* renderer,int x, int y, int w, int h,int radius, SDL_Color color,bool filled = true, int borderThickness = 1);
SDL_Color darker(SDL_Color c, float factor = 0.55f) noexcept;
void draw_rounded_corners(int x, int y, int w, int h, int radius,
                          std::function<void(int, int, float)> pixelFunc);
void fill_solid_rounded_rect(SDL_Renderer* renderer, int x, int y, int w, int h, int radius, SDL_Color color);
void fill_gradient_rounded_rect(SDL_Renderer* renderer, int x, int y, int w, int h, int radius, SDL_Color color);
void draw_rounded_rect_border(SDL_Renderer* renderer, int x, int y, int w, int h, int radius, int borderThickness, SDL_Color color);
void draw_tetris_cell(SDL_Renderer* renderer, int x, int y, int w, int h, int radius, int margin, int borderThickness, SDL_Color outerColor, SDL_Color borderColor);
void draw_smooth_parabolic_highlight_arc(SDL_Renderer* renderer, int x, int y, int w, int h, int margin, int borderThickness);
void draw_preview_block(SDL_Renderer* r,
                               int x, int y, int w, int h,
                               SDL_Color baseCol);