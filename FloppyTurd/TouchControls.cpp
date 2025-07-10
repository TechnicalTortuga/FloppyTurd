#include "TouchControls.h"
#include "PlatformLayer.h"
#include "RaylibCompat.h"
#include <cmath>

// Define the static constant
const int TouchControls::MAX_TOUCHES;

// Static touch state management (single source of truth)
TouchInputBuffer TouchControls::s_TouchBuffer;
uint64_t TouchControls::s_CurrentFrameId = 0;
bool TouchControls::s_TouchStateChanged = false;

// Static multi-touch state management (for 2-3 touches)
MultiTouchBuffer TouchControls::s_MultiTouchBuffer;

// Static touch state management functions (called by PlatformLayer)
void TouchControls::SetTouchState(bool pressed, float x, float y) {
    TraceLog(LOG_INFO, "[TOUCH_DEBUG] SetTouchState: Setting rawIsDown=%d, rawTouchPosition=(%.1f, %.1f)", pressed, x, y);
    s_TouchBuffer.rawIsDown = pressed;
    s_TouchBuffer.rawTouchPosition = {x, y};
}

void TouchControls::UpdateTouchState() {
    TraceLog(LOG_INFO, "[TOUCH_DEBUG] UpdateTouchState ENTRY - Frame %llu", s_CurrentFrameId);
    TraceLog(LOG_INFO, "[TOUCH_DEBUG] Raw input before update: rawIsDown=%d, rawTouchPosition=(%.1f, %.1f)", s_TouchBuffer.rawIsDown, s_TouchBuffer.rawTouchPosition.x, s_TouchBuffer.rawTouchPosition.y);
    // 1. Latch the state from the previous game frame
    s_TouchBuffer.previousDown = s_TouchBuffer.isDown;
    s_TouchBuffer.previousPosition = s_TouchBuffer.touchPosition;
    // 2. Consume the latest raw input from OS events
    s_TouchBuffer.isDown = s_TouchBuffer.rawIsDown;
    s_TouchBuffer.touchPosition = s_TouchBuffer.rawTouchPosition;
    // 3. Calculate the transient states for this frame
    s_TouchBuffer.pressed = s_TouchBuffer.isDown && !s_TouchBuffer.previousDown;
    s_TouchBuffer.released = !s_TouchBuffer.isDown && s_TouchBuffer.previousDown;
    // 4. Update frame count for debugging
    if (s_TouchBuffer.pressed) {
        s_TouchBuffer.frameCount = s_CurrentFrameId;
    }
    TraceLog(LOG_INFO, "[TOUCH_DEBUG] Processed state after update: isDown=%d, previousDown=%d, pressed=%d, released=%d, pos=(%.1f, %.1f)", s_TouchBuffer.isDown, s_TouchBuffer.previousDown, s_TouchBuffer.pressed, s_TouchBuffer.released, s_TouchBuffer.touchPosition.x, s_TouchBuffer.touchPosition.y);
}

// Static update method to process raw input into state variables
void TouchControls::UpdateStatic() {
    TraceLog(LOG_INFO, "[TOUCH_DEBUG] TouchControls::UpdateStatic() called - Frame %llu", s_CurrentFrameId);
    
    // Increment frame counter
    s_CurrentFrameId++;
    
    // Process raw input into state variables
    UpdateTouchState();
    UpdateMultiTouchState();
    
    TraceLog(LOG_INFO, "[TOUCH_DEBUG] TouchControls::UpdateStatic() completed - Frame %llu", s_CurrentFrameId);
}

// New method to clear transient states at the end of the frame
void TouchControls::ClearTransientStates() {
    TraceLog(LOG_INFO, "[TOUCH] TouchControls::ClearTransientStates ENTRY - Frame %llu", s_CurrentFrameId);
    
    // Clear transient states (pressed/released) at the end of the frame
    // This ensures all systems have had a chance to read them
    s_TouchBuffer.ClearTransientStates();
    
    TraceLog(LOG_INFO, "[TOUCH] TouchControls::ClearTransientStates EXIT - Frame %llu", s_CurrentFrameId);
}


void TouchControls::ClearAllTouchStates() {
    TraceLog(LOG_INFO, "[TOUCH] TouchControls::ClearAllTouchStates ENTRY");
    
    s_TouchBuffer.Reset();
    
    TraceLog(LOG_INFO, "[TOUCH] TouchControls::ClearAllTouchStates EXIT: all states reset");
}

