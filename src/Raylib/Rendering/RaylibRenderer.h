#ifndef RAYLIB_PLATFORM_H
#define RAYLIB_PLATFORM_H

#include "../../Engine/Platform/PlatformInterfaces.h"
#include "../../Engine/Core/GnosisTypes.h"
#include <map>
#include <vector>
#include <string>

// Forward declarations for Raylib types
struct Texture2D;
struct Font;
struct Shader;
struct RenderTexture2D;
struct Sound;
struct Music;
struct Camera2D;
struct Color;
struct Rectangle;
struct Vector2;

namespace Gnosis {

    /**
     * Raylib implementation of the platform interface.
     * Provides cross-platform functionality using Raylib as the backend.
     */
    class RaylibPlatform : public IPlatform {
    public:
        RaylibPlatform();
        virtual ~RaylibPlatform() throw();
        
        // Platform lifecycle
        virtual bool Initialize() override;
        virtual void Shutdown() override;
        virtual void Update(float deltaTime) override;
        
        // Platform information
        virtual std::string GetPlatformName() const override;
        virtual std::string GetPlatformVersion() const override;
        virtual GNVector2 GetScreenSize() const override;
        virtual float GetScreenScale() const override;
        
        // File system
        virtual std::string GetDocumentsPath() const override;
        virtual std::string GetCachePath() const override;
        virtual std::string GetResourcePath() const override;
        virtual bool FileExists(const std::string& path) const override;
        virtual std::vector<unsigned char> LoadFile(const std::string& path) const override;
        virtual bool SaveFile(const std::string& path, const std::vector<unsigned char>& data) const override;
        
        // System capabilities
        virtual bool HasTouchInput() const override;
        virtual bool HasKeyboard() const override;
        virtual bool HasGamepad() const override;
        virtual bool HasVibration() const override;
        
        // Platform-specific features
        virtual void ShowKeyboard() override;
        virtual void HideKeyboard() override;
        virtual void Vibrate(float intensity, float duration) override;
        virtual void OpenURL(const std::string& url) override;
        
        // Performance and memory
        virtual size_t GetAvailableMemory() const override;
        virtual float GetBatteryLevel() const override;
        virtual bool IsLowPowerMode() const override;
        
        // Additional methods for window management (not in interface)
        bool InitializeWindow(int width, int height, const std::string& title);
        void SetWindowTitle(const std::string& title);
        void SetWindowSize(int width, int height);
        bool IsWindowFullscreen() const;
        void ToggleFullscreen();
        void SetTargetFPS(int fps);
        int GetFPS() const;
        float GetFrameTime() const;
        bool ShouldClose() const;
        
    private:
        bool m_initialized;
        std::string m_windowTitle;
        int m_windowWidth;
        int m_windowHeight;
        bool m_isMobile;
    };
    
    /**
     * Raylib implementation of the renderer interface.
     * Handles all rendering operations using Raylib's graphics API.
     */
    class RaylibRenderer : public IRenderer {
    public:
        RaylibRenderer();
        virtual ~RaylibRenderer() throw();
        
        // Renderer lifecycle
        virtual bool Initialize(int width, int height) override;
        virtual void Shutdown() override;
        virtual void Resize(int width, int height) override;
        
        // Frame management
        virtual void BeginFrame() override;
        virtual void EndFrame() override;
        virtual void Present() override;
        virtual void Clear(const GNColor& color) override;
        
        // Basic drawing primitives
        virtual void DrawRectangle(const GNRectangle& rect, const GNColor& color) override;
        virtual void DrawRectangleOutline(const GNRectangle& rect, const GNColor& color, float thickness = 1.0f) override;
        virtual void DrawCircle(const GNVector2& center, float radius, const GNColor& color) override;
        virtual void DrawCircleOutline(const GNVector2& center, float radius, const GNColor& color, float thickness = 1.0f) override;
        virtual void DrawLine(const GNVector2& start, const GNVector2& end, const GNColor& color, float thickness = 1.0f) override;
        
        // Texture management
        virtual GNTextureHandle LoadTexture(const std::string& path) override;
        virtual GNTextureHandle CreateTexture(int width, int height, const unsigned char* data) override;
        virtual void UnloadTexture(GNTextureHandle texture) override;
        virtual void DrawTexture(GNTextureHandle texture, const GNVector2& position, const GNColor& tint = GNColor()) override;
        virtual void DrawTextureEx(GNTextureHandle texture, const GNVector2& position, float rotation, float scale, const GNColor& tint = GNColor()) override;
        virtual void DrawTexturePro(GNTextureHandle texture, const GNRectangle& source, const GNRectangle& dest, const GNVector2& origin, float rotation, const GNColor& tint = GNColor()) override;
        
