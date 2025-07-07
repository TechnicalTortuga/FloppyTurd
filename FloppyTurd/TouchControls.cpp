#include "TouchControls.h"
#include "PlatformLayer.h"

TouchControls::TouchControls() {
    // Constructor
}

void TouchControls::Initialize(int width, int height) {
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
}

void TouchControls::Update() {
    if (!isEnabled) return;
    
    // Reset pressed states and gestures for this frame
    jumpPressed = false;
    shootPressed = false;
    detectedGestures = 0;
    
    // Update jump zone height based on shooting availability
    jumpZone.height = screenHeight - (shootingEnabled ? SHOOT_BAR_HEIGHT : 0);
    
    auto& platform = PlatformLayer::GetInstance();
    double currentTime = GetTime();
    
    // Get touch points
    auto touchPoints = platform.GetTouchPoints();
    touchCount = static_cast<int>(touchPoints.size());
    
    // Check for new touches this frame
    if (platform.IsPrimaryInputPressed()) {
        Vector2 touchPos = platform.GetPrimaryInputPosition();
        
        // Check if touch is in shoot zone (if shooting is enabled)
        if (shootingEnabled && CheckCollisionPointRec(touchPos, shootZone)) {
            shootPressed = true;
            shootHeld = true;
        }
        // Otherwise it's a jump
        else if (CheckCollisionPointRec(touchPos, jumpZone)) {
            jumpPressed = true;
        }
        
        // Track gesture start for single touch
        if (touchCount == 1) {
            gestureStartPoint = touchPos;
            gestureStartTime = static_cast<float>(currentTime);
        }
    }
    
    // Update held state for shooting
    if (shootingEnabled && platform.IsPrimaryInputDown()) {
        Vector2 touchPos = platform.GetPrimaryInputPosition();
        previousShootHeld = shootHeld;
        shootHeld = CheckCollisionPointRec(touchPos, shootZone);
        
        // If we just started holding shoot
        if (shootHeld && !previousShootHeld) {
            shootPressed = true;
        }
    }
    else {
        shootHeld = false;
    }
    
    // Handle multi-touch for simultaneous jump and shoot, and gesture detection
    if (touchCount > 0) {
        for (const auto& touch : touchPoints) {
            if (shootingEnabled && CheckCollisionPointRec(touch, shootZone)) {
                if (!shootHeld) {
                    shootPressed = true;
                    shootHeld = true;
                }
            }
            else if (CheckCollisionPointRec(touch, jumpZone)) {
                jumpPressed = true;
            }
        }
        
        // Update gesture end point for single touch
        if (touchCount == 1) {
            gestureEndPoint = touchPoints[0];
            gestureDuration = static_cast<float>(currentTime) - gestureStartTime;
            
            // Check for tap (quick touch and release)
            if (platform.IsPrimaryInputReleased()) {
                float distance = Vector2Distance(gestureStartPoint, gestureEndPoint);
                if (gestureDuration <= TAP_MAX_TIME && distance <= TAP_MAX_DISTANCE) {
                    detectedGestures |= GESTURE_TAP;
                }
            }
            
            // Check for hold (long press without much movement)
            if (gestureDuration >= HOLD_MIN_TIME) {
                float distance = Vector2Distance(gestureStartPoint, gestureEndPoint);
                if (distance <= TAP_MAX_DISTANCE) {
                    detectedGestures |= GESTURE_HOLD;
                }
            }
            
            // Check for swipe (quick movement over a distance)
            if (gestureDuration <= SWIPE_MAX_TIME) {
                float distance = Vector2Distance(gestureStartPoint, gestureEndPoint);
                if (distance >= SWIPE_MIN_DISTANCE) {
                    // Determine swipe direction
                    Vector2 delta = Vector2Subtract(gestureEndPoint, gestureStartPoint);
                    if (fabsf(delta.x) > fabsf(delta.y)) {
                        if (delta.x > 0) {
                            detectedGestures |= GESTURE_SWIPE_RIGHT;
                        } else {
                            detectedGestures |= GESTURE_SWIPE_LEFT;
                        }
                    } else {
                        if (delta.y > 0) {
                            detectedGestures |= GESTURE_SWIPE_DOWN;
                        } else {
                            detectedGestures |= GESTURE_SWIPE_UP;
                        }
                    }
                }
            }
        }
        // Check for pinch gestures with two touches
        else if (touchCount == 2) {
            // Track initial and current distances for pinch detection
            static Vector2 initialTouch1 = {0, 0};
            static Vector2 initialTouch2 = {0, 0};
            static bool pinchInitialized = false;
            
            if (platform.IsSecondaryInputPressed()) {
                initialTouch1 = touchPoints[0];
                initialTouch2 = touchPoints[1];
                pinchInitialized = true;
            }
            
            if (pinchInitialized && platform.IsSecondaryInputDown()) {
                float initialDistance = Vector2Distance(initialTouch1, initialTouch2);
                float currentDistance = Vector2Distance(touchPoints[0], touchPoints[1]);
                float distanceChange = fabsf(currentDistance - initialDistance);
                
                if (distanceChange >= PINCH_MIN_DISTANCE_CHANGE) {
                    if (currentDistance < initialDistance) {
                        detectedGestures |= GESTURE_PINCH_IN;
                    } else {
                        detectedGestures |= GESTURE_PINCH_OUT;
                    }
                    // Reset pinch initialization to avoid repeated triggers
                    pinchInitialized = false;
                }
            }
            
            if (platform.IsSecondaryInputReleased()) {
                pinchInitialized = false;
            }
        }
    } else {
        // Reset gesture tracking when no touches are active
        gestureStartPoint = {0, 0};
        gestureEndPoint = {0, 0};
        gestureStartTime = 0.0f;
        gestureDuration = 0.0f;
    }
}

bool TouchControls::IsGestureDetected(int gesture) const {
    return (detectedGestures & gesture) != 0;
}

void TouchControls::Draw(float alpha) {
    if (!isEnabled) return;
    
    // Draw shoot bar at bottom if shooting is enabled
    if (shootingEnabled) {
        Color barColor = shootHeld ? SHOOT_BAR_ACTIVE : SHOOT_BAR_COLOR;
        DrawRectangleRec(shootZone, barColor);
        
        // Draw shoot text
        const char* shootText = "SHOOT";
        int fontSize = 16;
        int textWidth = MeasureText(shootText, fontSize);
        int textX = screenWidth / 2 - textWidth / 2;
        int textY = screenHeight - SHOOT_BAR_HEIGHT / 2 - fontSize / 2;
        
        DrawText(shootText, textX, textY, fontSize, WHITE);
        
        // Draw a separator line with proper thickness
        DrawLineEx({0, (float)(screenHeight - SHOOT_BAR_HEIGHT)}, {(float)screenWidth, (float)(screenHeight - SHOOT_BAR_HEIGHT)}, 2.0f, WHITE);
    }
    
    // Optionally draw a subtle indicator for the jump area when touched
    if (jumpPressed) {
        DrawRectangleRec(jumpZone, Fade(WHITE, 0.1f));
    }
} 