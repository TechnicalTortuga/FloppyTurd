#pragma once
#include "RaylibCompat.h"

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
    
    // Query input states
    bool IsJumpPressed() const { return jumpPressed; }
    bool IsShootPressed() const { return shootPressed; }
    bool IsShootHeld() const { return shootHeld; }
    
    // Gesture recognition
    bool IsGestureDetected(int gesture) const;
    Vector2 GetGestureStartPoint() const { return gestureStartPoint; }
    Vector2 GetGestureEndPoint() const { return gestureEndPoint; }
    float GetGestureDuration() const { return gestureDuration; }
    
    // Enable/disable controls
    void SetEnabled(bool enabled) { isEnabled = enabled; }
    bool IsEnabled() const { return isEnabled; }
    
    // Enable/disable shoot bar (for when shooting is unlocked)
    void SetShootingEnabled(bool enabled) { shootingEnabled = enabled; }

private:
    bool isEnabled = true;
    bool shootingEnabled = false;
    
    // Screen dimensions
    int screenWidth = 0;
    int screenHeight = 0;
    
    // Touch zones
    Rectangle shootZone;    // Bottom bar for shooting
    Rectangle jumpZone;     // Rest of the screen for jumping
    
    // Input states
    bool jumpPressed = false;
    bool shootPressed = false;
    bool shootHeld = false;
    bool previousShootHeld = false;
    
    // Gesture tracking
    int detectedGestures = 0; // Bitfield of detected gestures
    Vector2 gestureStartPoint = {0, 0};
    Vector2 gestureEndPoint = {0, 0};
    float gestureStartTime = 0.0f;
    float gestureDuration = 0.0f;
    int touchCount = 0; // Number of active touches for multi-touch gestures
    
    // Visual settings
    const float SHOOT_BAR_HEIGHT = 30.0f; // Height of the shoot bar at bottom
    const Color SHOOT_BAR_COLOR = { 255, 200, 100, 80 };
    const Color SHOOT_BAR_ACTIVE = { 255, 200, 100, 160 };
    
    // Gesture detection thresholds
    const float SWIPE_MIN_DISTANCE = 50.0f; // Minimum distance for swipe detection
    const float SWIPE_MAX_TIME = 0.5f; // Maximum time for swipe to be considered
    const float TAP_MAX_TIME = 0.2f; // Maximum time for tap detection
    const float TAP_MAX_DISTANCE = 20.0f; // Maximum movement for tap detection
    const float HOLD_MIN_TIME = 0.8f; // Minimum time for hold detection
    const float PINCH_MIN_DISTANCE_CHANGE = 30.0f; // Minimum distance change for pinch detection
};