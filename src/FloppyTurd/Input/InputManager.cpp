#include "InputManager.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Utility/Utils.h"
#include <algorithm>
#include <chrono>

namespace GameCore {

// Initialize static instance
InputManager* InputManager::s_instance = nullptr;

InputManager::InputManager(Gnosis::ECS* ecs, PlatformDelegates* platformDelegates)
    : m_ecs(ecs)
    , m_platformDelegates(platformDelegates)
    , m_initialized(false)
    , m_inputEnabled(true)
    , m_currentFrameNumber(0)
    , m_lastUpdateTime(0)
    , m_screenWidth(1179.0f)
    , m_screenHeight(2556.0f)
    , m_isLandscape(false)
    , m_maxTouchId(0)
{
    GN_LOG_INFO("InputManager created");
}

InputManager::~InputManager() {
    Shutdown();
    GN_LOG_INFO("InputManager destroyed");
}

// Singleton implementation
InputManager* InputManager::GetInstance() {
    if (!s_instance) {
        GN_LOG_ERROR("InputManager::GetInstance() called before InitializeInstance()!");
        return nullptr;
    }
    return s_instance;
}

void InputManager::InitializeInstance(Gnosis::ECS* ecs, PlatformDelegates* platformDelegates) {
    if (s_instance) {
        GN_LOG_WARN("InputManager instance already exists, destroying old instance");
        delete s_instance;
    }

    GN_LOG_INFO("Creating InputManager singleton instance");
    s_instance = new InputManager(ecs, platformDelegates);

    if (!s_instance->Initialize()) {
        GN_LOG_ERROR("Failed to initialize InputManager singleton instance");
        delete s_instance;
        s_instance = nullptr;
    }
}

void InputManager::DestroyInstance() {
    if (s_instance) {
        GN_LOG_INFO("Destroying InputManager singleton instance");
        delete s_instance;
        s_instance = nullptr;
    }
}

bool InputManager::Initialize() {
    if (m_initialized) {
        GN_LOG_WARN("InputManager already initialized");
        return true;
    }

    GN_LOG_INFO("Initializing InputManager...");

    // Update screen information
    UpdateScreenInfo();

    // Setup default action zones
    SetupDefaultActionZones();

    // Clear any lingering input
    ClearInputBuffers();

    m_initialized = true;
    GN_LOG_INFO("InputManager initialized successfully");
    return true;
}

void InputManager::Update(float deltaTime) {
    if (!m_initialized || !m_inputEnabled) {
        return;
    }

    // Update frame counter
    m_currentFrameNumber++;
    m_lastUpdateTime = GetCurrentTimestamp();

    // Update screen information (handles orientation changes)
    UpdateScreenInfo();

    // Store previous touch state for state change detection
    m_previousTouches = m_currentTouches;

    // Poll platform for new input
    PollPlatformInput();

    // Update touch states (pressed, held, released)
    UpdateTouchStates();

    // Process action mappings
    ProcessActionMappings();

    // Cleanup frame history
    CleanupFrameHistory();
}

void InputManager::Shutdown() {
    if (!m_initialized) {
        return;
    }

    GN_LOG_INFO("Shutting down InputManager...");

    ClearInputBuffers();
    m_frameHistory.clear();
    m_actionZones.clear();

    m_initialized = false;
    GN_LOG_INFO("InputManager shutdown complete");
}

void InputManager::SetECSSystem(Gnosis::ECS* ecs) {
    m_ecs = ecs;
    GN_LOG_INFO("InputManager ECS system updated");
}

// Touch Input Queries

int InputManager::GetTouchCount() const {
    return static_cast<int>(m_currentTouches.size());
}

TouchData InputManager::GetTouchData(int touchId) const {
    for (const auto& touch : m_currentTouches) {
        if (touch.touchId == touchId) {
            return touch;
        }
    }
    return TouchData(); // Return empty TouchData
}

std::vector<TouchData> InputManager::GetActiveTouches() const {
    return m_currentTouches;
}

bool InputManager::IsTouchPressed(int touchId) const {
    auto touch = GetTouchData(touchId);
    return touch.state == TouchState::PRESSED;
}

bool InputManager::IsTouchHeld(int touchId) const {
    auto touch = GetTouchData(touchId);
    return touch.state == TouchState::HELD;
}

bool InputManager::IsTouchReleased(int touchId) const {
    auto touch = GetTouchData(touchId);
    return touch.state == TouchState::RELEASED;
}

bool InputManager::IsAnyTouchActive() const {
    return !m_currentTouches.empty();
}

// Action-Based Input

bool InputManager::IsActionPressed(InputAction action) const {
    // For now, map actions to touch zones
    // This can be extended to support keyboard, gamepad, etc.
    for (const auto& touch : m_currentTouches) {
        if (touch.state == TouchState::PRESSED) {
            for (const auto& zone : m_actionZones) {
                if (zone.action == action && IsPointInZone(touch.x, touch.y, zone)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool InputManager::IsActionHeld(InputAction action) const {
    for (const auto& touch : m_currentTouches) {
        if (touch.state == TouchState::HELD) {
            for (const auto& zone : m_actionZones) {
                if (zone.action == action && IsPointInZone(touch.x, touch.y, zone)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool InputManager::IsActionReleased(InputAction action) const {
    for (const auto& touch : m_currentTouches) {
        if (touch.state == TouchState::RELEASED) {
            for (const auto& zone : m_actionZones) {
                if (zone.action == action && IsPointInZone(touch.x, touch.y, zone)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Coordinate Conversion

void InputManager::ScreenToNormalized(float screenX, float screenY, float& normalizedX, float& normalizedY) const {
    if (m_screenWidth > 0 && m_screenHeight > 0) {
        normalizedX = screenX / m_screenWidth;
        normalizedY = screenY / m_screenHeight;
    } else {
        normalizedX = 0.0f;
        normalizedY = 0.0f;
    }
}

void InputManager::NormalizedToScreen(float normalizedX, float normalizedY, float& screenX, float& screenY) const {
    screenX = normalizedX * m_screenWidth;
    screenY = normalizedY * m_screenHeight;
}

// Screen and Orientation

void InputManager::GetScreenDimensions(float& width, float& height) const {
    width = m_screenWidth;
    height = m_screenHeight;
}

bool InputManager::IsLandscapeOrientation() const {
    return m_isLandscape;
}

uint64_t InputManager::GetCurrentFrameNumber() const {
    return m_currentFrameNumber;
}

// Input State Management

void InputManager::ClearInputBuffers() {
    m_currentTouches.clear();
    m_previousTouches.clear();
    m_maxTouchId = 0;
    m_frameHistory.clear();

    // Clear platform input buffer if available
    if (m_platformDelegates && m_platformDelegates->input.clearInputBuffer) {
        m_platformDelegates->input.clearInputBuffer();
        GN_LOG_INFO("InputManager: Cleared platform input buffer");
    }
}

void InputManager::SetInputEnabled(bool enabled) {
    if (m_inputEnabled != enabled) {
        m_inputEnabled = enabled;
        GN_LOG_INFO("InputManager: Input " + std::string(enabled ? "enabled" : "disabled"));
    }
}

bool InputManager::IsInputEnabled() const {
    return m_inputEnabled;
}

// Debug and Analytics

std::string InputManager::GetDebugInfo() const {
    std::string info = "InputManager Debug Info:\n";
    info += "  Frame: " + std::to_string(m_currentFrameNumber) + "\n";
    info += "  Screen: " + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight) + "\n";
    info += "  Orientation: " + std::string(m_isLandscape ? "Landscape" : "Portrait") + "\n";
    info += "  Touch Count: " + std::to_string(GetTouchCount()) + "\n";
    info += "  Input Enabled: " + std::string(m_inputEnabled ? "Yes" : "No") + "\n";

    for (const auto& touch : m_currentTouches) {
        info += "  Touch " + std::to_string(touch.touchId) + ": ";
        info += "(" + std::to_string(touch.x) + ", " + std::to_string(touch.y) + ") ";
        info += "Raw(" + std::to_string(touch.rawX) + ", " + std::to_string(touch.rawY) + ") ";
        info += "State: ";
        switch (touch.state) {
            case TouchState::PRESSED: info += "PRESSED"; break;
            case TouchState::HELD: info += "HELD"; break;
            case TouchState::RELEASED: info += "RELEASED"; break;
            default: info += "NONE"; break;
        }
        info += "\n";
    }

    return info;
}

// Private Methods

void InputManager::UpdateScreenInfo() {
    if (!m_ecs) {
        return;
    }

    // Get screen dimensions from ECS
    float newWidth = m_ecs->GetScreenWidth();
    float newHeight = m_ecs->GetScreenHeight();

    if (newWidth > 0 && newHeight > 0) {
        bool wasLandscape = m_isLandscape;
        m_screenWidth = newWidth;
        m_screenHeight = newHeight;
        m_isLandscape = (newWidth > newHeight);

        // Log orientation changes
        if (wasLandscape != m_isLandscape) {
            GN_LOG_INFO("InputManager: Orientation changed to " +
                       std::string(m_isLandscape ? "landscape" : "portrait") +
                       " (" + std::to_string((int)newWidth) + "x" + std::to_string((int)newHeight) + ")");
        }
    }
}

void InputManager::PollPlatformInput() {
    if (!m_platformDelegates) {
        GN_LOG_ERROR("🎮 InputManager: Platform delegates NULL!");
        return;
    }

    // Clear current touches and repopulate from platform delegates
    m_currentTouches.clear();

    // Poll for touch input through platform delegates
    if (m_platformDelegates->input.getTouchCount &&
        m_platformDelegates->input.getTouchPosition &&
        m_platformDelegates->input.isTouchJustPressed &&
        m_platformDelegates->input.isTouchJustReleased) {

        int touchCount = m_platformDelegates->input.getTouchCount();
        bool isJustPressed = m_platformDelegates->input.isTouchJustPressed();
        bool isJustReleased = m_platformDelegates->input.isTouchJustReleased();

        GN_LOG_INFO("🎮 InputManager: Polling - touchCount=" + std::to_string(touchCount) + 
                    " pressed=" + std::to_string(isJustPressed) + 
                    " released=" + std::to_string(isJustReleased));

        for (int i = 0; i < touchCount; ++i) {
            float pixelX = 0.0f, pixelY = 0.0f;
            m_platformDelegates->input.getTouchPosition(i, &pixelX, &pixelY);

            // Determine touch state - these are global flags, not per-touch
            // For simplicity, assume all touches have the same state
            TouchState state = TouchState::HELD; // Default to held
            if (isJustPressed && i == 0) {  // Only first touch gets pressed state
                state = TouchState::PRESSED;
            } else if (isJustReleased && i == 0) {  // Only first touch gets released state
                state = TouchState::RELEASED;
            }

            // Create touch data - delegates give pixel coords directly
            // Normalize for TouchData storage, but keep pixel coords for rawX/rawY
            float normalizedX = pixelX / m_screenWidth;
            float normalizedY = pixelY / m_screenHeight;

            TouchData touchData(i, normalizedX, normalizedY, pixelX, pixelY, state, GetCurrentTimestamp());
            m_currentTouches.push_back(touchData);

            // Update max touch ID for future assignments
            m_maxTouchId = std::max(m_maxTouchId, i);

            // Enhanced logging for coordinate handling
            GN_LOG_INFO("🎯 InputManager: Touch " + std::to_string(i) +
                        " - pixel(" + std::to_string(pixelX) + ", " + std::to_string(pixelY) +
                        ") -> norm(" + std::to_string(normalizedX) + ", " + std::to_string(normalizedY) +
                        ") screenDims=" + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
        }
        
        GN_LOG_INFO("🎮 InputManager: Polling complete - " + std::to_string(m_currentTouches.size()) + " touches stored");
    }
}

void InputManager::UpdateTouchStates() {
    // Touch states are already set during polling
    // This method can be extended for more complex state management
}

void InputManager::ProcessActionMappings() {
    // Action processing is handled in the IsAction* methods
    // This can be extended for more complex action processing
}

void InputManager::AddTouchToHistory(const TouchData& touch) {
    // Add current frame to history
    InputFrame frame(m_currentFrameNumber, m_lastUpdateTime, m_isLandscape);
    frame.touches = m_currentTouches;
    m_frameHistory.push_back(frame);
}

void InputManager::CleanupFrameHistory() {
    if (m_frameHistory.size() > MAX_FRAME_HISTORY) {
        m_frameHistory.erase(m_frameHistory.begin());
    }
}

TouchData InputManager::CreateTouchData(int touchId, float rawX, float rawY, TouchState state) const {
    // Convert raw coordinates to normalized
    float normalizedX, normalizedY;
    ScreenToNormalized(rawX, rawY, normalizedX, normalizedY);

    return TouchData(touchId, normalizedX, normalizedY, rawX, rawY, state, m_lastUpdateTime);
}

bool InputManager::IsPointInZone(float x, float y, const ActionZone& zone) const {
    return (x >= zone.x1 && x <= zone.x2 && y >= zone.y1 && y <= zone.y2);
}

void InputManager::SetupDefaultActionZones() {
    // Clear existing zones
    m_actionZones.clear();

    // Setup default action zones based on screen layout
    // These can be configured based on level, UI layout, etc.

    // Jump zone (most of the screen)
    ActionZone jumpZone = {0.0f, 0.0f, 1.0f, 0.8f, InputAction::JUMP};
    m_actionZones.push_back(jumpZone);

    // Shoot zone (bottom right corner)
    ActionZone shootZone = {0.7f, 0.8f, 1.0f, 1.0f, InputAction::SHOOT};
    m_actionZones.push_back(shootZone);

    // Pause zone (top left area for settings button)
    ActionZone pauseZone = {0.0f, 0.0f, 0.2f, 0.1f, InputAction::PAUSE};
    m_actionZones.push_back(pauseZone);

    GN_LOG_INFO("InputManager: Default action zones configured");
}

uint64_t InputManager::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace GameCore
