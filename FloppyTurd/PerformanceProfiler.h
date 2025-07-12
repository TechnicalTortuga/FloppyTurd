#pragma once
#include "PlatformAPI.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

// Types are provided by raylib.h include above

// Performance metrics for frame analysis
struct FrameMetrics {
    float frameTime = 0.0f;        // Total frame time in ms
    float updateTime = 0.0f;       // Update logic time in ms
    float drawTime = 0.0f;         // Rendering time in ms
    int drawCalls = 0;             // Number of draw calls
    size_t memoryUsage = 0;        // Memory usage in bytes
    int activeTextures = 0;        // Active texture count
    float cpuUsage = 0.0f;         // CPU usage percentage (estimated)
};

// Profiler scope for automatic timing
class ProfilerScope {
public:
    ProfilerScope(const std::string& name);
    ~ProfilerScope();
    
private:
    std::string scopeName;
    std::chrono::high_resolution_clock::time_point startTime;
};

// Draw call tracking wrapper macros and functions
namespace DrawCallTracker {
    void TrackDrawTexture(Texture2D texture, int posX, int posY, Color tint);
    void TrackDrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
    void TrackDrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
    void TrackDrawTextureV(Texture2D texture, Vector2 position, Color tint);
    void TrackDrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
    void TrackDrawRectangle(int posX, int posY, int width, int height, Color color);
    void TrackDrawText(const char* text, int posX, int posY, int fontSize, Color color);
    void TrackDrawCircle(int centerX, int centerY, float radius, Color color);
    void TrackDrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
    void TrackDrawRectangleRec(Rectangle rec, Color color);
    void TrackDrawRectangleLinesEx(Rectangle rec, float lineThick, Color color);
    void TrackClearBackground(Color color);
    
    // Reset counters each frame
    void ResetFrameCounters();
    
    // Get current frame stats
    int GetCurrentDrawCalls();
    size_t GetCurrentMemoryUsage();
    int GetCurrentActiveTextures();
}

// Performance profiler singleton
class PerformanceProfiler {
public:
    static PerformanceProfiler& GetInstance() {
        static PerformanceProfiler instance;
        return instance;
    }

    // Frame tracking
    void BeginFrame();
    void EndFrame();
    void BeginScope(const std::string& name);
    void EndScope(const std::string& name);
    
    // Manual metrics
    void IncrementDrawCalls(int count = 1);
    void SetMemoryUsage(size_t bytes);
    void SetActiveTextures(int count);
    
    // Performance analysis
    float GetAverageFrameTime() const;
    float GetAverageFPS() const;
    int GetAverageDrawCalls() const;
    size_t GetAverageMemoryUsage() const;
    
    // Configuration
    void SetTargetFPS(int fps);
    void EnableOverlay(bool enable);
    bool IsOverlayEnabled() const;
    
    // Visual overlay
    void DrawPerformanceOverlay(Vector2 position);
    
    // History tracking
    void ClearHistory();
    const std::vector<FrameMetrics>& GetFrameHistory() const;
    
private:
    PerformanceProfiler() = default;
    ~PerformanceProfiler() = default;
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;
    
    // Configuration
    int targetFPS = 60;
    bool overlayEnabled = false;
    
    // Frame tracking
    std::chrono::high_resolution_clock::time_point frameStartTime;
    bool frameInProgress = false;
    
    // Current frame data
    FrameMetrics currentFrame;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> scopeStartTimes;
    std::unordered_map<std::string, float> scopeDurations;
    
    // Historical data (last 60 frames for 1-second averages at 60fps)
    static constexpr size_t MAX_FRAME_HISTORY = 60;
    std::vector<FrameMetrics> frameHistory;
    size_t historyIndex = 0;
    
    // Helper methods
    void AddFrameToHistory(const FrameMetrics& metrics);
    float CalculateAverageFromHistory(float FrameMetrics::*member) const;
    int CalculateAverageFromHistory(int FrameMetrics::*member) const;
    size_t CalculateAverageFromHistory(size_t FrameMetrics::*member) const;
    
    // Scope tracking utilities
    void UpdateScopeMetrics();
};

// Convenience macro for scoped profiling
#define PROFILE_SCOPE(name) ProfilerScope _prof_scope(name) 