#pragma once
#include <SDL2/SDL_mixer.h>
#include <string>

class SoundManager {
    public:
        static Mix_Chunk* moveSound;
        static Mix_Chunk* holdSound;
        static Mix_Chunk* dropSound;
        static Mix_Music* backgroundMusic;
        static Mix_Chunk* clearSound;
    
        static void Load();
        static void CleanUp();
    
        static void PlayMoveSound();
        static void PlayHoldSound();
        static void PlayDropSound();
        static void PlayBackgroundMusic();
        static void PauseBackgroundMusic();
        static void ResumeBackgroundMusic();
        static void RestartBackgroundMusic();
        static void StopBackgroundMusic();
        static void PlayClearSound();
    };