// Static getters for PlatformLayer
bool TouchControls::IsPrimaryInputDown() {
    return s_TouchBuffer.isDown;
}

bool TouchControls::IsPrimaryInputPressed() {
    return s_TouchBuffer.pressed;
}

bool TouchControls::IsPrimaryInputReleased() {
    return s_TouchBuffer.released;
}

Vector2 TouchControls::GetPrimaryInputPosition() {
    return s_TouchBuffer.touchPosition;
}

int TouchControls::GetTouchCount() {
    return s_TouchBuffer.isDown ? 1 : 0;
}

std::vector<Vector2> TouchControls::GetTouchPoints() {
    std::vector<Vector2> points;
    if (s_TouchBuffer.isDown || s_TouchBuffer.pressed) {
        points.push_back(s_TouchBuffer.touchPosition);
    }
    return points;
}

uint64_t TouchControls::GetFramesSinceFirstDown() {
    return s_TouchBuffer.frameCount;
}

// Static multi-touch methods (for 2-3 touches: jump + shoot scenarios)
void TouchControls::SetMultiTouchState(int touchIndex, bool active, float x, float y) {
    if (touchIndex < 0 || touchIndex >= MultiTouchBuffer::MAX_TOUCHES) {
        TraceLog(LOG_WARNING, "[TOUCH] SetMultiTouchState: Invalid touch index %d", touchIndex);
        return;
    }
    TraceLog(LOG_INFO, "[TOUCH] SetMultiTouchState: index=%d, active=%d, x=%.1f, y=%.1f", touchIndex, active, x, y);
    s_MultiTouchBuffer.touches[touchIndex].rawActive = active;
    s_MultiTouchBuffer.touches[touchIndex].rawTouchPosition = {x, y};
}

void TouchControls::UpdateMultiTouchState() {
    s_MultiTouchBuffer.activeTouchCount = 0;
    for (int i = 0; i < MultiTouchBuffer::MAX_TOUCHES; i++) {
        auto& touch = s_MultiTouchBuffer.touches[i];
        // 1. Latch the state from the previous game frame
        bool wasActive = touch.active;

        // 2. Consume the latest raw input from OS events
        touch.active = touch.rawActive;
        touch.x = touch.rawTouchPosition.x;
        touch.y = touch.rawTouchPosition.y;

        // 3. Calculate the transient states for this frame
        touch.pressed = touch.active && !wasActive;
        touch.released = !touch.active && wasActive;

        if (touch.active) {
            s_MultiTouchBuffer.activeTouchCount++;
        }
    }

    // Calculate pinch distance and scale if there are at least 2 active touches
    if (s_MultiTouchBuffer.activeTouchCount >= 2) {
        float previousPinchDistance = s_MultiTouchBuffer.pinchDistance;
        Vector2 pos1 = s_MultiTouchBuffer.GetTouchPosition(0);
        Vector2 pos2 = s_MultiTouchBuffer.GetTouchPosition(1);
        s_MultiTouchBuffer.pinchDistance = sqrtf(powf(pos2.x - pos1.x, 2) + powf(pos2.y - pos1.y, 2));
        if (previousPinchDistance > 0.0f) {
            s_MultiTouchBuffer.pinchScale = s_MultiTouchBuffer.pinchDistance / previousPinchDistance;
        } else {
            s_MultiTouchBuffer.pinchScale = 1.0f;
        }
    } else {
        s_MultiTouchBuffer.pinchDistance = 0.0f;
        s_MultiTouchBuffer.pinchScale = 1.0f;
    }

    TraceLog(LOG_INFO, "[TOUCH] UpdateMultiTouchState: activeCount=%d, pinchDistance=%.1f, pinchScale=%.2f", 
             s_MultiTouchBuffer.activeTouchCount, s_MultiTouchBuffer.pinchDistance, s_MultiTouchBuffer.pinchScale);
}

void TouchControls::ClearMultiTouchTransientStates() {
    TraceLog(LOG_INFO, "[TOUCH] TouchControls::ClearMultiTouchTransientStates ENTRY - Frame %llu", s_CurrentFrameId);
    
    s_MultiTouchBuffer.ClearTransientStates();
}

int TouchControls::GetMultiTouchCount() {
    return s_MultiTouchBuffer.activeTouchCount;
}

Vector2 TouchControls::GetMultiTouchPosition(int index) {
    return s_MultiTouchBuffer.GetTouchPosition(index);
}

