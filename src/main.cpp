#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "Shape.hpp"

using namespace std;

const int CELL_SIZE = 50;
const int OFFSET = 50;
const int WIDTH = 600;
const int HEIGHT = 1000;
const int SPEED = 100; // Milliseconds delay between steps

bool operator==(const SDL_Rect& a, const SDL_Rect& b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

pair<int,int>& mostLeft(vector<pair<int, int>>& list){
    auto& result = list[0];
    for(const auto& p:list){
        if(result.first>p.first){
            result = p;
        }
    }
    return result;
}

pair<int,int>& mostRight(vector<pair<int, int>>& list){
    auto& result = list[0];
    for(const auto& p:list){
        if(result.first<p.first){
            result = p;
        }
    }
    return result;
}

pair<int,int> mostDown(vector<pair<int, int>>& list){
    auto result = list[0];
    for(const auto& p:list){
        if(result.second<p.second){
            result = p;
        }
    }
    return result;
}

int main() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cerr << "SDL cannot be initialized: " << SDL_GetError() << endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);

    if (!window) {
        cerr << "Window cannot be created" << endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Background
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderClear(renderer);

    // Draw outer rectangle
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect outerRect = {OFFSET, OFFSET, WIDTH - 2 * OFFSET, HEIGHT - 2 * OFFSET};
    SDL_RenderDrawRect(renderer, &outerRect);

    SDL_RenderPresent(renderer);

    vector<Shape> rects;
    rects.push_back(Shape {renderer});
    rects.back().display();

    bool quit = false;
    SDL_Event event;

    Uint32 lastMoveTime = SDL_GetTicks();

    // Initialize board with zeros
    vector<vector<int>> board((HEIGHT - 2 * OFFSET) / CELL_SIZE + 1, vector<int>((WIDTH - 2 * OFFSET) / CELL_SIZE, 0));

    // Game loop
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }

            // Detect key presses
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                        if (mostLeft(rects.back().coords).first > 0 && board[mostLeft(rects.back().coords).second][mostLeft(rects.back().coords).first - 1] == 0) { // Move left if within bounds and not occupied
                            rects.back().updateLeft();
                        }
                        break;
                    case SDLK_RIGHT:
                        if (mostRight(rects.back().coords).first <= WIDTH / CELL_SIZE && board[mostRight(rects.back().coords).second][mostRight(rects.back().coords).first + 1] == 0) { // Move right
                            rects.back().updateRight();
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        int i = board.size()-1;
        
        while(i >=0){
            if (board[i]==vector<int>((WIDTH - (2 * OFFSET) ) / CELL_SIZE,1)){
                board.erase(board.begin()+i);
                board.emplace(board.begin(),vector<int>((WIDTH - 2 * OFFSET ) / CELL_SIZE,0));
                for(auto& y:board){
                    for(auto x:y){
                        cout << x << " ";
                    }
                    cout << endl;
                }
                auto it = rects.begin();
                while(it!= rects.end()){
                    for(auto& s:(*it).shape_rects){
                        if((s.y) / CELL_SIZE == i){
                            (*it).shape_rects.erase(find((*it).shape_rects.begin(),(*it).shape_rects.end(),s));
                            (*it).coords.erase(find((*it).coords.begin(),(*it).coords.end(),make_pair(s.x/CELL_SIZE - 1, ((s.y - OFFSET) / CELL_SIZE) + 1)));
                        }
                    }
                    ++it;
                }
                for(auto r = rects.begin(); r != prev(rects.end()); ++r){
                    for(auto& s:r->shape_rects){
                        if(s.y/CELL_SIZE<i){
                            s.y+= CELL_SIZE;
                        }
                    }
                }
            }else{
                i--;
            }
            
        }

        // Automatic downward movement
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime > lastMoveTime + SPEED) {
            bool collision = false;
            for(auto& coord:rects.back().coords){
                cout << coord.first << " " << coord.second << endl;
            }
            if(mostDown(rects.back().coords).second < 18){
                for(auto& coord:rects.back().coords){
                    if (board[coord.second + 1][coord.first] == 1){
                        for(auto& y:board){
                            for(auto x:y){
                                cout << x << " ";
                            }
                            cout << endl;
                        }
                        collision = true;
                        for(auto& coord:rects.back().coords){
                            board[coord.second][coord.first] = 1;
                        }
                        rects.push_back(Shape {renderer});
                        break;
                    }
                }
            }else{
                collision = true;
                for(auto& coord:rects.back().coords){
                    board[coord.second][coord.first] = 1;
                }
                rects.push_back(Shape {renderer});
            }
            if(!collision){
                rects.back().updateDown();
            }
            lastMoveTime = currentTime;
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderClear(renderer);

        // Draw outer rectangle
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &outerRect);

        // Draw all rectangles
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        for (const auto& rect : rects) {
            for(const auto& r : rect.shape_rects){
                SDL_RenderFillRect(renderer, &r);
                SDL_RenderDrawRect(renderer, &r);
            }
            
        }

        SDL_RenderPresent(renderer);
    }

    // Clean up resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
