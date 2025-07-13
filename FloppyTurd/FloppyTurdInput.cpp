#include "FloppyTurdInput.h"
#include "GameLog.h"

FloppyTurdInput::FloppyTurdInput() {
    // Constructor - initialization happens in Initialize()
}

void FloppyTurdInput::Initialize(TouchControls* touchControls, MNKControls* mnkControls, int screenWidth, int screenHeight) {
    this->touchControls = touchControls;
    this->mnkControls = mnkControls;
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;
    
    // Set up touch zones for gameplay logic
    shootZone = { 0, (float)(screenHeight - SHOOT_BAR_HEIGHT), (float)screenWidth, SHOOT_BAR_HEIGHT };
    jumpZone = { 0, 0, (float)screenWidth, (float)(screenHeight - SHOOT_BAR_HEIGHT) };
    
    TraceLog(LOG_INFO, "[INPUT] FloppyTurdInput initialized: screen=%dx%d, shootZone=(%.1f,%.1f,%.1f,%.1f)", 
             screenWidth, screenHeight, shootZone.x, shootZone.y, shootZone.width, shootZone.height);
}

void FloppyTurdInput::Update() {
    // Reset per-frame states
    jumpPressed = false;
    shootPressed = false;
    
    // Update controls first
    if (touchControls) {
        touchControls->Update();
        UpdateTouchGameplay();
    }
    
    if (mnkControls) {
        mnkControls->Update();
        UpdateMNKGameplay();
    }
}

void FloppyTurdInput::UpdateTouchGameplay() {
    if (!touchControls || !touchControls->IsEnabled()) return;
    
    // Get raw touch input
    bool touchActive = touchControls->IsPrimaryInputDown();
    bool touchPressed = touchControls->IsPrimaryInputPressed();
    Vector2 touchPos = touchControls->GetPrimaryInputPosition();
    
    if (touchActive) {
        // Determine which zone was touched for gameplay logic
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
}

void FloppyTurdInput::UpdateMNKGameplay() {
    if (!mnkControls || !mnkControls->IsEnabled()) return;
    
    // Check for jump key (space)
    if (mnkControls->IsKeyPressed(KEY_SPACE)) {
        jumpPressed = true;
    }
    
    // Check for shoot key (X) or right mouse button
    if (shootingEnabled) {
        if (mnkControls->IsKeyPressed(KEY_X) || mnkControls->IsRightMousePressed()) {
            shootPressed = true;
        }
        
        // Check for held shoot
        if (mnkControls->IsKeyDown(KEY_X) || mnkControls->IsRightMouseDown()) {
            shootHeld = true;
        } else {
            shootHeld = false;
        }
    }
}

// Generic input passthrough methods
bool FloppyTurdInput::IsPrimaryInputDown() const {
    if (touchControls && touchControls->IsEnabled()) {
        return touchControls->IsPrimaryInputDown();
    }
    if (mnkControls && mnkControls->IsEnabled()) {
        return mnkControls->IsPrimaryInputDown();
    }
    return false;
}

bool FloppyTurdInput::IsPrimaryInputPressed() const {
    if (touchControls && touchControls->IsEnabled()) {
        return touchControls->IsPrimaryInputPressed();
    }
    if (mnkControls && mnkControls->IsEnabled()) {
        return mnkControls->IsPrimaryInputPressed();
    }
    return false;
}

bool FloppyTurdInput::IsPrimaryInputReleased() const {
    if (touchControls && touchControls->IsEnabled()) {
        return touchControls->IsPrimaryInputReleased();
    }
    if (mnkControls && mnkControls->IsEnabled()) {
        return mnkControls->IsPrimaryInputReleased();
    }
    return false;
}

Vector2 FloppyTurdInput::GetPrimaryInputPosition() const {
    if (touchControls && touchControls->IsEnabled()) {
        return touchControls->GetPrimaryInputPosition();
    }
    if (mnkControls && mnkControls->IsEnabled()) {
        return mnkControls->GetPrimaryInputPosition();
    }
    return {0, 0};
} 