#ifndef PLATFORM_IOS_H
#define PLATFORM_IOS_H

#pragma once

#include "PlatformSpecific.h"
#include "RaylibCompat.h"
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
 * PlatformIOS - iOS/Metal implementation of PlatformSpecific
 * 
 * This class provides iOS-specific implementations of all Raylib-compatible functions,
 * using Metal for rendering and iOS native APIs for other functionality.
 */
class PlatformIOS : public PlatformSpecific {
public:
    PlatformIOS();
    virtual ~PlatformIOS();

    // ============================================================================
    // INITIALIZATION AND LIFECYCLE
    // ============================================================================
    
    void Initialize(void* nativeView) override;
    void Initialize() override;
    void Shutdown() override;
    void Update() override;

    // ============================================================================
    // RENDERING FUNCTIONS (Raylib-compatible)
    // ============================================================================
    
    // Drawing primitives
    void DrawRectangle(int x, int y, int width, int height, Color color) override;
    void DrawRectangleRec(Rectangle rec, Color color) override;
    void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) override;
    void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color) override;
    void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) override;
    
    void DrawCircle(float centerX, float centerY, float radius, Color color) override;
    void DrawCircleV(Vector2 center, float radius, Color color) override;
    
    void DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color) override;
    void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) override;
    void DrawLineV(Vector2 startPos, Vector2 endPos, Color color) override;
    
    // Texture drawing
    void DrawTexture(Texture2D texture, int posX, int posY, Color tint) override;
    void DrawTextureV(Texture2D texture, Vector2 position, Color tint) override;
    void DrawTextureRec(Texture2D texture, Rectangle source, Rectangle dest, Color tint) override;
    void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) override;
    
    // Text rendering
    void DrawText(const char* text, int posX, int posY, int fontSize, Color color) override;
    void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) override;
    
    // Render texture operations
    void BeginDrawing(void* renderTexture) override;
    void EndDrawing(void* renderTexture) override;
    void* LoadRenderTexture(int width, int height) override;
    void UnloadRenderTexture(void* renderTexture) override;

    // ============================================================================
    // TEXTURE AND IMAGE FUNCTIONS
    // ============================================================================
    
    Texture2D LoadTexture(const char* fileName) override;
    void UnloadTexture(Texture2D texture) override;
    void* LoadTextureFromImage(void* imageData, int width, int height, int format) override;
    void* CreateTextureFromImage(void* image, int* width, int* height) override;
    
    Image LoadImage(const char* fileName) override;
    void UnloadImage(Image image) override;
    void* CreateSolidColorImage(int width, int height, Color color) override;

    // ============================================================================
    // AUDIO FUNCTIONS
    // ============================================================================
    
    void InitializeAudio() override;
    void ShutdownAudio() override;
    void* LoadSound(const char* fileName) override;
    void UnloadSound(void* sound) override;
    void PlaySound(void* sound) override;
    void SetSoundVolume(void* sound, float volume) override;
    
    void* LoadMusic(const char* fileName) override;
    void UnloadMusic(void* music) override;
    void PlayMusic(void* music) override;
    void StopMusic(void* music) override;
    void UpdateMusic(void* music) override;
    bool IsMusicPlaying(void* music) override;
    void SetMusicVolume(void* music, float volume) override;
    
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
    
    void* LoadFont(const char* fileName, int size) override;
    void UnloadFont(void* font) override;
    Vector2 MeasureText(const char* text, void* font, float fontSize, float spacing) override;
    void DrawText(const char* text, float x, float y, float fontSize, Color color, void* font) override;

    // ============================================================================
    // INPUT FUNCTIONS
    // ============================================================================
    
    // Primary input (primary touch on mobile)
    bool IsPrimaryInputDown() override;
    bool IsPrimaryInputPressed() override;
    bool IsPrimaryInputReleased() override;
    Vector2 GetPrimaryInputPosition() override;
    
    // Secondary input
    bool IsSecondaryInputDown() override;
    bool IsSecondaryInputPressed() override;
    bool IsSecondaryInputReleased() override;
    
    // Touch-specific (mobile)
    bool IsTouchSupported() override;
    int GetTouchCount() override;
    Vector2 GetTouchPosition(int index) override;
    std::vector<Vector2> GetTouchPoints() override;

    // ============================================================================
    // SCREEN AND WINDOW FUNCTIONS
    // ============================================================================
    
    int GetScreenWidth() override;
    int GetScreenHeight() override;
    Vector2 GetScreenSize() override;
    float GetScreenDensity() override;
    float GetScreenScale() override;
    Rectangle GetSafeArea() override;
    bool IsWindowFullscreen() override;
    void ToggleFullscreen() override;
    void SetWindowTitle(const std::string& title) override;
    void SetWindowSize(int width, int height) override;
    bool SupportsFullscreen() override;

    // ============================================================================
    // UTILITY FUNCTIONS
    // ============================================================================
    
    double GetCurrentTime() override;
    float GetLastFrameTime() override;
    int GetLastFPS() override;
    std::string GetResourcePath(const std::string& relativePath) override;
    std::string GetSavePath(const std::string& filename) override;
    std::string GetPlatformResourcePath(const std::string& relativePath) override;
    bool PreferLowPowerMode() override;
    int GetRecommendedTextureSize() override;
    void SetOrientation(bool landscape) override;
    void ShowVirtualKeyboard(bool show) override;
    bool IsVirtualKeyboardShown() override;
    void Vibrate(int milliseconds) override;

    // ============================================================================
    // APP LIFECYCLE
    // ============================================================================
    
    void OnAppWillResignActive() override;
    void OnAppDidBecomeActive() override;

    // ============================================================================
    // PLATFORM DETECTION
    // ============================================================================
    
    bool IsMobilePlatform() override;
    bool IsTouchSupported() const override;

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
    
    // Cache statistics
    size_t m_soundCacheHits;
    size_t m_soundCacheMisses;
    size_t m_soundCacheEvictions;
    
    // Audio session management
    bool m_audioSessionActive;
    bool m_wasInterrupted;
    bool m_wasInBackground;
    float m_preInterruptionVolume;
    bool m_preInterruptionPlaying;
    
    // Notification observers (bridged from Objective-C)
    void* m_interruptionObserver;
    void* m_routeChangeObserver;
    void* m_appStateObserver;
    
    // ============================================================================
    // AUDIO HELPER METHODS
    // ============================================================================
    
    void* CreateAVAudioPlayer(const char* fileName);
    void CleanupAVAudioPlayer(void* player);
    void ActivateAudioSession();
    void DeactivateAudioSession();
    
    // Audio session management
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
    
    // Cache statistics
    struct CacheStats {
        size_t hits;
        size_t misses;
        size_t evictions;
        size_t currentSize;
        size_t maxSize;
        float hitRate;
    };
    CacheStats GetSoundCacheStats() const;
    
    // ============================================================================
    // HELPER METHODS
    // ============================================================================
    
    MetalRenderer* GetMetalRenderer();
    GameView* GetGameView();
    void UpdateScreenMetrics();
    void UpdateInputState();
};

#endif // PLATFORM_IOS_H 