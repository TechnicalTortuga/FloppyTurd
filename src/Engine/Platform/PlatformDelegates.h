#pragma once

#include <cstdint>

namespace FloppyTurd {

    // Forward declarations
    struct Sprite;

    // Action constants for input
    enum GameAction {
        ACTION_JUMP = 0,
        ACTION_PAUSE = 1,
        ACTION_MENU = 2,
        ACTION_RESTART = 3
    };

    // Renderer delegate - platform-agnostic rendering interface
    struct RendererDelegate {
        // Core rendering functions
        void (*beginFrame)();
        void (*endFrame)();
        void (*clearScreen)(float r, float g, float b, float a);
        
        // Sprite rendering (using void* for sprite to avoid forward declaration issues)
        void (*drawSprite)(void* sprite, float x, float y, float rotation);
        void (*drawSpriteScaled)(void* sprite, float x, float y, float scaleX, float scaleY, float rotation);
        
        // Text rendering
        void (*drawText)(const char* text, float x, float y, float fontSize, float r, float g, float b, float a);
        
        // Primitive rendering
        void (*drawRectangle)(float x, float y, float width, float height, float r, float g, float b, float a);
        void (*drawCircle)(float x, float y, float radius, float r, float g, float b, float a);
        
        // Screen info
        void (*getScreenSize)(float* width, float* height);
        
        // Platform-specific context (iOS: Swift objects, Raylib: global state)
        void* platformContext;
        
        // Initialize to null
        RendererDelegate() : beginFrame(nullptr), endFrame(nullptr), clearScreen(nullptr),
                           drawSprite(nullptr), drawSpriteScaled(nullptr), drawText(nullptr),
                           drawRectangle(nullptr), drawCircle(nullptr), getScreenSize(nullptr),
                           platformContext(nullptr) {}
    };

    // Input delegate - platform-agnostic input interface
    struct InputDelegate {
        // Action-based input (works for both touch and keyboard)
        bool (*isActionPressed)(int action);
        bool (*isActionJustPressed)(int action);  // Single frame press
        bool (*isActionReleased)(int action);
        
        // Position input (mouse/touch)
        void (*getPrimaryInputPosition)(float* x, float* y);  // Mouse or primary touch
        bool (*isPrimaryInputDown)();
        bool (*isPrimaryInputJustPressed)();
        bool (*isPrimaryInputJustReleased)();
        
        // Multi-touch (iOS specific, but can be stubbed for desktop)
        int (*getTouchCount)();
        void (*getTouchPosition)(int touchIndex, float* x, float* y);
        
        // Keyboard (desktop specific, but can be stubbed for iOS)
        bool (*isKeyPressed)(int keyCode);
        bool (*isKeyJustPressed)(int keyCode);
        
        // Platform-specific context
        void* platformContext;
        
        // Initialize to null
        InputDelegate() : isActionPressed(nullptr), isActionJustPressed(nullptr), isActionReleased(nullptr),
                        getPrimaryInputPosition(nullptr), isPrimaryInputDown(nullptr),
                        isPrimaryInputJustPressed(nullptr), isPrimaryInputJustReleased(nullptr),
                        getTouchCount(nullptr), getTouchPosition(nullptr),
                        isKeyPressed(nullptr), isKeyJustPressed(nullptr),
                        platformContext(nullptr) {}
    };

    // Audio delegate - platform-agnostic audio interface
    struct AudioDelegate {
        // Sound effects
        void (*playSound)(const char* soundName);
        void (*playSoundWithVolume)(const char* soundName, float volume);
        void (*stopSound)(const char* soundName);
        
        // Background music
        void (*playMusic)(const char* musicName);
        void (*stopMusic)();
        void (*pauseMusic)();
        void (*resumeMusic)();
        
        // Volume control
        void (*setMasterVolume)(float volume);  // 0.0 to 1.0
        void (*setSFXVolume)(float volume);
        void (*setMusicVolume)(float volume);
        
        // Audio state
        bool (*isMusicPlaying)();
        bool (*isSoundPlaying)(const char* soundName);
        
        // Platform-specific context
        void* platformContext;
        
        // Initialize to null
        AudioDelegate() : playSound(nullptr), playSoundWithVolume(nullptr), stopSound(nullptr),
                        playMusic(nullptr), stopMusic(nullptr), pauseMusic(nullptr), resumeMusic(nullptr),
                        setMasterVolume(nullptr), setSFXVolume(nullptr), setMusicVolume(nullptr),
                        isMusicPlaying(nullptr), isSoundPlaying(nullptr),
                        platformContext(nullptr) {}
    };

    // Platform delegate container - holds all platform-specific delegates
    struct PlatformDelegates {
        RendererDelegate renderer;
        InputDelegate input;
        AudioDelegate audio;
        
        // Platform identification
        enum PlatformType {
            PLATFORM_TYPE_UNKNOWN = 0,
            PLATFORM_TYPE_IOS = 1,
            PLATFORM_TYPE_MACOS = 2,
            PLATFORM_TYPE_WINDOWS = 3,
            PLATFORM_TYPE_LINUX = 4
        };
        
        PlatformType platformType;
        
        PlatformDelegates() : platformType(PLATFORM_TYPE_UNKNOWN) {}
        
        // Validation - check if all required delegates are set
        bool IsValid() const {
            return renderer.beginFrame != nullptr &&
                   renderer.endFrame != nullptr &&
                   renderer.clearScreen != nullptr &&
                   renderer.drawSprite != nullptr &&
                   input.isActionPressed != nullptr &&
                   input.getPrimaryInputPosition != nullptr &&
                   audio.playSound != nullptr;
        }
        
        // Get platform name as string
        const char* GetPlatformName() const {
            switch (platformType) {
                case PLATFORM_TYPE_IOS: return "iOS";
                case PLATFORM_TYPE_MACOS: return "macOS";
                case PLATFORM_TYPE_WINDOWS: return "Windows";
                case PLATFORM_TYPE_LINUX: return "Linux";
                default: return "Unknown";
            }
        }
    };

} // namespace FloppyTurd