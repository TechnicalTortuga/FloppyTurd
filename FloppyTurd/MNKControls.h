#pragma once
#include "PlatformAPI.h"
#include <vector>
#include <unordered_set>

// Mouse and Keyboard controls for desktop gameplay
class MNKControls {
public:
    MNKControls();
    ~MNKControls() = default;

    // Initialize with screen dimensions
    void Initialize(int screenWidth, int screenHeight);
    
    // Update input states
    void Update();
    
    // Generic input methods (shared with TouchControls)
    bool IsPrimaryInputDown() const;
    bool IsPrimaryInputPressed() const;
    bool IsPrimaryInputReleased() const;
    Vector2 GetPrimaryInputPosition() const;
    
    // Mouse-specific methods
    bool IsMouseButtonDown(int button) const;
    bool IsMouseButtonPressed(int button) const;
    bool IsMouseButtonReleased(int button) const;
    Vector2 GetMousePosition() const;
    Vector2 GetMouseDelta() const;
    float GetScrollWheelDelta() const;
    bool IsRightMouseDown() const;
    bool IsRightMousePressed() const;
    bool IsRightMouseReleased() const;
    
    // Keyboard-specific methods
    bool IsKeyDown(int key) const;
    bool IsKeyPressed(int key) const;
    bool IsKeyReleased(int key) const;
    bool IsKeyHeld(int key) const;
    
    // General input methods (no gameplay logic)
    
    // Input state management
    void SetMousePosition(Vector2 position);
    void SetMouseDelta(Vector2 delta);
    void SetScrollWheelDelta(float delta);
    void SetKeyState(int key, bool down, bool pressed, bool released);
    void SetMouseButtonState(int button, bool down, bool pressed, bool released);
    void ClearAllInputStates();
    
    // Enable/disable controls
    void SetEnabled(bool enabled) { isEnabled = enabled; }
    bool IsEnabled() const { return isEnabled; }
    
private:
    bool isEnabled = true;
    
    // Screen dimensions
    int screenWidth = 0;
    int screenHeight = 0;
    
    // Shared input state (matches TouchControls interface)
    bool primaryInputDown = false;
    bool primaryInputPressed = false;
    bool primaryInputReleased = false;
    Vector2 primaryInputPosition = {0, 0};
    
    // Mouse state
    Vector2 mousePosition = {0, 0};
    Vector2 mouseDelta = {0, 0};
    float scrollWheelDelta = 0.0f;
    bool mouseButtons[3] = {false, false, false}; // Left, Right, Middle
    bool mouseButtonsPressed[3] = {false, false, false};
    bool mouseButtonsReleased[3] = {false, false, false};
    
    // Keyboard state
    std::unordered_set<int> keysDown;
    std::unordered_set<int> keysPressed;
    std::unordered_set<int> keysReleased;
    std::unordered_set<int> keysHeld;
    
    // Helper methods
    void UpdatePrimaryInput();
}; 