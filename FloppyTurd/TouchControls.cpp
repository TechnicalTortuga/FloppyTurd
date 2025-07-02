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
    
    // Reset pressed states
    jumpPressed = false;
    shootPressed = false;
    
    // Update jump zone height based on shooting availability
    jumpZone.height = screenHeight - (shootingEnabled ? SHOOT_BAR_HEIGHT : 0);
    
    auto& platform = PlatformLayer::GetInstance();
    
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
    
    // Handle multi-touch for simultaneous jump and shoot
    auto touchPoints = platform.GetTouchPoints();
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
        
        // Draw a separator line
        DrawLine(0, screenHeight - SHOOT_BAR_HEIGHT, screenWidth, screenHeight - SHOOT_BAR_HEIGHT, WHITE);
    }
    
    // Optionally draw a subtle indicator for the jump area when touched
    if (jumpPressed) {
        DrawRectangleRec(jumpZone, Fade(WHITE, 0.1f));
    }
} 