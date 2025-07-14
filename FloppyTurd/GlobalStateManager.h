#ifndef GLOBAL_STATE_MANAGER_H
#define GLOBAL_STATE_MANAGER_H

#include <string>
#include <map>
#include <vector>

// Include PlatformTypes.h for Vector2, Rectangle, and Color definitions
#include "PlatformTypes.h"

// ============================================================================
// GLOBAL STATE MANAGER
// ============================================================================
// Handles cross-platform state that can be shared between iOS and Raylib
// Platform-specific state is handled by traits, not this manager

class GlobalStateManager {
private:
    // ============================================================================
    // AUDIO STATE (Cross-platform)
    // ============================================================================
    float m_masterVolume = 1.0f;
    bool m_audioEnabled = true;
    std::string m_currentMusicFile;
    bool m_musicLooping = false;
    float m_musicVolume = 1.0f;
    bool m_isMusicPlaying = false;
    bool m_isMusicPaused = false;
    
    // ============================================================================
    // RENDERING STATE (Cross-platform)
    // ============================================================================
    int m_screenWidth = 0;
    int m_screenHeight = 0;
    float m_screenScale = 1.0f;
    bool m_vsyncEnabled = true;
    int m_targetFPS = 60;
    bool m_windowFullscreen = false;
    
    // ============================================================================
    // INPUT STATE (Cross-platform)
    // ============================================================================
    Vector2 m_mousePosition = {0, 0};
    Vector2 m_mouseDelta = {0, 0};
    bool m_primaryInputDown = false;
    bool m_primaryInputPressed = false;
    bool m_primaryInputReleased = false;
    std::vector<Vector2> m_touchPoints;
    
    // ============================================================================
    // UTILITY STATE (Cross-platform)
    // ============================================================================
    unsigned int m_randomSeed = 0;
    int m_traceLogLevel = 3; // LOG_INFO
    unsigned int m_configFlags = 0;
    std::string m_resourcePath;
    bool m_lowPowerMode = false;
    int m_recommendedTextureSize = 1024;
    
    // ============================================================================
    // PLATFORM-SPECIFIC STATE POINTERS
    // ============================================================================
    // These are opaque pointers that traits can use to store platform-specific state
    void* m_platformAudioState = nullptr;
    void* m_platformRenderState = nullptr;
    void* m_platformInputState = nullptr;
    
    // Singleton instance
    static GlobalStateManager* s_instance;
    
    // Private constructor for singleton
    GlobalStateManager() = default;
    
public:
    // ============================================================================
    // SINGLETON ACCESS
    // ============================================================================
    static GlobalStateManager& GetInstance() {
        static GlobalStateManager instance;
        return instance;
    }
    
    // ============================================================================
    // AUDIO STATE MANAGEMENT
    // ============================================================================
    float GetMasterVolume() const { return m_masterVolume; }
    void SetMasterVolume(float volume) { m_masterVolume = volume; }
    
    bool IsAudioEnabled() const { return m_audioEnabled; }
    void SetAudioEnabled(bool enabled) { m_audioEnabled = enabled; }
    
    const std::string& GetCurrentMusicFile() const { return m_currentMusicFile; }
    void SetCurrentMusicFile(const std::string& file) { m_currentMusicFile = file; }
    
    bool IsMusicLooping() const { return m_musicLooping; }
    void SetMusicLooping(bool looping) { m_musicLooping = looping; }
    
    float GetMusicVolume() const { return m_musicVolume; }
    void SetMusicVolume(float volume) { m_musicVolume = volume; }
    
    bool IsMusicPlaying() const { return m_isMusicPlaying; }
    void SetMusicPlaying(bool playing) { m_isMusicPlaying = playing; }
    
    bool IsMusicPaused() const { return m_isMusicPaused; }
    void SetMusicPaused(bool paused) { m_isMusicPaused = paused; }
    
    // ============================================================================
    // RENDERING STATE MANAGEMENT
    // ============================================================================
    int GetScreenWidth() const { return m_screenWidth; }
    void SetScreenWidth(int width) { m_screenWidth = width; }
    
    int GetScreenHeight() const { return m_screenHeight; }
    void SetScreenHeight(int height) { m_screenHeight = height; }
    
    float GetScreenScale() const { return m_screenScale; }
    void SetScreenScale(float scale) { m_screenScale = scale; }
    
    bool IsVSyncEnabled() const { return m_vsyncEnabled; }
    void SetVSyncEnabled(bool enabled) { m_vsyncEnabled = enabled; }
    
    int GetTargetFPS() const { return m_targetFPS; }
    void SetTargetFPS(int fps) { m_targetFPS = fps; }
    
    bool IsWindowFullscreen() const { return m_windowFullscreen; }
    void SetWindowFullscreen(bool fullscreen) { m_windowFullscreen = fullscreen; }
    
    // ============================================================================
    // RENDERING STATE QUERIES (No rendering operations - just state queries)
    // ============================================================================
    Vector2 GetScreenCenter() const;
    Vector2 GetRenderScale() const;
    Rectangle GetSafeArea() const;
    float GetScreenDensity() const;
    bool IsLandscape() const;
    bool IsPortrait() const;
    
    // ============================================================================
    // INPUT STATE MANAGEMENT
    // ============================================================================
    Vector2 GetMousePosition() const { return m_mousePosition; }
    void SetMousePosition(Vector2 position) { m_mousePosition = position; }
    
