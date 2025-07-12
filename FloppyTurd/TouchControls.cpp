#include "TouchControls.h"
#include "PlatformAPI.h"
#include "GameLog.h"

TouchControls::TouchControls() {
    // Constructor - all initialization done in Initialize()
}

void TouchControls::Initialize(int screenWidth, int screenHeight) {
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;
    
    // Set up touch zones
    shootZone = { 0, (float)(screenHeight - SHOOT_BAR_HEIGHT), (float)screenWidth, SHOOT_BAR_HEIGHT };
    jumpZone = { 0, 0, (float)screenWidth, (float)(screenHeight - SHOOT_BAR_HEIGHT) };
    
    TraceLog(LOG_INFO, "[TOUCH] TouchControls initialized: screen=%dx%d, shootZone=(%.1f,%.1f,%.1f,%.1f)", 
             screenWidth, screenHeight, shootZone.x, shootZone.y, shootZone.width, shootZone.height);
}

void TouchControls::Update() {
    // Reset transient states
    jumpPressed = false;
    shootPressed = false;
    
    // Get touch input from PlatformLayer
    bool touchActive = IsPrimaryInputDown();
    bool touchPressed = IsPrimaryInputPressed();
    Vector2 touchPos = GetPrimaryInputPosition();
    
    if (touchActive && isEnabled) {
        // Determine which zone was touched
        if (CheckCollisionPointRec(touchPos, shootZone) && shootingEnabled) {
            // Touch in shoot zone
            if (touchPressed) {
                shootPressed = true;
            }
            shootHeld = true;
        } else if (CheckCollisionPointRec(touchPos, jumpZone)) {
            // Touch in jump zone
            if (touchPressed) {
                jumpPressed = true;
            }
        }
    } else {
        shootHeld = false;
    }
    
    // Update gesture detection
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
    if (!isEnabled) return;
    
    // Draw shoot bar if shooting is enabled
    if (shootingEnabled) {
        Color barColor = shootHeld ? SHOOT_BAR_ACTIVE : SHOOT_BAR_COLOR;
        barColor.a = (unsigned char)(barColor.a * alpha);
        DrawRectangle(
            (int)shootZone.x, (int)shootZone.y, 
            (int)shootZone.width, (int)shootZone.height, 
            barColor
        );
    }
    
    // Debug: Draw touch zones (optional)
    #ifdef DEBUG_TOUCH_ZONES
    Color zoneColor = { 255, 255, 255, 50 };
    zoneColor.a = (unsigned char)(zoneColor.a * alpha);
    DrawRectangle(
        (int)jumpZone.x, (int)jumpZone.y, 
        (int)jumpZone.width, (int)jumpZone.height, 
        zoneColor
    );
    #endif
} 

// Touch state management methods
void TouchControls::UpdateGestureDetection() {
    // Clear per-frame states
    primaryInputPressed = false;
    primaryInputReleased = false;
    
    // Update gameplay-specific logic based on raw input
    if (primaryInputDown) {
        // Touch is active - determine if it's jump or shoot based on zones
        if (shootingEnabled && primaryInputPosition.y > screenHeight - SHOOT_BAR_HEIGHT) {
            // Touch in shoot zone
            shootHeld = true;
            if (!previousShootHeld) {
                shootPressed = true;
            }
        } else {
            // Touch in jump zone
            jumpPressed = true;
        }
    } else {
        // No touch
        jumpPressed = false;
        shootPressed = false;
        shootHeld = false;
    }
    
    previousShootHeld = shootHeld;
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
    
    // Clear gameplay states
    jumpPressed = false;
    shootPressed = false;
    shootHeld = false;
    previousShootHeld = false;
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