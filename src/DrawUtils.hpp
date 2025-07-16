#pragma once
#include <SDL2/SDL.h>
#include <cmath>
#include <algorithm>

void drawAACircle(SDL_Renderer* renderer, int cx, int cy, int radius);
void drawRoundedRect(SDL_Renderer* renderer, int x, int y, int w, int h, int radius, SDL_Color color, Uint8 alpha, bool filled = true);
void drawUIMenuRoundedRect(SDL_Renderer* renderer, int x, int y, int w, int h, int radius, SDL_Color color, Uint8 alpha);