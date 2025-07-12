#ifndef PLATFORM_IOS_H
#define PLATFORM_IOS_H

#pragma once

#include "PlatformAPI.h"
#include <string>
#include <vector>
#include <unordered_map>

// Forward declarations for iOS-specific classes
class MetalRenderer;
class MetalTextRenderer;

#ifdef __OBJC__
@class GameView;
@class UIView;
#endif

/**
 * PlatformIOS - iOS/Metal implementation of Raylib-compatible functions
 * 
 * This class provides iOS-specific implementations of all Raylib-compatible functions,
 * using Metal for rendering and iOS native APIs for other functionality.
 */
class PlatformIOS {
public:
    PlatformIOS();
    virtual ~PlatformIOS();

    // ============================================================================
    // INITIALIZATION AND LIFECYCLE
    // ============================================================================
    
    void Initialize(void* nativeView);
    void Initialize();
    void Shutdown();
    void Update();

    // ============================================================================
    // RENDERING FUNCTIONS (Raylib-compatible)
    // ============================================================================
    
    // Drawing primitives
    void DrawRectangle(int x, int y, int width, int height, Color color);
    void DrawRectangleRec(Rectangle rec, Color color);
    void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);
    void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color);
    void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color);
    
    void DrawCircle(float centerX, float centerY, float radius, Color color);
    void DrawCircleV(Vector2 center, float radius, Color color);
    
    void DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color);
    void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);
    void DrawLineV(Vector2 startPos, Vector2 endPos, Color color);
    
    // Texture drawing
    void DrawTexture(Texture2D texture, int posX, int posY, Color tint);
    void DrawTextureV(Texture2D texture, Vector2 position, Color tint);
    void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
    void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
    void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
    
    // Text rendering
    void DrawText(const char* text, int posX, int posY, int fontSize, Color color);
    void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint);
    
    // Render texture operations
    void BeginDrawing(void* renderTexture);
    void EndDrawing(void* renderTexture);
    void* LoadRenderTexture(int width, int height);
    void UnloadRenderTexture(void* renderTexture);

    // ============================================================================
    // TEXTURE AND IMAGE FUNCTIONS
    // ============================================================================
    
    Texture2D LoadTexture(const char* fileName);
    void UnloadTexture(Texture2D texture);
    void* LoadTextureFromImage(void* imageData, int width, int height, int format);
    void* CreateTextureFromImage(void* image, int* width, int* height);
    
    Image LoadImage(const char* fileName);
    void UnloadImage(Image image);
    void* CreateSolidColorImage(int width, int height, Color color);

    // ============================================================================
    // AUDIO FUNCTIONS
    // ============================================================================
    
    void InitializeAudio();
    void ShutdownAudio();
    void* LoadSound(const char* fileName);
    void UnloadSound(void* sound);
    void PlaySound(void* sound);
    void SetSoundVolume(void* sound, float volume);
    
    void* LoadMusic(const char* fileName);
    void UnloadMusic(void* music);
    void PlayMusic(void* music);
    void StopMusic(void* music);
    void UpdateMusic(void* music);
    bool IsMusicPlaying(void* music);
    void SetMusicVolume(void* music, float volume);
    
    // Advanced audio functions
    void* PreloadNextTrack(const char* fileName);
    void SwitchToNextTrack();
    void ClearNextTrack();
    void StartCrossfade(float duration);
    void UpdateCrossfade(float deltaTime);
    void CompleteCrossfade();
    void FadeOutMusic(float duration);
    void FadeInMusic(float duration);
    void UpdateFade(float deltaTime);

    // ============================================================================
    // FONT AND TEXT FUNCTIONS
    // ============================================================================
    
    void* LoadFont(const char* fileName, int size);
    void UnloadFont(void* font);
    Vector2 MeasureText(const char* text, void* font, float fontSize, float spacing);
    void DrawText(const char* text, float x, float y, float fontSize, Color color, void* font);

    // ============================================================================
    // INPUT FUNCTIONS
    // ============================================================================
    
    // Primary input (primary touch on mobile)
    bool IsPrimaryInputDown();
    bool IsPrimaryInputPressed();
    bool IsPrimaryInputReleased();
    Vector2 GetPrimaryInputPosition();
    
    // Secondary input
    bool IsSecondaryInputDown();
    bool IsSecondaryInputPressed();
    bool IsSecondaryInputReleased();
    
    // Touch-specific (mobile)
    bool IsTouchSupported();
    int GetTouchCount();
    Vector2 GetTouchPosition(int index);
    std::vector<Vector2> GetTouchPoints();

    // ============================================================================
    // SCREEN AND WINDOW FUNCTIONS
    // ============================================================================
    
    int GetScreenWidth();
    int GetScreenHeight();
    Vector2 GetScreenSize();
    float GetScreenDensity();
    float GetScreenScale();
    Rectangle GetSafeArea();
    bool IsWindowFullscreen();
    void ToggleFullscreen();
    void SetWindowTitle(const std::string& title);
    void SetWindowSize(int width, int height);
    bool SupportsFullscreen();

    // ============================================================================
    // UTILITY FUNCTIONS
    // ============================================================================
    
    double GetCurrentTime();
    float GetLastFrameTime();
    int GetLastFPS();
    std::string GetResourcePath(const std::string& relativePath);
    std::string GetSavePath(const std::string& filename);
    std::string GetPlatformResourcePath(const std::string& relativePath);
    bool PreferLowPowerMode();
    int GetRecommendedTextureSize();
    void SetOrientation(bool landscape);
    void ShowVirtualKeyboard(bool show);
    bool IsVirtualKeyboardShown();
    void Vibrate(int milliseconds);

    // ============================================================================
    // APP LIFECYCLE
    // ============================================================================
    
    void OnAppWillResignActive();
    void OnAppDidBecomeActive();

    // ============================================================================
    // PLATFORM DETECTION
    // ============================================================================
    
    bool IsMobilePlatform();
    bool IsTouchSupported() const;

