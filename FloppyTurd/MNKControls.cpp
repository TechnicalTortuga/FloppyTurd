#include "MNKControls.h"
#include "GlobalStateManager.h"
#include "PlatformAPI.h"

MNKControls::MNKControls() {
    // Constructor - initialization happens in Initialize()
}

void MNKControls::Initialize(int screenWidth, int screenHeight) {
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;
    
    // Initialize state
    ClearAllInputStates();
}

void MNKControls::Update() {
    if (!isEnabled) return;
    
    // Update mouse state from PlatformAPI
    mousePosition = GetMousePosition();
    mouseDelta = GetMouseDelta();
    
    // Update mouse button states
    for (int i = 0; i < 3; i++) {
        bool wasDown = mouseButtons[i];
        bool isDown = IsMouseButtonDown(i);
        
        mouseButtons[i] = isDown;
        mouseButtonsPressed[i] = isDown && !wasDown;
        mouseButtonsReleased[i] = !isDown && wasDown;
    }
    
    // Update keyboard states (this would be populated by the platform layer)
    // For now, we'll rely on the traits system to handle this
    
    // Update shared input state
    UpdatePrimaryInput();
    
    // Update GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    globalState.SetMousePosition(mousePosition);
    globalState.SetMouseDelta(mouseDelta);
    globalState.SetPrimaryInputDown(primaryInputDown);
    globalState.SetPrimaryInputPressed(primaryInputPressed);
    globalState.SetPrimaryInputReleased(primaryInputReleased);
}

// Shared input methods (matches TouchControls interface)
bool MNKControls::IsPrimaryInputDown() const {
    return primaryInputDown;
}

bool MNKControls::IsPrimaryInputPressed() const {
    return primaryInputPressed;
}

bool MNKControls::IsPrimaryInputReleased() const {
    return primaryInputReleased;
}

Vector2 MNKControls::GetPrimaryInputPosition() const {
    return primaryInputPosition;
}

// Mouse-specific methods
bool MNKControls::IsMouseButtonDown(int button) const {
    if (button >= 0 && button < 3) {
        return mouseButtons[button];
    }
    return false;
}

bool MNKControls::IsMouseButtonPressed(int button) const {
    if (button >= 0 && button < 3) {
        return mouseButtonsPressed[button];
    }
    return false;
}

bool MNKControls::IsMouseButtonReleased(int button) const {
    if (button >= 0 && button < 3) {
        return mouseButtonsReleased[button];
    }
    return false;
}

Vector2 MNKControls::GetMousePosition() const {
    return mousePosition;
}

Vector2 MNKControls::GetMouseDelta() const {
    return mouseDelta;
}

float MNKControls::GetScrollWheelDelta() const {
    return scrollWheelDelta;
}

bool MNKControls::IsRightMouseDown() const {
    return IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
}

bool MNKControls::IsRightMousePressed() const {
    return IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
}

bool MNKControls::IsRightMouseReleased() const {
    return IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
}

// Keyboard-specific methods
bool MNKControls::IsKeyDown(int key) const {
    return keysDown.find(key) != keysDown.end();
}

bool MNKControls::IsKeyPressed(int key) const {
    return keysPressed.find(key) != keysPressed.end();
}

bool MNKControls::IsKeyReleased(int key) const {
    return keysReleased.find(key) != keysReleased.end();
}

bool MNKControls::IsKeyHeld(int key) const {
    return keysHeld.find(key) != keysHeld.end();
}

// Input state management
void MNKControls::SetMousePosition(Vector2 position) {
    mousePosition = position;
}

void MNKControls::SetMouseDelta(Vector2 delta) {
    mouseDelta = delta;
}

void MNKControls::SetScrollWheelDelta(float delta) {
    scrollWheelDelta = delta;
}

void MNKControls::SetKeyState(int key, bool down, bool pressed, bool released) {
    if (down) {
        keysDown.insert(key);
    } else {
        keysDown.erase(key);
    }
    
    if (pressed) {
        keysPressed.insert(key);
    } else {
        keysPressed.erase(key);
    }
    
    if (released) {
        keysReleased.insert(key);
    } else {
        keysReleased.erase(key);
    }
    
    // Handle held keys
    if (down && !pressed) {
        keysHeld.insert(key);
    } else {
        keysHeld.erase(key);
    }
}

void MNKControls::SetMouseButtonState(int button, bool down, bool pressed, bool released) {
    if (button >= 0 && button < 3) {
        mouseButtons[button] = down;
        mouseButtonsPressed[button] = pressed;
        mouseButtonsReleased[button] = released;
    }
}

void MNKControls::ClearAllInputStates() {
    // Clear shared input state
    primaryInputDown = false;
    primaryInputPressed = false;
    primaryInputReleased = false;
    primaryInputPosition = {0, 0};
    
    // Clear mouse state
    mousePosition = {0, 0};
    mouseDelta = {0, 0};
    scrollWheelDelta = 0.0f;
    for (int i = 0; i < 3; i++) {
        mouseButtons[i] = false;
        mouseButtonsPressed[i] = false;
        mouseButtonsReleased[i] = false;
    }
    
    // Clear keyboard state
    keysDown.clear();
    keysPressed.clear();
    keysReleased.clear();
    keysHeld.clear();
}

// Helper methods
void MNKControls::UpdatePrimaryInput() {
    // Primary input is left mouse button for MNK
    bool wasDown = primaryInputDown;
    primaryInputDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    primaryInputPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    primaryInputReleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    primaryInputPosition = mousePosition;
} 