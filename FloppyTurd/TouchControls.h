#pragma once
#include "RaylibCompat.h"
#include <vector>
#include <string>

// Frame-accurate touch input buffer structure (industry standard)
struct TouchInputBuffer {
    bool isDown;            // Current processed state for this frame
    bool previousDown;      // State from previous frame for edge detection
    bool pressed;           // Transitioned to down this frame
    bool released;          // Transitioned to up this frame
    Vector2 touchPosition;  // Current processed position
    Vector2 previousPosition; // Position from previous frame
    uint64_t frameCount;    // Frame when touch started
    bool rawIsDown;         // Raw state from OS events, not yet processed
    Vector2 rawTouchPosition; // Raw position from OS events, not yet processed

    // Update method with proper state transitions
    void Update(bool newDown, float newX, float newY, uint64_t currentFrame) {
        // Store previous state
        previousDown = isDown;
        previousPosition = touchPosition;

        // Update position
        touchPosition = {newX, newY};
        frameCount = currentFrame;

        // State transition logic (industry standard)
        if (newDown && !previousDown) {
            // Touch just started
            isDown = true;
            pressed = true;
            released = false;
        } else if (!newDown && previousDown) {
            // Touch just ended
            isDown = false;
            pressed = false;
            released = true;
        } else if (newDown && previousDown) {
            // Touch continues
            isDown = true;
            pressed = false;  // Only true for one frame
            released = false;
        } else {
            // No touch
            isDown = false;
            pressed = false;
            released = false;
        }
    }

    // Clear transient states after processing
    void ClearTransientStates() {
        released = false;
        // pressed is managed by state transitions, not forcibly cleared here
    }

    // Reset all states
    void Reset() {
        isDown = false;
        pressed = false;
        released = false;
        touchPosition = {0.0f, 0.0f};
        previousPosition = {0.0f, 0.0f};
        frameCount = 0;
        rawIsDown = false;
        rawTouchPosition = {0.0f, 0.0f};
    }

    // Check if state has changed this frame
    bool HasStateChanged() const {
        return (isDown != previousDown) || pressed || released;
    }

    // Debug method to get state summary
    std::string GetStateString() const {
        return "down=" + std::string(isDown ? "true" : "false") + 
               ", pressed=" + std::string(pressed ? "true" : "false") + 
               ", released=" + std::string(released ? "true" : "false") + 
               ", pos=(" + std::to_string(touchPosition.x) + "," + std::to_string(touchPosition.y) + ")" +
               ", frame=" + std::to_string(frameCount);
    }
};

// Multi-touch gesture buffer for 2-3 touches (jump + shoot scenarios)
struct MultiTouchBuffer {
    static const int MAX_TOUCHES = 3; // Support 2-3 touches max
    
    // Touch data for each finger
    struct TouchData {
        bool active{false};
        bool pressed{false};
        bool released{false};
        float x{0.0f};
        float y{0.0f};
        uint64_t frameId{0};

        // Raw input state from OS events (asynchronous)
        bool rawActive{false};
        Vector2 rawTouchPosition{0.0f, 0.0f};
    };
    
    TouchData touches[MAX_TOUCHES];
    int activeTouchCount{0};
    
    // Pinch gesture data
    float pinchDistance{0.0f};
    float pinchScale{1.0f};
    float initialPinchDistance{0.0f};
    
    // Update multi-touch state
    void UpdateTouch(int touchIndex, bool active, float x, float y, uint64_t frameId) {
        if (touchIndex < 0 || touchIndex >= MAX_TOUCHES) return;
        
        TouchData& touch = touches[touchIndex];
        bool wasActive = touch.active;
        
        // Update touch state
        touch.active = active;
        touch.x = x;
        touch.y = y;
        touch.frameId = frameId;
        
        // Handle state transitions
        if (active && !wasActive) {
            touch.pressed = true;
            touch.released = false;
        } else if (!active && wasActive) {
            touch.pressed = false;
            touch.released = true;
        } else {
            touch.pressed = false;
            touch.released = false;
        }
        
        // Update active touch count
        UpdateActiveTouchCount();
        
        // Update pinch data if we have 2+ touches
        if (activeTouchCount >= 2) {
            UpdatePinchData();
        } else {
            ResetPinchData();
        }
    }
    
    // Clear transient states
    void ClearTransientStates() {
        for (int i = 0; i < MAX_TOUCHES; i++) {
            touches[i].pressed = false;
            touches[i].released = false;
        }
    }
    
    // Reset all states
    void Reset() {
        for (int i = 0; i < MAX_TOUCHES; i++) {
            touches[i] = TouchData{};
        }
        activeTouchCount = 0;
        ResetPinchData();
    }
    
    // Get touch position by index
    Vector2 GetTouchPosition(int index) const {
        if (index >= 0 && index < MAX_TOUCHES && touches[index].active) {
            return {touches[index].x, touches[index].y};
        }
        return {0, 0};
    }
    
    // Check if touch is active by index
    bool IsTouchActive(int index) const {
        return (index >= 0 && index < MAX_TOUCHES && touches[index].active);
    }
    
    // Check if touch was pressed this frame by index
    bool IsTouchPressed(int index) const {
        return (index >= 0 && index < MAX_TOUCHES && touches[index].pressed);
    }
    