        // Text rendering
        virtual GNFontHandle LoadFont(const std::string& path, int fontSize) override;
        virtual GNFontHandle GetDefaultFont() override;
        virtual void UnloadFont(GNFontHandle font) override;
        virtual void DrawText(const std::string& text, const GNVector2& position, float fontSize, const GNColor& color, GNFontHandle font = INVALID_FONT_HANDLE) override;
        virtual GNVector2 MeasureText(const std::string& text, float fontSize, GNFontHandle font = INVALID_FONT_HANDLE) override;
        
        // Shader support (optional)
        virtual GNShaderHandle LoadShader(const std::string& vertexPath, const std::string& fragmentPath) override;
        virtual void UnloadShader(GNShaderHandle shader) override;
        virtual void BeginShaderMode(GNShaderHandle shader) override;
        virtual void EndShaderMode() override;
        virtual void SetShaderValue(GNShaderHandle shader, const std::string& uniformName, float value) override;
        virtual void SetShaderValue(GNShaderHandle shader, const std::string& uniformName, const GNVector2& value) override;
        virtual void SetShaderValue(GNShaderHandle shader, const std::string& uniformName, const GNColor& value) override;
        
        // Camera and transforms
        virtual void SetCamera(const GNVector2& position, float zoom, float rotation = 0.0f) override;
        virtual void ResetCamera() override;
        
        // Render targets (for post-processing)
        virtual GNRenderTargetHandle CreateRenderTarget(int width, int height) override;
        virtual void UnloadRenderTarget(GNRenderTargetHandle target) override;
        virtual void BeginRenderTarget(GNRenderTargetHandle target) override;
        virtual void EndRenderTarget() override;
        
        // Performance and debugging
        virtual void SetVSync(bool enabled) override;
        virtual int GetFPS() const override;
        virtual float GetFrameTime() const override;
        virtual std::string GetRendererInfo() const override;
        
    private:
        // Helper methods
        Color ToRaylibColor(const GNColor& color) const;
        GNColor FromRaylibColor(const Color& color) const;
        Rectangle ToRaylibRectangle(const GNRectangle& rect) const;
        Vector2 ToRaylibVector2(const GNVector2& vec) const;
        
        // Resource management
        std::map<GNTextureHandle, Texture2D> m_textures;
        std::map<GNFontHandle, Font> m_fonts;
        std::map<GNShaderHandle, Shader> m_shaders;
        std::map<GNRenderTargetHandle, RenderTexture2D> m_renderTargets;
        
        // Handle generation
        GNTextureHandle m_nextTextureHandle;
        GNFontHandle m_nextFontHandle;
        GNShaderHandle m_nextShaderHandle;
        GNRenderTargetHandle m_nextRenderTargetHandle;
        
        // Camera state
        Camera2D* m_camera;
        bool m_cameraActive;
        
        bool m_initialized;
        int m_screenWidth;
        int m_screenHeight;
    };
    
    /**
     * Raylib implementation of the input handler interface.
     * Handles touch, keyboard, mouse, and gamepad input using Raylib.
     */
    class RaylibInputHandler : public IInputHandler {
    public:
        RaylibInputHandler();
        virtual ~RaylibInputHandler() throw();
        
        // Input system lifecycle
        virtual bool Initialize() override;
        virtual void Shutdown() override;
        virtual void Update() override;
        
        // Touch input (primary for mobile)
        virtual bool IsTouchDown(int touchId = 0) const override;
        virtual bool IsTouchPressed(int touchId = 0) const override;
        virtual bool IsTouchReleased(int touchId = 0) const override;
        virtual GNVector2 GetTouchPosition(int touchId = 0) const override;
        virtual int GetTouchCount() const override;
        
        // Keyboard input (for desktop/debugging)
        virtual bool IsKeyDown(KeyCode key) const override;
        virtual bool IsKeyPressed(KeyCode key) const override;
        virtual bool IsKeyReleased(KeyCode key) const override;
        virtual std::string GetKeyboardInput() const override;
        
        // Mouse input (for desktop)
        virtual bool IsMouseButtonDown(MouseButton button) const override;
        virtual bool IsMouseButtonPressed(MouseButton button) const override;
        virtual bool IsMouseButtonReleased(MouseButton button) const override;
        virtual GNVector2 GetMousePosition() const override;
        virtual float GetMouseWheelMove() const override;
        
        // Gamepad input (optional)
        virtual bool IsGamepadAvailable(int gamepadId) const override;
        virtual bool IsGamepadButtonDown(int gamepadId, GamepadButton button) const override;
        virtual bool IsGamepadButtonPressed(int gamepadId, GamepadButton button) const override;
        virtual bool IsGamepadButtonReleased(int gamepadId, GamepadButton button) const override;
        virtual float GetGamepadAxisMovement(int gamepadId, GamepadAxis axis) const override;
        
        // Gesture recognition (for touch devices)
        virtual bool IsGestureDetected(GestureType gesture) const override;
        virtual GNVector2 GetGesturePosition() const override;
        virtual float GetGestureDistance() const override;
        virtual float GetGestureAngle() const override;
        