bool TouchControls::IsMultiTouchActive(int index) {
    return s_MultiTouchBuffer.IsTouchActive(index);
}

bool TouchControls::IsMultiTouchPressed(int index) {
    return s_MultiTouchBuffer.IsTouchPressed(index);
}

bool TouchControls::IsMultiTouchReleased(int index) {
    return s_MultiTouchBuffer.IsTouchReleased(index);
}

float TouchControls::GetPinchDistance() {
    return s_MultiTouchBuffer.pinchDistance;
}

float TouchControls::GetPinchScale() {
    return s_MultiTouchBuffer.pinchScale;
}

TouchControls::TouchControls() {
    // Constructor - all initialization done in Initialize()
}

void TouchControls::Initialize(float width, float height) {
    screenWidth = width;
    screenHeight = height;
    
    // Define touch zones
    // Shoot zone is at the bottom when shooting is enabled
    shootZone = {
        0, 
        (float)(screenHeight - SHOOT_BAR_HEIGHT),
        (float)screenWidth,
        SHOOT_BAR_HEIGHT
    };
    
    // Jump zone is the rest of the screen
    jumpZone = {
        0,
        0,
        (float)screenWidth,
        (float)(screenHeight - (shootingEnabled ? SHOOT_BAR_HEIGHT : 0))
    };
    
    // Reset all states
    jumpPressed = false;
    shootPressed = false;
    shootHeld = false;
    previousShootHeld = false;
    detectedGestures = 0;
    gestureStartPoint = {0, 0};
    gestureEndPoint = {0, 0};
    gestureStartTime = 0.0f;
    gestureDuration = 0.0f;
    touchCount = 0;
    touchActive = false;
    touchStartPoint = {0, 0};
    touchStartTime = 0.0f;
    currentTouchPoint = {0, 0};
    
    // Reset multi-touch data
    for (int i = 0; i < MAX_TOUCHES; i++) {
        touchPositions[i] = {0, 0};
        touchActiveStates[i] = false;
    }
    initialPinchDistance = 0.0f;
    pinchDistance = 0.0f;
    pinchScale = 1.0f;
    
    TraceLog(LOG_INFO, "[TOUCH] TouchControls initialized: %.0fx%.0f, shootingEnabled: %s", 
             width, height, shootingEnabled ? "true" : "false");
}

void TouchControls::ProcessTouch(float x, float y, bool isDown) {
    if (!isEnabled) return;
    
    Vector2 touchPoint = {x, y};
    currentTouchPoint = touchPoint;
    
    // NOTE: PlatformLayer::SetTouchState is now called directly from GameView.mm
    // to avoid double updates and ensure single source of truth for touch state
    
    if (isDown) {
        // Touch started
        if (!touchActive) {
            touchActive = true;
            touchStartPoint = touchPoint;
            touchStartTime = GetTime();
            touchCount = 1;
            
            // Check if touch is in shoot zone (if shooting is enabled)
            if (shootingEnabled && CheckCollisionPointRec(touchPoint, shootZone)) {
                shootPressed = true;
                shootHeld = true;
                TraceLog(LOG_INFO, "[TOUCH] Shoot touch started at (%.1f, %.1f)", x, y);
            }
            // Otherwise it's a jump
            else if (CheckCollisionPointRec(touchPoint, jumpZone)) {
                jumpPressed = true;
                TraceLog(LOG_INFO, "[TOUCH] Jump touch started at (%.1f, %.1f)", x, y);
            }
            
            // Track gesture start for single touch
            gestureStartPoint = touchPoint;
            gestureStartTime = static_cast<float>(GetTime());
        } else {
            // Additional touch (multi-touch)
            touchCount++;
        }
    } else {
        // Touch ended
        if (touchActive) {
            touchActive = false;
            touchCount = 0;
            
            // Update gesture end point
            gestureEndPoint = touchPoint;
            gestureDuration = static_cast<float>(GetTime()) - gestureStartTime;
            
            // Check for tap (quick touch and release)
            float touchDistance = Vector2Distance(touchStartPoint, touchPoint);
            if (gestureDuration <= TAP_MAX_TIME && touchDistance <= TAP_MAX_DISTANCE) {
                detectedGestures |= GESTURE_TAP;
                TraceLog(LOG_INFO, "[TOUCH] Tap detected at (%.1f, %.1f) - duration: %.3f, distance: %.1f", 
                         x, y, gestureDuration, touchDistance);
            }
            
            // Check for hold (long press without much movement)
            if (gestureDuration >= HOLD_MIN_TIME) {
                if (touchDistance <= TAP_MAX_DISTANCE) {
                    detectedGestures |= GESTURE_HOLD;
                    TraceLog(LOG_INFO, "[TOUCH] Hold detected at (%.1f, %.1f) - duration: %.3f", 
                             x, y, gestureDuration);
                }
            }
            
            // Check for swipe (quick movement over a distance)
            if (gestureDuration <= SWIPE_MAX_TIME) {
                if (touchDistance >= SWIPE_MIN_DISTANCE) {
                    // Determine swipe direction
                    Vector2 delta = Vector2Subtract(gestureEndPoint, gestureStartPoint);
                    if (fabsf(delta.x) > fabsf(delta.y)) {
                        if (delta.x > 0) {
                            detectedGestures |= GESTURE_SWIPE_RIGHT;
                            TraceLog(LOG_INFO, "[TOUCH] Swipe right detected");
                        } else {
                            detectedGestures |= GESTURE_SWIPE_LEFT;
                            TraceLog(LOG_INFO, "[TOUCH] Swipe left detected");
                        }
                    } else {
                        if (delta.y > 0) {
                            detectedGestures |= GESTURE_SWIPE_DOWN;
                            TraceLog(LOG_INFO, "[TOUCH] Swipe down detected");
                        } else {
                            detectedGestures |= GESTURE_SWIPE_UP;
                            TraceLog(LOG_INFO, "[TOUCH] Swipe up detected");
                        }
                    }
                }
            }
            
            // Reset press states
            jumpPressed = false;
            shootPressed = false;
            shootHeld = false;
            
            TraceLog(LOG_INFO, "[TOUCH] Touch ended at (%.1f, %.1f)", x, y);
        }
    }
}

