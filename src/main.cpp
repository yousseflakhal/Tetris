#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>

using namespace std;

const int CELL_SIZE = 50;
const int OFFSET = 50;
const int WIDTH = 600;
const int HEIGHT = 1000;
const int SPEED = 100; // Milliseconds delay between steps

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

    // Initial position of the moving rectangle
    vector<SDL_Rect> rects;
    rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});

    SDL_RenderPresent(renderer);

    bool quit = false;
    SDL_Event event;

    Uint32 lastMoveTime = SDL_GetTicks(); // Timer to manage movement speed

    // Initialize board with zeros
    vector<vector<int>> board((HEIGHT - 2 * OFFSET) / CELL_SIZE + 1, vector<int>((WIDTH - 2 * OFFSET) / CELL_SIZE, 0));
    int x = 4, y = 1; // Initial position on the board

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
                        if (x > 0 && board[y][x - 1] == 0) { // Move left if within bounds and not occupied
                            rects.back().x -= CELL_SIZE;
                            x--;
                        }
                        break;
                    case SDLK_RIGHT:
                        if (x <= WIDTH / CELL_SIZE && board[y][x + 1] == 0) { // Move right
                            rects.back().x += CELL_SIZE;
                            x++;
                        }
                        break;
                    case SDLK_DOWN:
                        if (y < HEIGHT / CELL_SIZE - 2 && board[y + 1][x] == 0) { // Move down
                            rects.back().y += CELL_SIZE;
                            y++;
                        } else {
                            // Piece reaches bottom or another piece
                            board[y][x] = 1;
                            rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
                            x = 5;
                            y = 1;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        for(int i=0; i < board.size();i++){
            if (board[i]==vector<int>((WIDTH - (2 * OFFSET) ) / CELL_SIZE,1)){
                y++;
                board.erase(board.begin()+i);
                board.emplace(board.begin(),vector<int>((WIDTH - 2 * OFFSET ) / CELL_SIZE,0));
                auto it = rects.begin();
                while(it!= rects.end()){
                    if((*it).y / CELL_SIZE == i){
                        rects.erase(it);
                        continue;
                    }
                    ++it;
                }
                // for(auto r:board){
                //     for(auto e:r){
                //         cout << e << " ";
                //     }
                //     cout << endl;
                // }
                // cout << "**************" << endl;
                for(auto& r:rects){
                    r.y+= CELL_SIZE;
                }
                
            }
        }
        // for(auto r:board){
        //     for(auto e:r){
        //         cout << e << " ";
        //     }
        //     cout << endl;
        // }
        // for(auto r:rects){
        //     cout << r.y << endl;
        // }

        // Automatic downward movement
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime > lastMoveTime + SPEED) {
            if (y < HEIGHT / CELL_SIZE - 2 && board[y + 1][x] == 0) {
                rects.back().y += CELL_SIZE;
                y++;
            } else {
                // Piece reaches bottom or another piece
                board[y][x] = 1;

                rects.push_back(SDL_Rect{5 * CELL_SIZE, OFFSET, CELL_SIZE - 1, CELL_SIZE - 1});
                x = 4;
                y = 1;
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
            SDL_RenderFillRect(renderer, &rect);
            SDL_RenderDrawRect(renderer, &rect);
        }

        SDL_RenderPresent(renderer);
    }

    // Clean up resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
