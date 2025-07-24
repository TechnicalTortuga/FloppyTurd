#pragma once

#include "../Core/GnosisTypes.h"
#include <string>
#include <vector>

namespace Gnosis {

    /**
     * Abstract platform interface that defines the contract for platform-specific implementations.
     * This allows our game to run on different platforms (iOS, Android, Desktop) with the same core logic.
     */
    class IPlatform {
    public:
        virtual ~IPlatform() = default;
        
        // Platform lifecycle
        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual void Update(float deltaTime) = 0;
        
        // Platform information
        virtual std::string GetPlatformName() const = 0;
        virtual std::string GetPlatformVersion() const = 0;
        virtual GNVector2 GetScreenSize() const = 0;
        virtual float GetScreenScale() const = 0;
        
        // File system
        virtual std::string GetDocumentsPath() const = 0;
        virtual std::string GetCachePath() const = 0;
        virtual std::string GetResourcePath() const = 0;
        virtual bool FileExists(const std::string& path) const = 0;
        virtual std::vector<unsigned char> LoadFile(const std::string& path) const = 0;
        virtual bool SaveFile(const std::string& path, const std::vector<unsigned char>& data) const = 0;
        
        // System capabilities
        virtual bool HasTouchInput() const = 0;
        virtual bool HasKeyboard() const = 0;
        virtual bool HasGamepad() const = 0;
        virtual bool HasVibration() const = 0;
        
        // Platform-specific features
        virtual void ShowKeyboard() = 0;
        virtual void HideKeyboard() = 0;
        virtual void Vibrate(float intensity, float duration) = 0;
        virtual void OpenURL(const std::string& url) = 0;
        
        // Performance and memory
        virtual size_t GetAvailableMemory() const = 0;
        virtual float GetBatteryLevel() const = 0; // Returns -1 if not available
        virtual bool IsLowPowerMode() const = 0;
    };
    
    /**
     * Abstract renderer interface for platform-specific rendering implementations.
     * Abstracts away the differences between Metal (iOS), OpenGL, Vulkan, etc.
     */
    class IRenderer {
    public:
        virtual ~IRenderer() = default;
        
        // Renderer lifecycle
        virtual bool Initialize(int width, int height) = 0;
        virtual void Shutdown() = 0;
        virtual void Resize(int width, int height) = 0;
        
        // Frame management
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void Present() = 0;
        virtual void Clear(const GNColor& color) = 0;
        
        // Basic drawing primitives
        virtual void DrawRectangle(const GNRectangle& rect, const GNColor& color) = 0;
        virtual void DrawRectangleOutline(const GNRectangle& rect, const GNColor& color, float thickness = 1.0f) = 0;
        virtual void DrawCircle(const GNVector2& center, float radius, const GNColor& color) = 0;
        virtual void DrawCircleOutline(const GNVector2& center, float radius, const GNColor& color, float thickness = 1.0f) = 0;
        virtual void DrawLine(const GNVector2& start, const GNVector2& end, const GNColor& color, float thickness = 1.0f) = 0;
        
        // Texture management
        virtual GNTextureHandle LoadTexture(const std::string& path) = 0;
        virtual GNTextureHandle CreateTexture(int width, int height, const unsigned char* data) = 0;
        virtual void UnloadTexture(GNTextureHandle texture) = 0;
        virtual void DrawTexture(GNTextureHandle texture, const GNVector2& position, const GNColor& tint = GNColor()) = 0;
        virtual void DrawTextureEx(GNTextureHandle texture, const GNVector2& position, float rotation, float scale, const GNColor& tint = GNColor()) = 0;
        virtual void DrawTexturePro(GNTextureHandle texture, const GNRectangle& source, const GNRectangle& dest, const GNVector2& origin, float rotation, const GNColor& tint = GNColor()) = 0;
        
        // Text rendering
        virtual GNFontHandle LoadFont(const std::string& path, int fontSize) = 0;
        virtual GNFontHandle GetDefaultFont() = 0;
        virtual void UnloadFont(GNFontHandle font) = 0;
        virtual void DrawText(const std::string& text, const GNVector2& position, float fontSize, const GNColor& color, GNFontHandle font = INVALID_FONT_HANDLE) = 0;
        virtual GNVector2 MeasureText(const std::string& text, float fontSize, GNFontHandle font = INVALID_FONT_HANDLE) = 0;
        
