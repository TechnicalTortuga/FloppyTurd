#include "GlobalStateManager.h"
#include "PlatformAPI.h"

// ============================================================================
// RENDERING STATE QUERIES IMPLEMENTATION
// ============================================================================

Vector2 GlobalStateManager::GetScreenCenter() const {
    return Vector2{
        static_cast<float>(m_screenWidth) * 0.5f,
        static_cast<float>(m_screenHeight) * 0.5f
    };
}

Vector2 GlobalStateManager::GetRenderScale() const {
    return Vector2{m_screenScale, m_screenScale};
}

Rectangle GlobalStateManager::GetSafeArea() const {
    // For now, return the full screen area
    // This should be updated by platform-specific code when safe area changes
    return Rectangle{0, 0, static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight)};
}

float GlobalStateManager::GetScreenDensity() const {
    return m_screenScale;
}

bool GlobalStateManager::IsLandscape() const {
    return m_screenWidth > m_screenHeight;
}

bool GlobalStateManager::IsPortrait() const {
    return m_screenHeight > m_screenWidth;
}

// ============================================================================
// C-STYLE WRAPPER FUNCTIONS FOR iOS INTEGRATION
// ============================================================================
// These allow GameView.mm to update GlobalStateManager without C++ linkage issues

extern "C" {
    void GlobalStateManager_SetPrimaryInputDown(bool down) {
        GlobalStateManager::GetInstance().SetPrimaryInputDown(down);
    }
    
    void GlobalStateManager_SetPrimaryInputPressed(bool pressed) {
        GlobalStateManager::GetInstance().SetPrimaryInputPressed(pressed);
    }
    
    void GlobalStateManager_SetPrimaryInputReleased(bool released) {
        GlobalStateManager::GetInstance().SetPrimaryInputReleased(released);
    }
    
    void GlobalStateManager_SetTouchPoints(const Vector2* points, int count) {
        std::vector<Vector2> touchPoints;
        for (int i = 0; i < count; ++i) {
            touchPoints.push_back(points[i]);
        }
        GlobalStateManager::GetInstance().SetTouchPoints(touchPoints);
    }
    
    void GlobalStateManager_SetMousePosition(float x, float y) {
        Vector2 pos = {x, y};
        GlobalStateManager::GetInstance().SetMousePosition(pos);
    }
    
    void GlobalStateManager_SetMouseDelta(float x, float y) {
        Vector2 delta = {x, y};
        GlobalStateManager::GetInstance().SetMouseDelta(delta);
    }
} 