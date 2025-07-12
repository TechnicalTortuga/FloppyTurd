#include "PerformanceProfiler.h"
#include "AIGUI.h"
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include "PlatformAPI.h"
#include <cstdint>

// DrawCallTracker implementation
namespace DrawCallTracker {
    static int frameDrawCalls = 0;
    static std::unordered_set<uintptr_t> activeTextureIDs;
    static size_t frameMemoryUsage = 0;
    
    void TrackDrawTexture(Texture2D texture, int posX, int posY, Color tint) {
        frameDrawCalls++;
        #if defined(__APPLE__) && TARGET_OS_IPHONE
            activeTextureIDs.insert(reinterpret_cast<uintptr_t>(texture.id));
        #else
            activeTextureIDs.insert(texture.id);
        #endif
        
        // Call the actual Raylib function
        ::DrawTexture(texture, posX, posY, tint);
    }
    
    void TrackDrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
        frameDrawCalls++;
        #if defined(__APPLE__) && TARGET_OS_IPHONE
            activeTextureIDs.insert(reinterpret_cast<uintptr_t>(texture.id));
        #else
            activeTextureIDs.insert(texture.id);
        #endif
        
        // Call the actual Raylib function
        ::DrawTexturePro(texture, source, dest, origin, rotation, tint);
    }
    
    void TrackDrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) {
        frameDrawCalls++;
        #if defined(__APPLE__) && TARGET_OS_IPHONE
            activeTextureIDs.insert(reinterpret_cast<uintptr_t>(texture.id));
        #else
            activeTextureIDs.insert(texture.id);
        #endif
        
        // Call the actual Raylib function
        ::DrawTextureRec(texture, source, position, tint);
    }
    
    void TrackDrawTextureV(Texture2D texture, Vector2 position, Color tint) {
        frameDrawCalls++;
        #if defined(__APPLE__) && TARGET_OS_IPHONE
            activeTextureIDs.insert(reinterpret_cast<uintptr_t>(texture.id));
        #else
            activeTextureIDs.insert(texture.id);
        #endif
        
        // Call the actual Raylib function
        ::DrawTextureV(texture, position, tint);
    }
    
    void TrackDrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) {
        frameDrawCalls++;
        #if defined(__APPLE__) && TARGET_OS_IPHONE
            activeTextureIDs.insert(reinterpret_cast<uintptr_t>(texture.id));
        #else
            activeTextureIDs.insert(texture.id);
        #endif
        
        // Call the actual Raylib function
        ::DrawTextureEx(texture, position, rotation, scale, tint);
    }
    
    void TrackDrawRectangle(int posX, int posY, int width, int height, Color color) {
        frameDrawCalls++;
        
        // Call the actual Raylib function
        ::DrawRectangle(posX, posY, width, height, color);
    }
    
    void TrackDrawText(const char* text, int posX, int posY, int fontSize, Color color) {
        frameDrawCalls++;
        
        // Call the actual Raylib function
        ::DrawText(text, posX, posY, fontSize, color);
    }
    
    void TrackDrawCircle(int centerX, int centerY, float radius, Color color) {
        frameDrawCalls++;
        
        // Call the actual Raylib function
        ::DrawCircle(centerX, centerY, radius, color);
    }
    
    void TrackDrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) {
        frameDrawCalls++;
        
        // Call the actual Raylib function
        ::DrawLine(startPosX, startPosY, endPosX, endPosY, color);
    }
    
    void TrackDrawRectangleRec(Rectangle rec, Color color) {
        frameDrawCalls++;
        
        // Call the actual Raylib function
        ::DrawRectangleRec(rec, color);
    }
    
    void TrackDrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) {
        frameDrawCalls++;
        
        // Call the actual Raylib function
        ::DrawRectangleLinesEx(rec, lineThick, color);
    }
    
    void TrackClearBackground(Color color) {
        frameDrawCalls++;
        
        // Call the actual Raylib function
        ::ClearBackground(color);
    }
    
    void ResetFrameCounters() {
        frameDrawCalls = 0;
        activeTextureIDs.clear();
        frameMemoryUsage = 0;
        
        // Calculate current memory usage from active textures
        // This is a rough estimate based on typical texture sizes
        frameMemoryUsage = activeTextureIDs.size() * 1024 * 1024; // ~1MB per texture estimate
    }
    
    int GetCurrentDrawCalls() {
        return frameDrawCalls;
    }
    
    size_t GetCurrentMemoryUsage() {
        return frameMemoryUsage;
    }
    
    int GetCurrentActiveTextures() {
        return static_cast<int>(activeTextureIDs.size());
    }
}