    Vector2 GetMouseDelta() const { return m_mouseDelta; }
    void SetMouseDelta(Vector2 delta) { m_mouseDelta = delta; }
    
    bool IsPrimaryInputDown() const { return m_primaryInputDown; }
    void SetPrimaryInputDown(bool down) { m_primaryInputDown = down; }
    
    bool IsPrimaryInputPressed() const { return m_primaryInputPressed; }
    void SetPrimaryInputPressed(bool pressed) { m_primaryInputPressed = pressed; }
    
    bool IsPrimaryInputReleased() const { return m_primaryInputReleased; }
    void SetPrimaryInputReleased(bool released) { m_primaryInputReleased = released; }
    
    Vector2 GetPrimaryInputPosition() const { return m_mousePosition; } // On iOS, primary input position is the same as mouse position
    void SetPrimaryInputPosition(Vector2 position) { m_mousePosition = position; }
    
    const std::vector<Vector2>& GetTouchPoints() const { return m_touchPoints; }
    void SetTouchPoints(const std::vector<Vector2>& points) { m_touchPoints = points; }
    
    // ============================================================================
    // UTILITY STATE MANAGEMENT
    // ============================================================================
    unsigned int GetRandomSeed() const { return m_randomSeed; }
    void SetRandomSeed(unsigned int seed) { m_randomSeed = seed; }
    
    int GetTraceLogLevel() const { return m_traceLogLevel; }
    void SetTraceLogLevel(int level) { m_traceLogLevel = level; }
    
    unsigned int GetConfigFlags() const { return m_configFlags; }
    void SetConfigFlags(unsigned int flags) { m_configFlags = flags; }
    
    const std::string& GetResourcePath() const { return m_resourcePath; }
    void SetResourcePath(const std::string& path) { m_resourcePath = path; }
    
    bool IsLowPowerMode() const { return m_lowPowerMode; }
    void SetLowPowerMode(bool enabled) { m_lowPowerMode = enabled; }
    
    int GetRecommendedTextureSize() const { return m_recommendedTextureSize; }
    void SetRecommendedTextureSize(int size) { m_recommendedTextureSize = size; }
    
    // ============================================================================
    // PLATFORM-SPECIFIC STATE ACCESS
    // ============================================================================
    // Traits can use these to store/access platform-specific state
    void* GetPlatformAudioState() const { return m_platformAudioState; }
    void SetPlatformAudioState(void* state) { m_platformAudioState = state; }
    
    void* GetPlatformRenderState() const { return m_platformRenderState; }
    void SetPlatformRenderState(void* state) { m_platformRenderState = state; }
    
    void* GetPlatformInputState() const { return m_platformInputState; }
    void SetPlatformInputState(void* state) { m_platformInputState = state; }
    
    // ============================================================================
    // RENDERING STATE SYNCHRONIZATION
    // ============================================================================
    // These methods are called by platform-specific code when rendering state changes
    
    void UpdateScreenDimensions(int width, int height) {
        m_screenWidth = width;
        m_screenHeight = height;
    }
    
    void UpdateScreenScale(float scale) {
        m_screenScale = scale;
    }
    
    void UpdateSafeArea([[maybe_unused]] Rectangle safeArea) {
        // Store safe area in platform-specific state for now
        // Could add a dedicated safe area member if needed
        // Parameter is currently unused but kept for future implementation
    }
    
    void UpdateTargetFPS(int fps) {
        m_targetFPS = fps;
    }
    
    void UpdateVSyncEnabled(bool enabled) {
        m_vsyncEnabled = enabled;
    }
    
    void UpdateWindowFullscreen(bool fullscreen) {
        m_windowFullscreen = fullscreen;
    }
    
    // ============================================================================
    // INITIALIZATION AND CLEANUP
    // ============================================================================
    void Initialize() {
        // Initialize default state
        m_masterVolume = 1.0f;
        m_audioEnabled = true;
        m_screenScale = 1.0f;
        m_vsyncEnabled = true;
        m_targetFPS = 60;
        m_traceLogLevel = 3; // LOG_INFO
        m_recommendedTextureSize = 1024;
    }
    
    void Shutdown() {
        // Clear platform-specific state pointers
        m_platformAudioState = nullptr;
        m_platformRenderState = nullptr;
        m_platformInputState = nullptr;
        
        // Clear dynamic state
        m_touchPoints.clear();
        m_currentMusicFile.clear();
        m_resourcePath.clear();
    }
};

// ============================================================================
// C-STYLE WRAPPER FUNCTIONS FOR iOS INTEGRATION
// ============================================================================
// These allow GameView.mm to update GlobalStateManager without C++ linkage issues

extern "C" {
    // Input state setters for iOS GameView
    void GlobalStateManager_SetPrimaryInputDown(bool down);
    void GlobalStateManager_SetPrimaryInputPressed(bool pressed);
    void GlobalStateManager_SetPrimaryInputReleased(bool released);
    void GlobalStateManager_SetTouchPoints(const Vector2* points, int count);
    void GlobalStateManager_SetMousePosition(float x, float y);
    void GlobalStateManager_SetMouseDelta(float x, float y);
}

// Convenience macro for accessing the global state manager
#define GLOBAL_STATE GlobalStateManager::GetInstance()

#endif // GLOBAL_STATE_MANAGER_H 