        // Input mapping and actions
        virtual void MapInputAction(InputAction action, KeyCode key) override;
        virtual void MapInputAction(InputAction action, MouseButton button) override;
        virtual void MapInputAction(InputAction action, GamepadButton button, int gamepadId = 0) override;
        virtual bool IsActionDown(InputAction action) const override;
        virtual bool IsActionPressed(InputAction action) const override;
        virtual bool IsActionReleased(InputAction action) const override;
        
        // Platform-specific input features
        virtual void SetTouchSensitivity(float sensitivity) override;
        virtual void EnableGestures(bool enabled) override;
        virtual void SetVibrationEnabled(bool enabled) override;
        
    private:
        // Helper methods
        int ToRaylibKey(KeyCode key) const;
        int ToRaylibMouseButton(MouseButton button) const;
        int ToRaylibGamepadButton(GamepadButton button) const;
        int ToRaylibGamepadAxis(GamepadAxis axis) const;
        int ToRaylibGesture(GestureType gesture) const;
        
        // Input mapping storage
        struct InputMapping {
            enum Type { KEY, MOUSE, GAMEPAD } type;
            union {
                KeyCode key;
                MouseButton mouseButton;
                struct {
                    GamepadButton button;
                    int gamepadId;
                } gamepad;
            } input;
        };
        
        std::map<InputAction, std::vector<InputMapping> > m_inputMappings;
        
        // Touch state tracking
        struct TouchState {
            bool isDown;
            bool wasPressed;
            bool wasReleased;
            GNVector2 position;
        };
        
        std::vector<TouchState> m_touchStates;
        float m_touchSensitivity;
        bool m_gesturesEnabled;
        bool m_vibrationEnabled;
        bool m_initialized;
    };
    
    /**
     * Raylib implementation of the audio handler interface.
     * Handles sound effects and music using Raylib's audio system.
     */
    class RaylibAudioHandler : public IAudioHandler {
    public:
        RaylibAudioHandler();
        virtual ~RaylibAudioHandler() throw();
        
        // Audio system lifecycle
        virtual bool Initialize() override;
        virtual void Shutdown() override;
        virtual void Update() override;
        
        // Sound loading and management
        virtual GNSoundHandle LoadSound(const std::string& path) override;
        virtual void UnloadSound(GNSoundHandle sound) override;
        virtual GNMusicHandle LoadMusic(const std::string& path) override;
        virtual void UnloadMusic(GNMusicHandle music) override;
        
        // Sound playback
        virtual void PlaySound(GNSoundHandle sound, float volume = 1.0f, float pitch = 1.0f) override;
        virtual void StopSound(GNSoundHandle sound) override;
        virtual void PauseSound(GNSoundHandle sound) override;
        virtual void ResumeSound(GNSoundHandle sound) override;
        virtual bool IsSoundPlaying(GNSoundHandle sound) const override;
        
        // Music playback
        virtual void PlayMusic(GNMusicHandle music, float volume = 1.0f) override;
        virtual void StopMusic() override;
        virtual void PauseMusic() override;
        virtual void ResumeMusic() override;
        virtual bool IsMusicPlaying() const override;
        virtual void SetMusicVolume(float volume) override;
        virtual void SetMusicPitch(float pitch) override;
        
        // Audio settings
        virtual void SetMasterVolume(float volume) override;
        virtual void SetSoundVolume(float volume) override;
        virtual float GetMasterVolume() const override;
        virtual float GetSoundVolume() const override;
        virtual float GetMusicVolumeLevel() const override;
        
        // Audio effects and processing
        virtual void SetSoundEffect(GNSoundHandle sound, AudioEffect effect, float value) override;
        virtual void SetMusicEffect(AudioEffect effect, float value) override;
        
        // 3D audio (optional)
        virtual void SetListenerPosition(const GNVector2& position) override;
        virtual void SetSoundPosition(GNSoundHandle sound, const GNVector2& position) override;
        virtual void SetSoundDistance(GNSoundHandle sound, float minDistance, float maxDistance) override;
        
        // Platform-specific audio features
        virtual bool IsAudioDeviceReady() const override;
        virtual std::string GetAudioDeviceInfo() const override;
        virtual void SetAudioLatency(float latency) override;
        
    private:
        // Resource management
        std::map<GNSoundHandle, Sound> m_sounds;
        std::map<GNMusicHandle, Music> m_music;
        
        // Handle generation
        GNSoundHandle m_nextSoundHandle;
        GNMusicHandle m_nextMusicHandle;
        
        // Audio state
        float m_masterVolume;
        float m_soundVolume;
        float m_musicVolume;
        
        // Current music tracking
        GNMusicHandle m_currentMusic;
        
        bool m_initialized;
    };
    
} // namespace Gnosis

#endif // RAYLIB_PLATFORM_H