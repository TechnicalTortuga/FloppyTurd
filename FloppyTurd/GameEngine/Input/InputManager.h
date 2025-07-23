//
//  InputManager.h
//  Professional Game Engine - C++ Input Management
//
//  Created for Swift C++ Interoperability
//  Zero overhead input coordinate transformations and screen management
//

#ifndef InputManager_h
#define InputManager_h

#include "../../PlatformTypes.h"

/// C++ Input Manager for screen coordinate transformations
/// Provides zero overhead screen/world coordinate conversions
class InputManager {
private:
    static InputManager* instance;
    float screenWidth;
    float screenHeight;
    
    InputManager() : screenWidth(800.0f), screenHeight(600.0f) {}
    
public:
    static InputManager* getInstance();
    
    // Screen management
    void setScreenSize(float width, float height);
    Vector2 getScreenSize() const;
    
    // Coordinate transformations
    Vector2 screenToWorld(Vector2 screenPosition) const;
    Vector2 worldToScreen(Vector2 worldPosition) const;
    
    // Utility functions
    bool isPointOnScreen(Vector2 position) const;
    Vector2 normalizeScreenPosition(Vector2 position) const;
};

#endif /* InputManager_h */
