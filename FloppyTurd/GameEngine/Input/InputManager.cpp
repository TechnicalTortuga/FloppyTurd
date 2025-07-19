//
//  InputManager.cpp
//  Professional Game Engine - C++ Input Management Implementation
//
//  Created for Swift C++ Interoperability
//  Zero overhead input coordinate transformations and screen management
//

#include "InputManager.h"
#include <algorithm>

// Static instance
InputManager* InputManager::instance = nullptr;

InputManager* InputManager::getInstance() {
    if (!instance) {
        instance = new InputManager();
    }
    return instance;
}

void InputManager::setScreenSize(float width, float height) {
    screenWidth = width;
    screenHeight = height;
}

Vector2 InputManager::getScreenSize() const {
    return {screenWidth, screenHeight};
}

Vector2 InputManager::screenToWorld(Vector2 screenPosition) const {
    // Simple 1:1 mapping for now - can be enhanced for camera transformations
    return screenPosition;
}

Vector2 InputManager::worldToScreen(Vector2 worldPosition) const {
    // Simple 1:1 mapping for now - can be enhanced for camera transformations
    return worldPosition;
}

bool InputManager::isPointOnScreen(Vector2 position) const {
    return position.x >= 0 && position.x <= screenWidth &&
           position.y >= 0 && position.y <= screenHeight;
}

Vector2 InputManager::normalizeScreenPosition(Vector2 position) const {
    return {
        screenWidth > 0 ? position.x / screenWidth : 0.0f,
        screenHeight > 0 ? position.y / screenHeight : 0.0f
    };
}
