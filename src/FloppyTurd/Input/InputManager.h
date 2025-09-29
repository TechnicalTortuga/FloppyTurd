#pragma once

#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <vector>
#include <memory>
#include <functional>

namespace GameCore {

/**
 * @enum TouchState
 * @brief Current state of a touch input
 */
enum class TouchState {
    NONE,           // No touch
    PRESSED,        // Just pressed this frame
    HELD,           // Currently being held down
    RELEASED        // Just released this frame
};

/**
 * @enum InputAction
 * @brief Game-specific input actions
 */
enum class InputAction {
    JUMP,
    SHOOT,
    PAUSE,
    MENU,
    CONFIRM,
    CANCEL
};

/**
 * @struct TouchData
 * @brief Data for a single touch point
 */
struct TouchData {
    int touchId;
    float x, y;                    // Normalized coordinates (0.0-1.0)
    float rawX, rawY;             // Raw pixel coordinates
    TouchState state;
    uint64_t timestamp;

    TouchData() : touchId(-1), x(0), y(0), rawX(0), rawY(0), state(TouchState::NONE), timestamp(0) {}
    TouchData(int id, float nx, float ny, float rx, float ry, TouchState s, uint64_t time)
        : touchId(id), x(nx), y(ny), rawX(rx), rawY(ry), state(s), timestamp(time) {}
};

/**
 * @struct InputFrame
 * @brief Snapshot of all input state for a single frame
 */
struct InputFrame {
    uint64_t frameNumber;
    uint64_t timestamp;
    std::vector<TouchData> touches;
    bool isOrientationLandscape;

    InputFrame() : frameNumber(0), timestamp(0), isOrientationLandscape(false) {}
    InputFrame(uint64_t frame, uint64_t time, bool landscape)
        : frameNumber(frame), timestamp(time), isOrientationLandscape(landscape) {}
};

/**
 * @class InputManager
 * @brief Centralized input management system for the entire game
 *
 * Features:
 * - Unified input handling across all game systems
 * - Orientation-aware coordinate transformation
 * - Touch state management (pressed, held, released)
 * - Action-based input abstraction
 * - Frame-based input buffering
 * - Clean separation of concerns
 */
class InputManager {
public:
    /**
     * @brief Get singleton instance
     * @return Pointer to the InputManager singleton
     */
    static InputManager* GetInstance();

    /**
     * @brief Initialize the singleton instance
     * @param ecs Pointer to ECS system for screen dimension queries
     * @param platformDelegates Platform-specific delegates for input polling
     */
    static void InitializeInstance(Gnosis::ECS* ecs, PlatformDelegates* platformDelegates);

    /**
     * @brief Destroy the singleton instance
     */
    static void DestroyInstance();

private:
    /**
     * @brief Private constructor for singleton
     * @param ecs Pointer to ECS system for screen dimension queries
     * @param platformDelegates Platform-specific delegates for input polling
     */
    InputManager(Gnosis::ECS* ecs, PlatformDelegates* platformDelegates);

    /**
     * @brief Private destructor for singleton
     */
    ~InputManager();

    // Prevent copying
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

public:

    /**
     * @brief Initialize the input manager
     * @return true if initialization successful
     */
    bool Initialize();

    /**
     * @brief Update input state - call once per frame
     * @param deltaTime Time since last frame
     */
    void Update(float deltaTime);

    /**
     * @brief Update the ECS system reference (for late initialization)
     * @param ecs Pointer to ECS system
     */
    void SetECSSystem(Gnosis::ECS* ecs);


    /**
     * @brief Shutdown the input manager and clean up resources
     */
    void Shutdown();

    // Touch Input Queries

    /**
     * @brief Get the number of active touches
     * @return Number of touches currently active
     */
    int GetTouchCount() const;

    /**
     * @brief Get touch data for a specific touch ID
     * @param touchId Touch identifier (0-based)
     * @return TouchData for the specified touch, or empty TouchData if not found
     */
    TouchData GetTouchData(int touchId) const;

    /**
     * @brief Get all active touches
     * @return Vector of all active touch data
     */
    std::vector<TouchData> GetActiveTouches() const;

    /**
     * @brief Check if a touch is currently pressed (just pressed this frame)
     * @param touchId Touch identifier (0-based)
     * @return true if touch was just pressed
     */
    bool IsTouchPressed(int touchId) const;