// Profiler scope implementation
ProfilerScope::ProfilerScope(const std::string& name) : scopeName(name) {
    PerformanceProfiler::GetInstance().BeginScope(name);
    startTime = std::chrono::high_resolution_clock::now();
}

ProfilerScope::~ProfilerScope() {
    PerformanceProfiler::GetInstance().EndScope(scopeName);
}

// Main profiler implementation
void PerformanceProfiler::BeginFrame() {
    frameStartTime = std::chrono::high_resolution_clock::now();
    frameInProgress = true;
    
    // Reset current frame metrics
    currentFrame = FrameMetrics{};
    scopeDurations.clear();
    
    // Reset draw call tracking for this frame
    DrawCallTracker::ResetFrameCounters();
}

void PerformanceProfiler::EndFrame() {
    if (!frameInProgress) return;
    
    auto frameEndTime = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(frameEndTime - frameStartTime);
    currentFrame.frameTime = frameDuration.count() / 1000.0f; // Convert to milliseconds
    
    // Update scope-based timing
    UpdateScopeMetrics();
    
    // Get real tracking data
    currentFrame.drawCalls = DrawCallTracker::GetCurrentDrawCalls();
    currentFrame.memoryUsage = DrawCallTracker::GetCurrentMemoryUsage();
    currentFrame.activeTextures = DrawCallTracker::GetCurrentActiveTextures();
    
    // Estimate CPU usage (very rough approximation)
    float targetFrameTime = 1000.0f / targetFPS;
    currentFrame.cpuUsage = std::min(100.0f, (currentFrame.frameTime / targetFrameTime) * 100.0f);
    
    AddFrameToHistory(currentFrame);
    frameInProgress = false;
}

void PerformanceProfiler::BeginScope(const std::string& name) {
    scopeStartTimes[name] = std::chrono::high_resolution_clock::now();
}

void PerformanceProfiler::EndScope(const std::string& name) {
    auto it = scopeStartTimes.find(name);
    if (it != scopeStartTimes.end()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - it->second);
        scopeDurations[name] = duration.count() / 1000.0f; // Convert to milliseconds
        scopeStartTimes.erase(it);
    }
}

void PerformanceProfiler::IncrementDrawCalls(int count) {
    currentFrame.drawCalls += count;
}

void PerformanceProfiler::SetMemoryUsage(size_t bytes) {
    currentFrame.memoryUsage = bytes;
}

void PerformanceProfiler::SetActiveTextures(int count) {
    currentFrame.activeTextures = count;
}

float PerformanceProfiler::GetAverageFrameTime() const {
    return CalculateAverageFromHistory(&FrameMetrics::frameTime);
}

float PerformanceProfiler::GetAverageFPS() const {
    float avgFrameTime = GetAverageFrameTime();
    return avgFrameTime > 0.0f ? 1000.0f / avgFrameTime : 0.0f;
}

int PerformanceProfiler::GetAverageDrawCalls() const {
    return CalculateAverageFromHistory(&FrameMetrics::drawCalls);
}

size_t PerformanceProfiler::GetAverageMemoryUsage() const {
    return CalculateAverageFromHistory(&FrameMetrics::memoryUsage);
}

