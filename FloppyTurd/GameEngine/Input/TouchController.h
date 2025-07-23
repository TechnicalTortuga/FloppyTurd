//
//  TouchController.h
//  Professional Game Engine - C++ Touch Management
//
//  Created for Swift C++ Interoperability
//  Zero overhead touch event handling and gesture recognition
//

#ifndef TouchController_h
#define TouchController_h

#include "../../PlatformTypes.h"
#include <vector>
#include <map>

struct TouchData {
    int id;
    Vector2 position;
    Vector2 normalizedPosition;
    float pressure;
    double timestamp;
    int phase; // 0=began, 1=moved, 2=ended, 3=cancelled
};

/// C++ Touch Controller for touch event management
/// Provides zero overhead touch tracking and gesture recognition
class TouchController {
private:
    static TouchController* instance;
    std::vector<TouchData> activeTouches;
    Vector2 safeAreaInsets;
    
    // Gesture detection
    std::map<int, Vector2> touchStartPositions;
    std::map<int, double> touchStartTimes;
    
    TouchController() : safeAreaInsets({0.0f, 0.0f}) {}
    
public:
    static TouchController* getInstance();
    
    // Touch management
    void onTouchBegan(int id, float x, float y, float normalizedX, float normalizedY, float pressure, double timestamp);
    void onTouchMoved(int id, float x, float y, float normalizedX, float normalizedY, float pressure, double timestamp);
    void onTouchEnded(int id, float x, float y, float normalizedX, float normalizedY, float pressure, double timestamp);
    void onTouchCancelled(int id, float x, float y, float normalizedX, float normalizedY, float pressure, double timestamp);
    
    // Touch queries
    bool isTouchActive(float x, float y) const;
    int getActiveTouchCount() const;
    Vector2 getTouchPosition(int index) const;
    
    // Gesture detection
    bool isTapGesture(float x, float y, float tolerance) const;
    Vector2 getSwipeDirection() const;
    
    // Configuration
    void setSafeArea(float top, float left, float bottom, float right);
};

#endif /* TouchController_h */