        // Shader support (optional)
        virtual GNShaderHandle LoadShader(const std::string& vertexPath, const std::string& fragmentPath) = 0;
        virtual void UnloadShader(GNShaderHandle shader) = 0;
        virtual void BeginShaderMode(GNShaderHandle shader) = 0;
        virtual void EndShaderMode() = 0;
        virtual void SetShaderValue(GNShaderHandle shader, const std::string& uniformName, float value) = 0;
        virtual void SetShaderValue(GNShaderHandle shader, const std::string& uniformName, const GNVector2& value) = 0;
        virtual void SetShaderValue(GNShaderHandle shader, const std::string& uniformName, const GNColor& value) = 0;
        
        // Camera and transforms
        virtual void SetCamera(const GNVector2& position, float zoom, float rotation = 0.0f) = 0;
        virtual void ResetCamera() = 0;
        
        // Render targets (for post-processing)
        virtual GNRenderTargetHandle CreateRenderTarget(int width, int height) = 0;
        virtual void UnloadRenderTarget(GNRenderTargetHandle target) = 0;
        virtual void BeginRenderTarget(GNRenderTargetHandle target) = 0;
        virtual void EndRenderTarget() = 0;
        
        // Performance and debugging
        virtual void SetVSync(bool enabled) = 0;
        virtual int GetFPS() const = 0;
        virtual float GetFrameTime() const = 0;
        virtual std::string GetRendererInfo() const = 0;
    };
    
    /**
     * Abstract input handler interface for platform-specific input implementations.
     * Handles touch, keyboard, gamepad, and other input methods.
     */
    class IInputHandler {
    public:
        virtual ~IInputHandler() = default;
        
        // Input system lifecycle
        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual void Update() = 0;
        
        // Touch input (primary for mobile)
        virtual bool IsTouchDown(int touchId = 0) const = 0;
        virtual bool IsTouchPressed(int touchId = 0) const = 0;
        virtual bool IsTouchReleased(int touchId = 0) const = 0;
        virtual GNVector2 GetTouchPosition(int touchId = 0) const = 0;
        virtual int GetTouchCount() const = 0;
        
        // Keyboard input (for desktop/debugging)
        virtual bool IsKeyDown(KeyCode key) const = 0;
        virtual bool IsKeyPressed(KeyCode key) const = 0;
        virtual bool IsKeyReleased(KeyCode key) const = 0;
        virtual std::string GetKeyboardInput() const = 0;
        
        // Mouse input (for desktop)
        virtual bool IsMouseButtonDown(MouseButton button) const = 0;
        virtual bool IsMouseButtonPressed(MouseButton button) const = 0;
        virtual bool IsMouseButtonReleased(MouseButton button) const = 0;
        virtual GNVector2 GetMousePosition() const = 0;
        virtual float GetMouseWheelMove() const = 0;
        
        // Gamepad input (optional)
        virtual bool IsGamepadAvailable(int gamepadId) const = 0;
        virtual bool IsGamepadButtonDown(int gamepadId, GamepadButton button) const = 0;
        virtual bool IsGamepadButtonPressed(int gamepadId, GamepadButton button) const = 0;
        virtual bool IsGamepadButtonReleased(int gamepadId, GamepadButton button) const = 0;
        virtual float GetGamepadAxisMovement(int gamepadId, GamepadAxis axis) const = 0;
        
        // Gesture recognition (for touch devices)
        virtual bool IsGestureDetected(GestureType gesture) const = 0;
        virtual GNVector2 GetGesturePosition() const = 0;
        virtual float GetGestureDistance() const = 0;
        virtual float GetGestureAngle() const = 0;
        
        // Input mapping and actions
        virtual void MapInputAction(InputAction action, KeyCode key) = 0;
        virtual void MapInputAction(InputAction action, MouseButton button) = 0;
        virtual void MapInputAction(InputAction action, GamepadButton button, int gamepadId = 0) = 0;
        virtual bool IsActionDown(InputAction action) const = 0;
        virtual bool IsActionPressed(InputAction action) const = 0;
        virtual bool IsActionReleased(InputAction action) const = 0;
        
        // Platform-specific input features
        virtual void SetTouchSensitivity(float sensitivity) = 0;
        virtual void EnableGestures(bool enabled) = 0;
        virtual void SetVibrationEnabled(bool enabled) = 0;
    };
    
    /**
     * Abstract audio handler interface for platform-specific audio implementations.
     * Handles sound effects, music, and audio processing.
     */
    class IAudioHandler {
    public:
        virtual ~IAudioHandler() = default;
        
