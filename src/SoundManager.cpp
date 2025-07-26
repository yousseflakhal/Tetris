#include "SoundManager.hpp"
#include <SDL2/SDL.h>
#include <stdexcept>
#include <iostream>

Mix_Chunk* SoundManager::moveSound = nullptr;
Mix_Chunk* SoundManager::holdSound = nullptr;
Mix_Chunk* SoundManager::dropSound = nullptr;
Mix_Music* SoundManager::backgroundMusic = nullptr;
Mix_Music* SoundManager::gameOverMusic = nullptr;
Mix_Chunk* SoundManager::clearSound = nullptr;

void SoundManager::Load() {
    moveSound = Mix_LoadWAV("sounds/move.ogg");
    holdSound = Mix_LoadWAV("sounds/hold.ogg");
    dropSound = Mix_LoadWAV("sounds/drop.ogg");
    backgroundMusic = Mix_LoadMUS("sounds/bg.ogg");
    clearSound = Mix_LoadWAV("sounds/clear.ogg");
    gameOverMusic = Mix_LoadMUS("sounds/gameover.ogg");

    if (!moveSound) SDL_Log("Failed to load move sound: %s", Mix_GetError());
    if (!holdSound) SDL_Log("Failed to load hold sound: %s", Mix_GetError());
    if (!dropSound) SDL_Log("Failed to load drop sound: %s", Mix_GetError());
    if (!backgroundMusic) SDL_Log("Failed to load bg music: %s", Mix_GetError());
    if (!clearSound) SDL_Log("Failed to load clear sound: %s", Mix_GetError());
    if (!gameOverMusic) SDL_Log("Failed to load game over music: %s", Mix_GetError());
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

void SoundManager::PauseBackgroundMusic() {
    if (Mix_PlayingMusic()) {
        Mix_PauseMusic();
    }
}

void SoundManager::ResumeBackgroundMusic() {
    if (!Mix_PlayingMusic()) {
        PlayBackgroundMusic();
    } else {
        Mix_ResumeMusic();
    }
}

void SoundManager::RestartBackgroundMusic() {
    if (Mix_PlayingMusic()) {
        Mix_HaltMusic();
    }
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

void SoundManager::PlayGameOverMusic() {
    if (Mix_PlayingMusic()) Mix_HaltMusic();
    if (gameOverMusic) Mix_PlayMusic(gameOverMusic, 1);
}

void SoundManager::StopGameOverMusic() {
    if (Mix_PlayingMusic()) {
        Mix_HaltMusic();
    }
}

void SoundManager::CleanUp() {
    if (moveSound)       { Mix_FreeChunk(moveSound); moveSound = nullptr; }
    if (holdSound)       { Mix_FreeChunk(holdSound); holdSound = nullptr; }
    if (dropSound)       { Mix_FreeChunk(dropSound); dropSound = nullptr; }
    if (clearSound)      { Mix_FreeChunk(clearSound); clearSound = nullptr; }
    if (backgroundMusic) { Mix_FreeMusic(backgroundMusic); backgroundMusic = nullptr; }
    if (gameOverMusic)   { Mix_FreeMusic(gameOverMusic); gameOverMusic = nullptr; }
}