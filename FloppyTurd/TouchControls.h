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
    
    // Visual settings
    const float SHOOT_BAR_HEIGHT = 30.0f; // Height of the shoot bar at bottom
    const Color SHOOT_BAR_COLOR = { 255, 200, 100, 80 };
    const Color SHOOT_BAR_ACTIVE = { 255, 200, 100, 160 };
}; 