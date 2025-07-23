//
//  TouchController.cpp
//  Professional Game Engine - C++ Touch Management Implementation
//
//  Created for Swift C++ Interoperability
//  Zero overhead touch event handling and gesture recognition
//

#include "TouchController.h"
#include <algorithm>
#include <cmath>

// Static instance
TouchController* TouchController::instance = nullptr;

TouchController* TouchController::getInstance() {
    if (!instance) {
        instance = new TouchController();
    }
    return instance;
}

void TouchController::onTouchBegan(int id, float x, float y, float normalizedX, float normalizedY, float pressure, double timestamp) {
    TouchData touch;
    touch.id = id;
    touch.position = {x, y};
    touch.normalizedPosition = {normalizedX, normalizedY};
    touch.pressure = pressure;
    touch.timestamp = timestamp;
    touch.phase = 0; // began
    
    activeTouches.push_back(touch);
    touchStartPositions[id] = {x, y};
    touchStartTimes[id] = timestamp;
}

void TouchController::onTouchMoved(int id, float x, float y, float normalizedX, float normalizedY, float pressure, double timestamp) {
    auto it = std::find_if(activeTouches.begin(), activeTouches.end(),
                          [id](const TouchData& touch) { return touch.id == id; });
    
    if (it != activeTouches.end()) {
        it->position = {x, y};
        it->normalizedPosition = {normalizedX, normalizedY};
        it->pressure = pressure;
        it->timestamp = timestamp;
        it->phase = 1; // moved
    }
}

void TouchController::onTouchEnded(int id, float x, float y, float normalizedX, float normalizedY, float pressure, double timestamp) {
    auto it = std::find_if(activeTouches.begin(), activeTouches.end(),
                          [id](const TouchData& touch) { return touch.id == id; });
    
    if (it != activeTouches.end()) {
        it->position = {x, y};
        it->normalizedPosition = {normalizedX, normalizedY};
        it->pressure = pressure;
        it->timestamp = timestamp;
        it->phase = 2; // ended
        
        // Remove from active touches
        activeTouches.erase(it);
        touchStartPositions.erase(id);
        touchStartTimes.erase(id);
    }
}

void TouchController::onTouchCancelled(int id, float x, float y, float normalizedX, float normalizedY, float pressure, double timestamp) {
    auto it = std::find_if(activeTouches.begin(), activeTouches.end(),
                          [id](const TouchData& touch) { return touch.id == id; });
    
    if (it != activeTouches.end()) {
        it->phase = 3; // cancelled
        activeTouches.erase(it);
        touchStartPositions.erase(id);
        touchStartTimes.erase(id);
    }
}

bool TouchController::isTouchActive(float x, float y) const {
    const float tolerance = 20.0f;
    for (const auto& touch : activeTouches) {
        float dx = touch.position.x - x;
        float dy = touch.position.y - y;
        if (std::sqrt(dx * dx + dy * dy) <= tolerance) {
            return true;
        }
    }
    return false;
}

int TouchController::getActiveTouchCount() const {
    return static_cast<int>(activeTouches.size());
}

Vector2 TouchController::getTouchPosition(int index) const {
    if (index >= 0 && index < static_cast<int>(activeTouches.size())) {
        return activeTouches[index].position;
    }
    return {0.0f, 0.0f};
}

bool TouchController::isTapGesture(float x, float y, float tolerance) const {
    // Simple tap detection - could be enhanced
    return isTouchActive(x, y);
}

Vector2 TouchController::getSwipeDirection() const {
    if (activeTouches.empty()) {
        return {0.0f, 0.0f};
    }
    
    // Get the most recent touch
    const TouchData& touch = activeTouches.back();
    auto startPosIt = touchStartPositions.find(touch.id);
    
    if (startPosIt != touchStartPositions.end()) {
        Vector2 startPos = startPosIt->second;
        return {
            touch.position.x - startPos.x,
            touch.position.y - startPos.y
        };
    }
    
    return {0.0f, 0.0f};
}

void TouchController::setSafeArea(float top, float left, float bottom, float right) {
    safeAreaInsets = {left + right, top + bottom};
}