void TouchControls::Update() {
    // This is the single, authoritative update function called once per frame.
    if (!isEnabled) return;

    // 1. Process the raw input from the OS into our static buffers.
    // This is the crucial step that was missing.
    UpdateTouchState();
    UpdateMultiTouchState();

    // 2. Get the now-correct state from the static buffer.
    bool touchActive = s_TouchBuffer.isDown;
    Vector2 touchPos = Vector2{s_TouchBuffer.touchPosition.x, s_TouchBuffer.touchPosition.y};
    bool touchPressed = s_TouchBuffer.pressed;
    bool touchReleased = s_TouchBuffer.released;

    // 3. Sync instance state with the static buffer.
    this->touchActive = touchActive;
    this->currentTouchPoint = touchPos;
    
    // Reset pressed states and gestures for this frame
    jumpPressed = false;
    shootPressed = false;
    detectedGestures = 0;
    
    // Update jump zone height based on shooting availability
    jumpZone.height = screenHeight - (shootingEnabled ? SHOOT_BAR_HEIGHT : 0);
    
    // Process current touch state if active
    if (touchActive) {
        // Check if current touch is in shoot zone (if shooting is enabled)
        if (shootingEnabled && CheckCollisionPointRec(currentTouchPoint, shootZone)) {
            shootPressed = true;
            shootHeld = true;
            TraceLog(LOG_INFO, "[TOUCH] TouchControls: shoot zone detected at (%.1f, %.1f)", currentTouchPoint.x, currentTouchPoint.y);
        }
        // Otherwise it's a jump
        else if (CheckCollisionPointRec(currentTouchPoint, jumpZone)) {
            jumpPressed = true;
            TraceLog(LOG_INFO, "[TOUCH] TouchControls: jump zone detected at (%.1f, %.1f)", currentTouchPoint.x, currentTouchPoint.y);
        }
        
        // Update held state for shooting
        if (shootingEnabled && touchActive) {
            previousShootHeld = shootHeld;
            shootHeld = CheckCollisionPointRec(currentTouchPoint, shootZone);
            
            // If we just started holding shoot
            if (shootHeld && !previousShootHeld) {
                shootPressed = true;
            }
        }
    } else {
        shootHeld = false;
    }
    
    // Update gesture tracking when no touches are active
    if (!touchActive) {
        gestureStartPoint = {0, 0};
        gestureEndPoint = {0, 0};
        gestureStartTime = 0.0f;
        gestureDuration = 0.0f;
    }
    
    // Debug logging
    if (touchActive || touchPressed || touchReleased) {
        TraceLog(LOG_INFO, "[TOUCH] TouchControls Update: active=%d, pressed=%d, released=%d, pos=(%.1f,%.1f), jump=%d, shoot=%d", 
                 touchActive, touchPressed, touchReleased, currentTouchPoint.x, currentTouchPoint.y, jumpPressed, shootPressed);
    }
}