        // Audio system lifecycle
        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual void Update() = 0;
        
        // Sound loading and management
        virtual GNSoundHandle LoadSound(const std::string& path) = 0;
        virtual void UnloadSound(GNSoundHandle sound) = 0;
        virtual GNMusicHandle LoadMusic(const std::string& path) = 0;
        virtual void UnloadMusic(GNMusicHandle music) = 0;
        
        // Sound playback
        virtual void PlaySound(GNSoundHandle sound, float volume = 1.0f, float pitch = 1.0f) = 0;
        virtual void StopSound(GNSoundHandle sound) = 0;
        virtual void PauseSound(GNSoundHandle sound) = 0;
        virtual void ResumeSound(GNSoundHandle sound) = 0;
        virtual bool IsSoundPlaying(GNSoundHandle sound) const = 0;
        
        // Music playback
        virtual void PlayMusic(GNMusicHandle music, float volume = 1.0f) = 0;
        virtual void StopMusic() = 0;
        virtual void PauseMusic() = 0;
        virtual void ResumeMusic() = 0;
        virtual bool IsMusicPlaying() const = 0;
        virtual void SetMusicVolume(float volume) = 0;
        virtual void SetMusicPitch(float pitch) = 0;
        
        // Audio settings
        virtual void SetMasterVolume(float volume) = 0;
        virtual void SetSoundVolume(float volume) = 0;
        virtual float GetMasterVolume() const = 0;
        virtual float GetSoundVolume() const = 0;
        virtual float GetMusicVolumeLevel() const = 0;
        
        // Audio effects and processing
        virtual void SetSoundEffect(GNSoundHandle sound, AudioEffect effect, float value) = 0;
        virtual void SetMusicEffect(AudioEffect effect, float value) = 0;
        
        // 3D audio (optional)
        virtual void SetListenerPosition(const GNVector2& position) = 0;
        virtual void SetSoundPosition(GNSoundHandle sound, const GNVector2& position) = 0;
        virtual void SetSoundDistance(GNSoundHandle sound, float minDistance, float maxDistance) = 0;
        
        // Platform-specific audio features
        virtual bool IsAudioDeviceReady() const = 0;
        virtual std::string GetAudioDeviceInfo() const = 0;
        virtual void SetAudioLatency(float latency) = 0;
    };
    
    /**
     * @struct PlatformInterfaces
     * @brief Container for platform-specific implementations
     * 
     * This struct holds unique pointers to platform-specific implementations
     * of the core interfaces. It serves as the control flow divergence point
     * where platform entry points (iOS AppDelegate, Desktop main) instantiate
     * their respective implementations and pass them to the Engine.
     */
    struct PlatformInterfaces {
        std::unique_ptr<IRenderer> renderer;
        std::unique_ptr<IInputHandler> inputHandler;
        std::unique_ptr<IAudioHandler> audioHandler;
        std::unique_ptr<IPlatform> platform;
        
        PlatformInterfaces() = default;
        
        PlatformInterfaces(std::unique_ptr<IRenderer> r, 
                          std::unique_ptr<IInputHandler> i,
                          std::unique_ptr<IAudioHandler> a,
                          std::unique_ptr<IPlatform> p = nullptr)
            : renderer(std::move(r))
            , inputHandler(std::move(i))
            , audioHandler(std::move(a))
            , platform(std::move(p)) {}
        
        // Move constructor
        PlatformInterfaces(PlatformInterfaces&& other) noexcept
            : renderer(std::move(other.renderer))
            , inputHandler(std::move(other.inputHandler))
            , audioHandler(std::move(other.audioHandler))
            , platform(std::move(other.platform)) {}
        
        // Move assignment operator
        PlatformInterfaces& operator=(PlatformInterfaces&& other) noexcept {
            if (this != &other) {
                renderer = std::move(other.renderer);
                inputHandler = std::move(other.inputHandler);
                audioHandler = std::move(other.audioHandler);
                platform = std::move(other.platform);
            }
            return *this;
        }
        
        // Delete copy constructor and copy assignment
        PlatformInterfaces(const PlatformInterfaces&) = delete;
        PlatformInterfaces& operator=(const PlatformInterfaces&) = delete;
        
        /**
         * @brief Check if all required interfaces are available
         * @return true if renderer, inputHandler, and audioHandler are all valid
         */
        bool IsValid() const {
             return renderer && inputHandler && audioHandler;
         }
     };

} // namespace Gnosis