    // Check if touch was released this frame by index
    bool IsTouchReleased(int index) const {
        return (index >= 0 && index < MAX_TOUCHES && touches[index].released);
    }
    
private:
    void UpdateActiveTouchCount() {
        activeTouchCount = 0;
        for (int i = 0; i < MAX_TOUCHES; i++) {
            if (touches[i].active) {
                activeTouchCount++;
            }
        }
    }
    
    void UpdatePinchData() {
        if (activeTouchCount < 2) return;
        
        // Find first two active touches
        Vector2 pos1 = {0, 0}, pos2 = {0, 0};
        int found = 0;
        
        for (int i = 0; i < MAX_TOUCHES && found < 2; i++) {
            if (touches[i].active) {
                if (found == 0) {
                    pos1 = {touches[i].x, touches[i].y};
                } else {
                    pos2 = {touches[i].x, touches[i].y};
                }
                found++;
            }
        }
        
        if (found == 2) {
            float dx = pos2.x - pos1.x;
            float dy = pos2.y - pos1.y;
            pinchDistance = sqrtf(dx * dx + dy * dy);
            
            // Set initial distance on first multi-touch
            if (initialPinchDistance == 0.0f) {
                initialPinchDistance = pinchDistance;
            }
            
            // Calculate scale
            if (initialPinchDistance > 0.0f) {
                pinchScale = pinchDistance / initialPinchDistance;
            }
        }
    }
    
    void ResetPinchData() {
        pinchDistance = 0.0f;
        initialPinchDistance = 0.0f;
        pinchScale = 1.0f;
    }
};

// Complete touch controls for mobile gameplay with gesture recognition and shooting
class TouchControls {
public:
    static const int MAX_TOUCHES = 10;
    
    // Static touch state management functions (called by PlatformLayer)
    static void SetTouchState(bool pressed, float x, float y);
    static void UpdateTouchState();
    static void UpdateStatic(); // Static update method to process raw input
    static void ClearTransientStates();
    static void ClearAllTouchStates();
    
    // Static multi-touch methods (for 2-3 touches: jump + shoot scenarios)
    static void SetMultiTouchState(int touchIndex, bool active, float x, float y);
    static void UpdateMultiTouchState();
    static void ClearMultiTouchTransientStates();
    static int GetMultiTouchCount();
    static Vector2 GetMultiTouchPosition(int index);
    static bool IsMultiTouchActive(int index);
    static bool IsMultiTouchPressed(int index);
    static bool IsMultiTouchReleased(int index);
    static float GetPinchDistance();
    static float GetPinchScale();
    
    // Static getters for PlatformLayer
    static bool IsPrimaryInputDown();
    static bool IsPrimaryInputPressed();
    static bool IsPrimaryInputReleased();
    static Vector2 GetPrimaryInputPosition();
    static int GetTouchCount();
    static std::vector<Vector2> GetTouchPoints();
    static uint64_t GetFramesSinceFirstDown(); // Debug: get frames since touch started

    TouchControls();
    ~TouchControls() = default;

    // Initialize with screen dimensions
    void Initialize(float width, float height);
    
    // Process touch events directly from GameView
    void ProcessTouch(float x, float y, bool isDown);
    
    // Update touch states (called from game loop)
    void Update();
    
    // Draw touch UI overlay using MetalRenderer
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
    
    // Multi-touch support
    void SetMultiTouchData(int touchCount, const Vector2* positions, const bool* active);
    Vector2 GetTouchPosition(int index) const;
    bool IsTouchActive(int index) const;
    float GetPinchDistanceInstance() const { return pinchDistance; }
    float GetPinchScaleInstance() const { return pinchScale; }
    
    // Enable/disable controls
    void SetEnabled(bool enabled) { isEnabled = enabled; }
    bool IsEnabled() const { return isEnabled; }
    
    // Enable/disable visual overlay (touch input still works when disabled)
    void SetVisualOverlayEnabled(bool enabled) { visualOverlayEnabled = enabled; }
    bool IsVisualOverlayEnabled() const { return visualOverlayEnabled; }
    
    // Enable/disable shoot bar (for when shooting is unlocked)
    void SetShootingEnabled(bool enabled) { shootingEnabled = enabled; }
    
    // Get touch zones for debugging/visualization
    Rectangle GetJumpZone() const { return jumpZone; }
    Rectangle GetShootZone() const { return shootZone; }

private:
    // Static touch state management (single source of truth)
    static TouchInputBuffer s_TouchBuffer;
    static uint64_t s_CurrentFrameId;
    static bool s_TouchStateChanged;
    
    // Static multi-touch state management (for 2-3 touches)
    static MultiTouchBuffer s_MultiTouchBuffer;
    
    // Instance variables for UI and gameplay logic
    bool isEnabled;
    bool shootingEnabled;
    bool visualOverlayEnabled;
    float screenWidth;
    float screenHeight;
    
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
    
    // Touch tracking for direct input from GameView
    Vector2 touchStartPoint = {0, 0};
    float touchStartTime = 0.0f;
    bool touchActive = false;
    Vector2 currentTouchPoint = {0, 0};
    
    // Multi-touch tracking
    Vector2 touchPositions[MAX_TOUCHES] = {{0, 0}};
    bool touchActiveStates[MAX_TOUCHES] = {false};
    float initialPinchDistance = 0.0f;
    float pinchDistance = 0.0f;
    float pinchScale = 1.0f;
    
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