bool TouchControls::IsGestureDetected(int gesture) const {
    return (detectedGestures & gesture) != 0;
}

void TouchControls::Draw(float alpha) {
    if (!isEnabled || !visualOverlayEnabled) return;
    
    // Draw shoot bar at bottom if shooting is enabled
    if (shootingEnabled) {
        Color barColor = shootHeld ? SHOOT_BAR_ACTIVE : SHOOT_BAR_COLOR;
        barColor.a = (unsigned char)(barColor.a * alpha);
        DrawRectangleRec(shootZone, barColor);
        
        // Draw shoot text
        const char* shootText = "SHOOT";
        int fontSize = 16;
        int textWidth = MeasureText(shootText, fontSize);
        int textX = screenWidth / 2 - textWidth / 2;
        int textY = screenHeight - SHOOT_BAR_HEIGHT / 2 - fontSize / 2;
        
        Color textColor = WHITE;
        textColor.a = (unsigned char)(textColor.a * alpha);
        DrawText(shootText, textX, textY, fontSize, textColor);
        
        // Draw a separator line with proper thickness
        Color lineColor = WHITE;
        lineColor.a = (unsigned char)(lineColor.a * alpha);
        DrawLineEx({0, (float)(screenHeight - SHOOT_BAR_HEIGHT)}, 
                   {(float)screenWidth, (float)(screenHeight - SHOOT_BAR_HEIGHT)}, 
                   2.0f, lineColor);
    }
    
    // Draw visual feedback for ANY touch (not just zone-specific touches)
    if (touchActive) {
        // Draw a subtle indicator at the touch position
        Color touchColor = {255, 255, 255, (unsigned char)(80 * alpha)};
        float touchSize = 40.0f;
        DrawCircleV(currentTouchPoint, touchSize, touchColor);
        
        // Draw a smaller, brighter center
        Color centerColor = {255, 255, 255, (unsigned char)(150 * alpha)};
        DrawCircleV(currentTouchPoint, touchSize * 0.3f, centerColor);
    }
    
    // Optionally draw a subtle indicator for the jump area when touched
    if (jumpPressed) {
        Color zoneColor = {255, 255, 255, (unsigned char)(50 * alpha)};
        DrawRectangleRec(jumpZone, zoneColor);
    }
    
    // Optionally draw jump zone outline for debugging (only in debug builds)
    #ifdef _DEBUG
    if (jumpPressed || shootHeld) {
        Color zoneColor = {255, 255, 255, (unsigned char)(100 * alpha)};
        DrawRectangleLinesEx(jumpZone, 2.0f, zoneColor);
        if (shootingEnabled) {
            DrawRectangleLinesEx(shootZone, 2.0f, zoneColor);
        }
    }
    #endif
}

void TouchControls::SetMultiTouchData(int newTouchCount, const Vector2* positions, const bool* active) {
    touchCount = std::min(newTouchCount, MAX_TOUCHES);
    
    // Copy touch data
    for (int i = 0; i < touchCount; i++) {
        if (positions) touchPositions[i] = positions[i];
        if (active) touchActiveStates[i] = active[i];
    }
    
    // Calculate pinch distance and scale for multi-touch
    if (touchCount >= 2) {
        Vector2 pos1 = touchPositions[0];
        Vector2 pos2 = touchPositions[1];
        
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
        
        TraceLog(LOG_INFO, "[TOUCH] Multi-touch: %d touches, pinch distance: %.1f, scale: %.2f", 
                 touchCount, pinchDistance, pinchScale);
    } else {
        pinchDistance = 0.0f;
        initialPinchDistance = 0.0f;
        pinchScale = 1.0f;
    }
}

Vector2 TouchControls::GetTouchPosition(int index) const {
    if (index >= 0 && index < touchCount && index < MAX_TOUCHES) {
        return touchPositions[index];
    }
    return {0, 0};
}

bool TouchControls::IsTouchActive(int index) const {
    if (index >= 0 && index < touchCount && index < MAX_TOUCHES) {
        return touchActiveStates[index];
    }
    return false;
} 