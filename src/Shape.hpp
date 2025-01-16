#pragma once
#include <iostream>
#include <SDL2/SDL.h>
#include <vector>
#include <random>

using namespace std;

extern const int CELL_SIZE;
extern const int OFFSET;

class Shape{
    public:
    SDL_Renderer* renderer;
    enum class Shapes {O,I,S,Z,L,J,T};
    int actual_shape;
    vector<pair<int,int>> coords;
    vector<SDL_Rect> shape_rects;
    Shape(SDL_Renderer* renderer);
    void display();
    void updateDown();
    void updateLeft();
    void updateRight();
};