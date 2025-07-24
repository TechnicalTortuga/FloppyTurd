#ifndef RAYLIB_INPUT_HANDLER_H
#define RAYLIB_INPUT_HANDLER_H

#include <string>
#include "../../Engine/Platform/PlatformDelegates.h"

namespace Gnosis {

    /**
     * @brief Raylib implementation of input handling for desktop platforms
     * 
     * This class implements the IInputHandler interface using Raylib's input functions
     * for keyboard, mouse, and gamepad input on desktop platforms.
     */
    class RaylibInputHandler : public IInputHandler {
    public:
        RaylibInputHandler();
        virtual ~RaylibInputHandler() throw();

        // IInputHandler interface implementation
        bool Initialize() override;
        void Shutdown() override;
        void Update() override;

        // Touch input methods (not applicable for desktop)
        bool IsTouchDown(int touchId) const override;
        bool IsTouchPressed(int touchId) const override;
        bool IsTouchReleased(int touchId) const override;
        GNVector2 GetTouchPosition(int touchId) const override;
        int GetTouchCount() const override;

        // Keyboard input methods
        bool IsKeyDown(KeyCode key) const override;
        bool IsKeyPressed(KeyCode key) const override;
        bool IsKeyReleased(KeyCode key) const override;
        std::string GetKeyboardInput() const override;

        // Mouse input methods
        bool IsMouseButtonDown(MouseButton button) const override;
        bool IsMouseButtonPressed(MouseButton button) const override;
        bool IsMouseButtonReleased(MouseButton button) const override;
        GNVector2 GetMousePosition() const override;
        float GetMouseWheelMove() const override;

        // Gamepad input methods
        bool IsGamepadAvailable(int gamepadId) const override;
        bool IsGamepadButtonDown(int gamepadId, GamepadButton button) const override;
        bool IsGamepadButtonPressed(int gamepadId, GamepadButton button) const override;
        bool IsGamepadButtonReleased(int gamepadId, GamepadButton button) const override;
        float GetGamepadAxisMovement(int gamepadId, GamepadAxis axis) const override;

        // Gesture recognition methods (not applicable for desktop)
        bool IsGestureDetected(GestureType gesture) const override;
        GNVector2 GetGesturePosition() const override;
        float GetGestureDistance() const override;
        float GetGestureAngle() const override;

        // Input mapping methods
        void MapInputAction(InputAction action, KeyCode key) override;
        void MapInputAction(InputAction action, MouseButton button) override;
        void MapInputAction(InputAction action, GamepadButton button, int gamepadId) override;
        bool IsActionDown(InputAction action) const override;
        bool IsActionPressed(InputAction action) const override;
        bool IsActionReleased(InputAction action) const override;

        // Platform-specific input features
        void SetTouchSensitivity(float sensitivity) override;
        void EnableGestures(bool enabled) override;
        void SetVibrationEnabled(bool enabled) override;

    private:
        // Helper methods for Raylib conversion
        int ToRaylibKey(KeyCode key) const;
        int ToRaylibMouseButton(MouseButton button) const;
        int ToRaylibGamepadButton(GamepadButton button) const;
        int ToRaylibGamepadAxis(GamepadAxis axis) const;

        // Input mapping storage
        std::map<InputAction, std::vector<KeyCode> > m_keyMappings;
        std::map<InputAction, std::vector<MouseButton> > m_mouseMappings;
        std::map<InputAction, std::pair<GamepadButton, int> > m_gamepadMappings;

        // Configuration
        bool m_initialized;
    };

} // namespace Gnosis

#endif // RAYLIB_INPUT_HANDLER_H