    /**
     * @brief Check if a touch is currently held down
     * @param touchId Touch identifier (0-based)
     * @return true if touch is being held
     */
    bool IsTouchHeld(int touchId) const;

    /**
     * @brief Check if a touch was just released
     * @param touchId Touch identifier (0-based)
     * @return true if touch was just released
     */
    bool IsTouchReleased(int touchId) const;

    /**
     * @brief Check if any touch is active
     * @return true if at least one touch is active
     */
    bool IsAnyTouchActive() const;

    // Action-Based Input

    /**
     * @brief Check if an action was just pressed
     * @param action The input action to check
     * @return true if action was just pressed
     */
    bool IsActionPressed(InputAction action) const;

    /**
     * @brief Check if an action is currently held
     * @param action The input action to check
     * @return true if action is being held
     */
    bool IsActionHeld(InputAction action) const;

    /**
     * @brief Check if an action was just released
     * @param action The input action to check
     * @return true if action was just released
     */
    bool IsActionReleased(InputAction action) const;

    // Coordinate Conversion

    /**
     * @brief Convert screen coordinates to normalized coordinates (0.0-1.0)
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @param[out] normalizedX Normalized X coordinate
     * @param[out] normalizedY Normalized Y coordinate
     */
    void ScreenToNormalized(float screenX, float screenY, float& normalizedX, float& normalizedY) const;

    /**
     * @brief Convert normalized coordinates to screen coordinates
     * @param normalizedX Normalized X coordinate
     * @param normalizedY Normalized Y coordinate
     * @param[out] screenX Screen X coordinate
     * @param[out] screenY Screen Y coordinate
     */
    void NormalizedToScreen(float normalizedX, float normalizedY, float& screenX, float& screenY) const;

    // Screen and Orientation

    /**
     * @brief Get current screen dimensions
     * @param[out] width Screen width in pixels
     * @param[out] height Screen height in pixels
     */
    void GetScreenDimensions(float& width, float& height) const;

    /**
     * @brief Check if device is in landscape orientation
     * @return true if landscape, false if portrait
     */
    bool IsLandscapeOrientation() const;

    /**
     * @brief Get current frame number
     * @return Current frame number
     */
    uint64_t GetCurrentFrameNumber() const;

    // Input State Management

    /**
     * @brief Clear all input buffers and reset state
     */
    void ClearInputBuffers();

    /**
     * @brief Set input enabled/disabled state
     * @param enabled Whether input should be processed
     */
    void SetInputEnabled(bool enabled);

    /**
     * @brief Check if input is currently enabled
     * @return true if input is enabled
     */
    bool IsInputEnabled() const;

    // Debug and Analytics

    /**
     * @brief Get debug information about current input state
     * @return String containing debug information
     */
    std::string GetDebugInfo() const;

private:
    // Core systems
    Gnosis::ECS* m_ecs;
    PlatformDelegates* m_platformDelegates;

    // Input state
    bool m_initialized;
    bool m_inputEnabled;
    uint64_t m_currentFrameNumber;
    uint64_t m_lastUpdateTime;

    // Screen state
    float m_screenWidth;
    float m_screenHeight;
    bool m_isLandscape;

    // Touch state management
    std::vector<TouchData> m_currentTouches;
    std::vector<TouchData> m_previousTouches;
    int m_maxTouchId;

    // Input frame history (for debugging and analytics)
    std::vector<InputFrame> m_frameHistory;
    static constexpr size_t MAX_FRAME_HISTORY = 60; // Keep 1 second of history at 60fps

    // Action mapping (configurable zones for different actions)
    struct ActionZone {
        float x1, y1, x2, y2; // Normalized coordinates
        InputAction action;
    };
    std::vector<ActionZone> m_actionZones;

    // Private methods
    void UpdateScreenInfo();
    void PollPlatformInput();
    void UpdateTouchStates();
    void ProcessActionMappings();
    void AddTouchToHistory(const TouchData& touch);
    void CleanupFrameHistory();
    TouchData CreateTouchData(int touchId, float rawX, float rawY, TouchState state) const;
    bool IsPointInZone(float x, float y, const ActionZone& zone) const;
    void SetupDefaultActionZones();
    uint64_t GetCurrentTimestamp() const;

    // Singleton instance
    static InputManager* s_instance;
};

} // namespace GameCore
