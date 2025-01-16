#include "Shape.hpp"
using namespace std;

// Global random generator and distribution
random_device rd;                      // Non-deterministic random device
mt19937 gen(rd());                     // Mersenne Twister engine
uniform_int_distribution<> dis(0, 6); // Range [0, 6]

Shape::Shape(SDL_Renderer* renderer): renderer{renderer}{
    int random_value = dis(gen);
    actual_shape = random_value;
    switch(static_cast<Shapes>(actual_shape)){
        case Shapes::O:
            shape_rects.push_back(SDL_Rect{6 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{6 * CELL_SIZE, OFFSET + CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET + CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            break;
        case Shapes::I:
            shape_rects.push_back(SDL_Rect{6 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{4 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{3 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            break;
        case Shapes::S:
            shape_rects.push_back(SDL_Rect{6 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET + CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{4 * CELL_SIZE, OFFSET + CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            break;
        case Shapes::Z:
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{4 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET + CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{6 * CELL_SIZE, OFFSET + CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            break;
        case Shapes::L:
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET + CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET + 2 * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{6 * CELL_SIZE, OFFSET + 2 * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            break;
        case Shapes::J:
            shape_rects.push_back(SDL_Rect{6 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{6 * CELL_SIZE, OFFSET + CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{6 * CELL_SIZE, OFFSET + 2 * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET + 2 * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            break;
        case Shapes::T:
            shape_rects.push_back(SDL_Rect{6 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{4 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
            shape_rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET + CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1});
            break;
    }
    for(auto& rect:shape_rects){
        coords.push_back(make_pair(rect.x/CELL_SIZE - 1, ((rect.y - OFFSET) / CELL_SIZE) + 1));
    }
}

void Shape::display(){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        for (const auto& rect : shape_rects) {
            SDL_RenderFillRect(renderer, &rect);
            SDL_RenderDrawRect(renderer, &rect);
        }
}

void Shape::updateDown(){
    for(auto& rect:shape_rects){
        rect.y+= CELL_SIZE;
    }
    for(auto& coord:coords){
        ++coord.second;
    }
}

void Shape::updateLeft(){
    for(auto& rect:shape_rects){
        rect.x-= CELL_SIZE;
    }
    for(auto& coord:coords){
        --coord.first;
    }
}

void Shape::updateRight(){
    for(auto& rect:shape_rects){
        rect.x+= CELL_SIZE;
    }
    for(auto& coord:coords){
        ++coord.first;
    }
}

