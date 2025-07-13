#include "TouchControls.h"
#include "PlatformAPI.h"
#include "GameLog.h"

TouchControls::TouchControls() {
    // Constructor - all initialization done in Initialize()
}

void TouchControls::Initialize(int screenWidth, int screenHeight) {
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;
    
    TraceLog(LOG_INFO, "[TOUCH] TouchControls initialized: screen=%dx%d", screenWidth, screenHeight);
}

void TouchControls::Update() {
    // Update gesture detection with current touch state
    bool touchActive = IsPrimaryInputDown();
    bool touchPressed = IsPrimaryInputPressed();
    Vector2 touchPos = GetPrimaryInputPosition();
    
    UpdateGestureDetection(touchActive, touchPressed, touchPos);
}

// Remove static variables - using instance variables instead

// Generic input methods (decoupled from gameplay)
bool TouchControls::IsPrimaryInputDown() const {
    return primaryInputDown;
}

bool TouchControls::IsPrimaryInputPressed() const {
    return primaryInputPressed;
}

bool TouchControls::IsPrimaryInputReleased() const {
    return primaryInputReleased;
}

Vector2 TouchControls::GetPrimaryInputPosition() const {
    return primaryInputPosition;
}

void TouchControls::UpdateGestureDetection(bool touchActive, bool touchPressed, Vector2 touchPos) {
    static float touchStartTime = 0.0f;
    static Vector2 touchStartPos = {0, 0};
    static bool gestureInProgress = false;
    
    if (touchPressed) {
        // Start new gesture
        touchStartTime = GetTime();
        touchStartPos = touchPos;
        gestureStartPoint = touchPos;
        gestureInProgress = true;
        detectedGestures = 0;
    }
    
    if (gestureInProgress) {
        float currentTime = GetTime();
        float gestureTime = currentTime - touchStartTime;
        float distance = Vector2Distance(touchStartPos, touchPos);
        
        // Update gesture end point
        gestureEndPoint = touchPos;
        gestureDuration = gestureTime;
        
        // Detect different gesture types
        if (gestureTime <= TAP_MAX_TIME && distance <= TAP_MAX_DISTANCE) {
            detectedGestures |= (1 << 0); // TAP
        }
        
        if (distance >= SWIPE_MIN_DISTANCE && gestureTime <= SWIPE_MAX_TIME) {
            detectedGestures |= (1 << 1); // SWIPE
        }
        
        if (gestureTime >= HOLD_MIN_TIME) {
            detectedGestures |= (1 << 2); // HOLD
        }
        
        if (!touchActive) {
            // Gesture ended
            gestureInProgress = false;
        }
    }
}

bool TouchControls::IsGestureDetected(int gesture) const {
    return (detectedGestures & (1 << gesture)) != 0;
}

void TouchControls::Draw(float alpha) {
    // Touch controls are now purely input - no visual elements
    // Visual elements should be handled by the game-specific input handler
} 

// Touch state management methods
void TouchControls::UpdateGestureDetection() {
    // This method is now handled by the game-specific input handler
    // Touch controls only provide raw input state
}

void TouchControls::SetTouchState(bool pressed, float x, float y) {
    bool wasDown = primaryInputDown;
    
    primaryInputDown = pressed;
    primaryInputPosition = {x, y};
    
    // Set per-frame states
    if (pressed && !wasDown) {
        primaryInputPressed = true;
        primaryInputReleased = false;
    } else if (!pressed && wasDown) {
        primaryInputPressed = false;
        primaryInputReleased = true;
    }
}

void TouchControls::ClearAllTouchStates() {
    primaryInputDown = false;
    primaryInputPressed = false;
    primaryInputReleased = false;
    primaryInputPosition = {0, 0};
    
    // Clear gesture states
    detectedGestures = 0;
    touchCount = 0;
}

std::vector<Vector2> TouchControls::GetTouchPoints() const {
    std::vector<Vector2> points;
    if (primaryInputDown) {
        points.push_back(primaryInputPosition);
    }
    return points;
} 