private:
    // iOS-specific members
    void* m_nativeView;           // UIView pointer
    MetalRenderer* m_metalRenderer; // Metal renderer instance
    MetalTextRenderer* m_textRenderer; // Text renderer instance
    
    // Input state
    std::vector<Vector2> m_touchPoints;
    bool m_primaryInputDown;
    bool m_primaryInputPressed;
    bool m_primaryInputReleased;
    bool m_secondaryInputDown;
    bool m_secondaryInputPressed;
    bool m_secondaryInputReleased;
    
    // Screen state
    int m_screenWidth;
    int m_screenHeight;
    float m_screenDensity;
    float m_screenScale;
    Rectangle m_safeArea;
    
    // Performance tracking
    double m_lastFrameTime;
    float m_frameTime;
    int m_fps;
    
    // ============================================================================
    // AUDIO MEMBER OBJECTS (Reused, not created dynamically)
    // ============================================================================
    
    // Music management
    void* m_currentMusicPlayer;  // AVAudioPlayer* (bridged)
    void* m_nextMusicPlayer;     // AVAudioPlayer* (bridged)
    void* m_fadeOutPlayer;       // AVAudioPlayer* (bridged) - for fade transitions
    bool m_isMusicPlaying;
    bool m_isMusicPaused;
    float m_musicVolume;
    bool m_musicLooping;
    
    // Transition state
    bool m_isTransitioning;
    float m_fadeProgress;
    float m_fadeDuration;
    bool m_isFadingOut;
    bool m_isFadingIn;
    
    // Sound management
    std::unordered_map<std::string, void*> m_soundCache;  // filename -> AVAudioPlayer*
    std::vector<std::string> m_soundCacheOrder;  // LRU order for cache eviction
    std::vector<void*> m_activeSounds;  // Currently playing sounds
    std::unordered_map<std::string, void*> m_loadedSounds;  // Preloaded sounds
    
    // Cache management
    static const size_t MAX_SOUND_CACHE_SIZE = 20;  // Maximum number of cached sounds
    
    // ============================================================================
    // PRIVATE HELPER FUNCTIONS
    // ============================================================================
    
    void CleanupAVAudioPlayer(void* player);
    void ActivateAudioSession();
    void DeactivateAudioSession();
    void ConfigureAudioSession();
    void HandleAudioInterruption(bool began);
    void HandleRouteChange();
    void HandleAppBackgrounding();
    void HandleAppForegrounding();
    void SaveAudioState();
    void RestoreAudioState();
    void SetupAudioSessionNotifications();
    void RemoveAudioSessionNotifications();
    
    // Cache management
    void UpdateSoundCacheOrder(const std::string& key);
    void EvictOldestSoundFromCache();
    void CleanupFinishedSounds();
    
    // Performance tracking
    struct CacheStats {
        size_t hits;
        size_t misses;
        size_t evictions;
        size_t currentSize;
        size_t maxSize;
        float hitRate;
    };
    
    CacheStats GetCacheStats() const;
    void LogCacheStats() const;
    
    // Screen and input updates
    void UpdateScreenMetrics();
    void UpdateInputState();
};

// ============================================================================
// GLOBAL PLATFORM INSTANCE
// ============================================================================

extern PlatformIOS* g_platformIOS;

// ============================================================================
// GLOBAL FUNCTIONS FOR PLATFORM ACCESS
// ============================================================================

void SetGameInstance(Game* instance);
void SetGlobalGameView(GameView* gameView);

// ============================================================================
// iOS-SPECIFIC FUNCTIONS
// ============================================================================

int game_main(int argc, char *argv[]);
void OnAppPause();
void OnAppResume();

// ============================================================================
// TOUCH AND SAFE AREA FUNCTIONS
// ============================================================================

void UpdateSafeAreaInsets(float top, float right, float bottom, float left);
void UpdateTouchState(int touchId, float x, float y, bool pressed);
void ClearAllTouchStates();

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

Texture2D CreateFallbackTexture(const char* fileName);

// ============================================================================
// RESOURCE PATH FUNCTIONS
// ============================================================================

std::string GetResourcePath(const std::string& relativePath);
std::string GetSavePath(const std::string& filename);
std::string GetPlatformResourcePath(const std::string& relativePath) const;

// ============================================================================
// PLATFORM-SPECIFIC FUNCTIONS
// ============================================================================

void ShowVirtualKeyboard(bool show);
void Vibrate(int milliseconds);

#endif // PLATFORM_IOS_H 