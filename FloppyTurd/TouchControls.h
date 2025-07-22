#pragma once
#include "PlatformAPI.h"
#include <vector>

#ifdef PLATFORM_MOBILE
// Forward declaration for Swift interop
extern "C" {
    // getCppInteropBridge is available from GameEngine-Swift.h when built
}
#endif

// Simplified touch controls for mobile gameplay
class TouchControls {
public:
    TouchControls();
    ~TouchControls() = default;

    // Initialize with screen dimensions
    void Initialize(int screenWidth, int screenHeight);
    
    // Update touch states
    void Update();
    
    // Draw touch UI overlay
    void Draw(float alpha = 0.5f);
    
    // Generic input methods (decoupled from gameplay)
    bool IsPrimaryInputDown() const;
    bool IsPrimaryInputPressed() const;
    bool IsPrimaryInputReleased() const;
    Vector2 GetPrimaryInputPosition() const;
    
    // Touch state management methods
    void UpdateGestureDetection();
    void SetTouchState(bool pressed, float x, float y);
    void ClearAllTouchStates();
    std::vector<Vector2> GetTouchPoints() const;
    
    // General input methods (no gameplay logic)
    bool IsTouchInZone(const Rectangle& zone) const;
    bool IsMultiTouch() const { return touchCount > 1; }
    
    // Gesture recognition
    bool IsGestureDetected(int gesture) const;
    Vector2 GetGestureStartPoint() const { return gestureStartPoint; }
    Vector2 GetGestureEndPoint() const { return gestureEndPoint; }
    float GetGestureDuration() const { return gestureDuration; }
    
    // Enable/disable controls
    void SetEnabled(bool enabled) { isEnabled = enabled; }
    bool IsEnabled() const { return isEnabled; }
    
private:
    bool isEnabled = true;
    
    // Screen dimensions
    int screenWidth = 0;
    int screenHeight = 0;
    
    // Raw touch state (instance-based, not static)
    bool primaryInputDown = false;
    bool primaryInputPressed = false;
    bool primaryInputReleased = false;
    Vector2 primaryInputPosition = {0, 0};
    
    // Gesture tracking
    int detectedGestures = 0; // Bitfield of detected gestures
    Vector2 gestureStartPoint = {0, 0};
    Vector2 gestureEndPoint = {0, 0};
    float gestureStartTime = 0.0f;
    float gestureDuration = 0.0f;
    int touchCount = 0; // Number of active touches for multi-touch gestures
    
    // Gesture detection thresholds
    const float SWIPE_MIN_DISTANCE = 50.0f; // Minimum distance for swipe detection
    const float SWIPE_MAX_TIME = 0.5f; // Maximum time for swipe to be considered
    const float TAP_MAX_TIME = 0.2f; // Maximum time for tap detection
    const float TAP_MAX_DISTANCE = 20.0f; // Maximum movement for tap detection
    const float HOLD_MIN_TIME = 0.8f; // Minimum time for hold detection
    const float PINCH_MIN_DISTANCE_CHANGE = 30.0f; // Minimum distance change for pinch detection
    
    // Helper methods
    void UpdateGestureDetection(bool touchActive, bool touchPressed, Vector2 touchPos);
};