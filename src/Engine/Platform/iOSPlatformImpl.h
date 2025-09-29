#pragma once

#ifdef PLATFORM_IOS

#include "PlatformDelegates.h"

// Swift 5.9+ Native C++ Interop Support
#if __has_include(<swift/bridging>)
#include <swift/bridging>
#endif

namespace GameCore {

    // Forward declarations for Swift classes (native interop)
    class MetalRenderer;
    class TouchInputHandler;
    class AVAudioHandler;

    // iOS Platform Implementation using native Swift/C++ interop
    namespace iOSPlatform {
        
        // Setup function to configure delegates for iOS
        void SetupDelegates(PlatformDelegates& delegates);
        
        // Renderer functions that call Swift MetalRenderer directly
        void BeginFrame();
        void EndFrame();
        void ClearScreen(float r, float g, float b, float a);
        void DrawSprite(void* sprite, float x, float y, float rotation);
        void DrawSpriteScaled(void* sprite, float x, float y, float scaleX, float scaleY, float rotation);
        void DrawSpriteScaledPivoted(void* sprite, float x, float y, float scaleX, float scaleY, float rotation, float pivotX, float pivotY);
        void DrawText(const std::string& text, float x, float y, float fontSize, float r, float g, float b, float a);
        void DrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a);
        void DrawCircle(float x, float y, float radius, float r, float g, float b, float a);
        void GetScreenSize(float* width, float* height);
        void GetScreenInfo(ScreenInfo* screenInfo);
        void LockOrientation();
        void UnlockOrientation();
        void LockToPortrait();
        void LockToLandscape();
        
        // Input functions that call Swift TouchInputHandler directly
        bool IsActionPressed(int action);
        bool IsActionJustPressed(int action);
        bool IsActionReleased(int action);
        void GetPrimaryInputPosition(float* x, float* y);
        bool IsPrimaryInputDown();
        bool IsPrimaryInputJustPressed();
        bool IsPrimaryInputJustReleased();
        int GetTouchCount();
        void GetTouchPosition(int touchIndex, float* x, float* y);
        bool IsTouchDown();
        bool IsTouchJustPressed();
        bool IsTouchJustReleased();
        bool IsKeyPressed(int keyCode);  // Stubbed for iOS
        bool IsKeyJustPressed(int keyCode);  // Stubbed for iOS
        
        // Audio functions that call Swift AVAudioHandler directly
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
        
        // Asset loading functions
        void LoadTexture(const char* texturePath, void (*callback)(bool success, const char* error));
        void LoadAudio(const char* audioPath, void (*callback)(bool success, const char* error));
        void LoadFont(const char* fontPath, int size, void (*callback)(bool success, const char* error));
        void LoadShader(const char* vertexPath, const char* fragmentPath, void (*callback)(bool success, const char* error));
        void LoadData(const char* dataPath, void (*callback)(bool success, const char* error));
        
        // Asset management
        const char* GetAssetPath(const char* relativePath);
        bool FileExists(const char* relativePath);

        // Asset cache management delegates
        void preloadEssentialAssets();
        bool isCached(const char* assetName, int type);

        // Platform delegates initialization for Swift/C++ interop
        void InitializePlatformDelegates();
    } // namespace iOSPlatform

} // namespace GameCore

#endif // PLATFORM_IOS