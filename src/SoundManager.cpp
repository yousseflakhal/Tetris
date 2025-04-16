#include "SoundManager.hpp"
#include <SDL2/SDL.h>
#include <stdexcept>
#include <iostream>

Mix_Chunk* SoundManager::moveSound = nullptr;
Mix_Chunk* SoundManager::holdSound = nullptr;
Mix_Chunk* SoundManager::dropSound = nullptr;
Mix_Music* SoundManager::backgroundMusic = nullptr;
Mix_Chunk* SoundManager::clearSound = nullptr;

void SoundManager::Load() {
    moveSound = Mix_LoadWAV("sounds/move.ogg");
    holdSound = Mix_LoadWAV("sounds/hold.ogg");
    dropSound = Mix_LoadWAV("sounds/drop.ogg");
    backgroundMusic = Mix_LoadMUS("sounds/bg.ogg");
    clearSound = Mix_LoadWAV("sounds/clear.ogg");

    if (!moveSound) SDL_Log("Failed to load move sound: %s", Mix_GetError());
    if (!holdSound) SDL_Log("Failed to load hold sound: %s", Mix_GetError());
    if (!dropSound) SDL_Log("Failed to load drop sound: %s", Mix_GetError());
    if (!backgroundMusic) SDL_Log("Failed to load bg music: %s", Mix_GetError());
    if (!clearSound) SDL_Log("Failed to load clear sound: %s", Mix_GetError());
}

void SoundManager::PlayMoveSound() {
    if (moveSound) {
        Mix_PlayChannel(-1, moveSound, 0);
    }
}

void SoundManager::PlayBackgroundMusic() {
    if (backgroundMusic) {
        Mix_PlayMusic(backgroundMusic, -1);
    }
}

void SoundManager::PlayHoldSound() {
    if (holdSound) Mix_PlayChannel(-1, holdSound, 0);
}

void SoundManager::PlayDropSound() {
    if (dropSound) Mix_PlayChannel(-1, dropSound, 0);
}

void SoundManager::PlayClearSound() {
    if (clearSound) Mix_PlayChannel(-1, clearSound, 0);
}

void SoundManager::CleanUp() {
    Mix_FreeChunk(moveSound);
    Mix_FreeChunk(holdSound);
    Mix_FreeChunk(dropSound);
    Mix_FreeMusic(backgroundMusic);
    Mix_FreeChunk(clearSound);
}
