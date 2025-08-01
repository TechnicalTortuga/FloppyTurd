#pragma once

#include <cstdint>
#include <string>
#include <queue>

namespace GameCore {

    // Threading System - Command Queue for Thread-Safe Platform Interop
    // Enqueuing turd draw commands for silky-smooth rendering! 🚀
    
    enum class CommandType : uint32_t {
        // Rendering commands
        CMD_BEGIN_FRAME = 0,
        CMD_END_FRAME = 1,
        CMD_PRESENT = 2,
        CMD_CLEAR_SCREEN = 3,
        CMD_DRAW_SPRITE = 4,
        CMD_DRAW_SPRITE_SCALED = 5,
        CMD_DRAW_TEXT = 6,
        CMD_DRAW_RECTANGLE = 7,
        CMD_DRAW_CIRCLE = 8,
        CMD_GET_SCREEN_SIZE = 9,
        
        // Audio commands
        CMD_PLAY_MUSIC = 10,
        CMD_STOP_MUSIC = 11,
        CMD_PLAY_SOUND = 12,
        CMD_STOP_SOUND = 13,
        CMD_SET_MUSIC_VOLUME = 14,
        CMD_SET_SOUND_VOLUME = 15,
        
        // Logging commands
        CMD_LOG_TRACE = 16,
        CMD_LOG_DEBUG = 17,
        CMD_LOG_INFO = 18,
        CMD_LOG_WARN = 19,
        CMD_LOG_ERROR = 20,
        CMD_LOG_FATAL = 21,
        
        // Asset loading commands
        CMD_LOAD_TEXTURE = 22,
        CMD_LOAD_AUDIO = 23,
        CMD_LOAD_FONT = 24,
        CMD_LOAD_DATA = 25,
        // Asset cache management commands
        CMD_PRELOAD_ESSENTIAL_ASSETS = 26,
        CMD_IS_CACHED = 27
    };
    
    // Rendering command data
    struct RenderCommandData {
        // Common color/position fields
        float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
        float x = 0.0f, y = 0.0f;
        float width = 0.0f, height = 0.0f;
        float radius = 0.0f;
        float fontSize = 0.0f;
        float rotation = 0.0f;
        float scaleX = 1.0f, scaleY = 1.0f;
        
        // Pointer fields
        uint32_t textureHandle = 0;
        const char* text = nullptr;  // Caller ensures lifetime
        float* screenWidth = nullptr;
        float* screenHeight = nullptr;
    };
    
    // Audio command data
    struct AudioCommandData {
        float volume = 1.0f;
        float pitch = 1.0f;
        int loopCount = 0;  // -1 for infinite loop, 0 for no loop, >0 for specific count
        std::string audioFileName;  // For music and sound file names - FIXED: Use string instead of pointer
    };
    
    // Logging command data
    struct LogCommandData {
        std::string logMessage;
        std::string logCategory;
    };
    
    // Command structures
    struct RenderCommand {
        CommandType type;
        RenderCommandData data;
        
        // Constructors
        RenderCommand() : type(CommandType::CMD_BEGIN_FRAME) {}
        RenderCommand(CommandType t) : type(t) {}
    };
    
    struct AudioCommand {
        CommandType type;
        AudioCommandData data;
        
        // Constructors
        AudioCommand() : type(CommandType::CMD_PLAY_MUSIC) {}
        AudioCommand(CommandType t) : type(t) {}
    };
    
    struct LogCommand {
        CommandType type;
        LogCommandData data;
        
        // Constructors
        LogCommand() : type(CommandType::CMD_LOG_INFO) {}
        LogCommand(CommandType t) : type(t) {}
    };
    
    // Asset loading command data
    struct AssetCommandData {
        std::string assetPath;  // Full path to asset
        int fontSize = 16;  // For font loading
        void* callback = nullptr;  // Callback function pointer
        void* userData = nullptr;  // User context data for callback

        // For cache management
        std::string cacheAssetName; // For isCached
        int cacheAssetType = 0;    // For isCached: 0=texture, 1=audio, 2=font, 3=data
        bool cacheResult = false;  // For isCached result
    };
    
    struct AssetCommand {
        CommandType type;
        AssetCommandData data;
        
        // Constructors
        AssetCommand() : type(CommandType::CMD_LOAD_TEXTURE) {}
        AssetCommand(CommandType t) : type(t) {}
    };

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
        void (*present)();  // Present the frame to screen
        void (*clearScreen)(float r, float g, float b, float a);
        
