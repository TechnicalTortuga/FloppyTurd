#pragma once
#include "PlatformAPI.h"
#include "TouchControls.h"
#include "MNKControls.h"

// Floppy Turd specific input handling - contains all gameplay logic
class FloppyTurdInput {
public:
    FloppyTurdInput();
    ~FloppyTurdInput() = default;

    // Initialize with controls and screen dimensions
    void Initialize(TouchControls* touchControls, MNKControls* mnkControls, int screenWidth, int screenHeight);
    
    // Update input states and gameplay logic
    void Update();
    
    // Gameplay-specific input methods
    bool IsJumpPressed() const { return jumpPressed; }
    bool IsShootPressed() const { return shootPressed; }
    bool IsShootHeld() const { return shootHeld; }
    
    // Enable/disable shooting
    void SetShootingEnabled(bool enabled) { shootingEnabled = enabled; }
    bool IsShootingEnabled() const { return shootingEnabled; }
    
    // Get the active controls system
    TouchControls* GetTouchControls() const { return touchControls; }
    MNKControls* GetMNKControls() const { return mnkControls; }
    
    // Generic input passthrough (for UI, etc.)
    bool IsPrimaryInputDown() const;
    bool IsPrimaryInputPressed() const;
    bool IsPrimaryInputReleased() const;
    Vector2 GetPrimaryInputPosition() const;

private:
    TouchControls* touchControls = nullptr;
    MNKControls* mnkControls = nullptr;
    
    // Screen dimensions
    int screenWidth = 0;
    int screenHeight = 0;
    
    // Gameplay state
    bool shootingEnabled = true;
    bool jumpPressed = false;
    bool shootPressed = false;
    bool shootHeld = false;
    
    // Input tracking
    bool previousShootHeld = false;
    
    // Touch zones (for touch gameplay logic)
    Rectangle shootZone;
    Rectangle jumpZone;
    const float SHOOT_BAR_HEIGHT = 30.0f;
    
    // Helper methods
    void UpdateTouchGameplay();
    void UpdateMNKGameplay();
}; 