void PerformanceProfiler::SetTargetFPS(int fps) {
    targetFPS = fps;
}

void PerformanceProfiler::EnableOverlay(bool enable) {
    overlayEnabled = enable;
}

bool PerformanceProfiler::IsOverlayEnabled() const {
    return overlayEnabled;
}

void PerformanceProfiler::DrawPerformanceOverlay(Vector2 position) {
    if (!overlayEnabled) return;
    
    float avgFPS = GetAverageFPS();
    int avgDrawCalls = GetAverageDrawCalls();
    size_t avgMemory = GetAverageMemoryUsage();
    
    // Background
    Rectangle bg = { position.x - 5, position.y - 5, 180, 90 };
    DrawRectangleRec(bg, ColorAlpha(BLACK, 0.7f));
    DrawRectangleLinesEx(bg, 1, WHITE);
    
    // Performance text
    float y = position.y;
    Color fpsColor = avgFPS >= 55 ? GREEN : (avgFPS >= 30 ? YELLOW : RED);
    
    AIGUI_DrawResponsiveText(TextFormat("FPS: %.1f", avgFPS), { position.x, y }, 14, fpsColor);
    y += 16;
    
    AIGUI_DrawResponsiveText(TextFormat("Frame: %.2fms", currentFrame.frameTime), { position.x, y }, 12, WHITE);
    y += 14;
    
    AIGUI_DrawResponsiveText(TextFormat("Draw Calls: %d", avgDrawCalls), { position.x, y }, 12, WHITE);
    y += 14;
    
    AIGUI_DrawResponsiveText(TextFormat("Memory: %.1fMB", avgMemory / (1024.0f * 1024.0f)), { position.x, y }, 12, WHITE);
    y += 14;
    
    // Performance status
    const char* statusText = avgFPS >= 55 ? "GOOD" : (avgFPS >= 30 ? "OK" : "POOR");
    AIGUI_DrawResponsiveText(statusText, { position.x, y }, 12, fpsColor);
}

void PerformanceProfiler::ClearHistory() {
    frameHistory.clear();
    historyIndex = 0;
}

const std::vector<FrameMetrics>& PerformanceProfiler::GetFrameHistory() const {
    return frameHistory;
}

// Helper methods
void PerformanceProfiler::AddFrameToHistory(const FrameMetrics& metrics) {
    if (frameHistory.size() < MAX_FRAME_HISTORY) {
        frameHistory.push_back(metrics);
    } else {
        frameHistory[historyIndex] = metrics;
        historyIndex = (historyIndex + 1) % MAX_FRAME_HISTORY;
    }
}

float PerformanceProfiler::CalculateAverageFromHistory(float FrameMetrics::*member) const {
    if (frameHistory.empty()) return 0.0f;
    
    float sum = 0.0f;
    for (const auto& frame : frameHistory) {
        sum += frame.*member;
    }
    return sum / frameHistory.size();
}

int PerformanceProfiler::CalculateAverageFromHistory(int FrameMetrics::*member) const {
    if (frameHistory.empty()) return 0;
    
    int sum = 0;
    for (const auto& frame : frameHistory) {
        sum += frame.*member;
    }
    return sum / static_cast<int>(frameHistory.size());
}

size_t PerformanceProfiler::CalculateAverageFromHistory(size_t FrameMetrics::*member) const {
    if (frameHistory.empty()) return 0;
    
    size_t sum = 0;
    for (const auto& frame : frameHistory) {
        sum += frame.*member;
    }
    return sum / frameHistory.size();
}

void PerformanceProfiler::UpdateScopeMetrics() {
    if (scopeDurations.count("Update")) {
        currentFrame.updateTime = scopeDurations["Update"];
    }
    if (scopeDurations.count("Draw")) {
        currentFrame.drawTime = scopeDurations["Draw"];
    }
} 