        // Sprite rendering (using void* for sprite to avoid forward declaration issues)
        void (*drawSprite)(uint32_t textureHandle, float x, float y, float rotation);
        void (*drawSpriteScaled)(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation);
        
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
        RendererDelegate() : beginFrame(nullptr), endFrame(nullptr), present(nullptr), clearScreen(nullptr),
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
        void (*playSound)(const std::string& soundName, float volume);
        void (*stopSound)();
        
        // Background music
        void (*playMusic)(const char* musicName, float volume, int loopCount);
        void (*stopMusic)();
        
        // Volume control
        void (*setMusicVolume)(float volume);
        void (*setSFXVolume)(float volume);
        
        // Platform-specific context
        void* platformContext;
        
        // Initialize to null
        AudioDelegate() : playSound(nullptr), stopSound(nullptr),
                        playMusic(nullptr), stopMusic(nullptr),
                        setMusicVolume(nullptr), setSFXVolume(nullptr),
                        platformContext(nullptr) {}
    };

    // Asset loading delegates - platform-agnostic asset management
    struct TextureData {
        void* platformTexture;
        int width;
        int height;
        int format;
        int channels;
        size_t dataSize;
    };

    struct AssetDelegate {
        // Modern texture loading with user context using std::string for Swift interop
        void (*loadTexture)(const std::string& texturePath, 
                           void (*callback)(TextureData* texture, const char* error, void* userData),
                           void* userData);
        
        void (*loadAudio)(const std::string& audioPath, 
                         void (*callback)(void* audioData, size_t size, const char* error, void* userData),
                         void* userData);
        
        void (*loadFont)(const std::string& fontPath, int size, 
                        void (*callback)(void* fontData, const char* error, void* userData),
                        void* userData);
        
        void (*loadShader)(const std::string& vertexPath, const std::string& fragmentPath,
                          void (*callback)(void* shaderProgram, const char* error, void* userData),
                          void* userData);
        
        void (*loadData)(const std::string& dataPath,
                        void (*callback)(void* data, size_t size, const char* error, void* userData),
                        void* userData);
        
        // Asset lifecycle management
        void (*unloadAsset)(void* platformAsset);
        bool (*isAssetLoaded)(const char* assetPath);
        
        // Asset path resolution
        const char* (*getAssetPath)(const char* relativePath);
        bool (*fileExists)(const char* relativePath);
        
        // Asset cache management
        void (*preloadEssentialAssets)();
        bool (*isCached)(const char* assetName, int assetType);  // assetType: 0=texture, 1=audio, 2=font, 3=data
        // Platform-specific context
        void* platformContext;
        
        // Initialize to null
        AssetDelegate() : loadTexture(nullptr), loadAudio(nullptr), loadFont(nullptr),
                         loadShader(nullptr), loadData(nullptr), unloadAsset(nullptr),
                         isAssetLoaded(nullptr), getAssetPath(nullptr), fileExists(nullptr),
                         preloadEssentialAssets(nullptr), isCached(nullptr),
                         platformContext(nullptr) {}
    };

    // Logging delegate - platform-agnostic logging interface
    struct LogDelegate {
        // Logging functions
        void (*logTrace)(const char* message, const char* category);
        void (*logDebug)(const char* message, const char* category);
        void (*logInfo)(const char* message, const char* category);
        void (*logWarn)(const char* message, const char* category);
        void (*logError)(const char* message, const char* category);
        void (*logFatal)(const char* message, const char* category);
        
        // Platform-specific context
        void* platformContext;
        
        // Initialize to null
        LogDelegate() : logTrace(nullptr), logDebug(nullptr), logInfo(nullptr),
                       logWarn(nullptr), logError(nullptr), logFatal(nullptr),
                       platformContext(nullptr) {}
    };

    // Platform delegate container - holds all platform-specific delegates
    struct PlatformDelegates {
        RendererDelegate renderer;
        InputDelegate input;
        AudioDelegate audio;
        AssetDelegate asset;
        LogDelegate log;
        
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
                   audio.playSound != nullptr &&
                   asset.loadTexture != nullptr &&
                   asset.loadAudio != nullptr;
            // Note: Input delegates are not required for iOS as input is handled directly by Swift
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

} // namespace GameCore