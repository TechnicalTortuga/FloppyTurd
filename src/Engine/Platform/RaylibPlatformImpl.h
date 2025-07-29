#pragma once

#ifndef PLATFORM_IOS

#include "PlatformDelegates.h"
// Forward declarations to avoid raylib.h dependency in header

namespace FloppyTurd {

    // Raylib Platform Implementation
    namespace RaylibPlatform {
        
        // Setup function to configure delegates for Raylib
        void SetupDelegates(PlatformDelegates& delegates);
        
        // Initialize Raylib window and systems
        bool Initialize(int screenWidth, int screenHeight, const char* title);
        void Shutdown();
        
        // Renderer functions using Raylib
        void BeginFrame();
        void EndFrame();
        void ClearScreen(float r, float g, float b, float a);
        // Delegate-compatible functions (void* sprite parameter)
        void DrawSprite(void* sprite, float x, float y, float rotation);
        void DrawSpriteScaled(void* sprite, float x, float y, float scaleX, float scaleY, float rotation);
        
        // Internal functions (const Sprite& parameter)
        void DrawSpriteInternal(const Sprite& sprite, float x, float y, float rotation);
        void DrawSpriteScaledInternal(const Sprite& sprite, float x, float y, float scaleX, float scaleY, float rotation);
        void DrawText(const char* text, float x, float y, float fontSize, float r, float g, float b, float a);
        void DrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a);
        void DrawCircle(float x, float y, float radius, float r, float g, float b, float a);
        void GetScreenSize(float* width, float* height);
        
        // Input functions using Raylib
        bool IsActionPressed(int action);
        bool IsActionJustPressed(int action);
        bool IsActionReleased(int action);
        void GetPrimaryInputPosition(float* x, float* y);
        bool IsPrimaryInputDown();
        bool IsPrimaryInputJustPressed();
        bool IsPrimaryInputJustReleased();
        int GetTouchCount();
        void GetTouchPosition(int touchIndex, float* x, float* y);
        bool IsKeyPressed(int keyCode);
        bool IsKeyJustPressed(int keyCode);
        
        // Audio functions using Raylib
        void PlaySound(const char* soundName);
        void PlaySoundWithVolume(const char* soundName, float volume);
        void StopSound(const char* soundName);
        void PlayMusic(const char* musicName);
        void StopMusic();
        void PauseMusic();
        void ResumeMusic();
        void SetMasterVolume(float volume);
        void SetSFXVolume(float volume);
        void SetMusicVolume(float volume);
        bool IsMusicPlaying();
        bool IsSoundPlaying(const char* soundName);
        
        // Utility functions
        bool ShouldClose();
        void SetTargetFPS(int fps);
        float GetFrameTime();
        
    } // namespace RaylibPlatform

} // namespace FloppyTurd

#endif // !PLATFORM_IOS