#pragma once

#include <cstdint>
#include <string>
#include <queue>

namespace GameCore {

    // Enhanced Screen Information for Dynamic Resolution Support
    struct ScreenInfo {
        float pixelWidth;        // Actual pixel dimensions (e.g., 1179 for iPhone 16)
        float pixelHeight;       // Actual pixel dimensions (e.g., 2556 for iPhone 16)
        float logicalWidth;      // Logical coordinate space (e.g., 393 for iPhone 16)
        float logicalHeight;     // Logical coordinate space (e.g., 852 for iPhone 16)
        float scaleFactor;       // Pixel to logical ratio (e.g., 3.0 for iPhone 16)
        bool isPortrait;         // Orientation flag
        std::string deviceModel; // For device-specific optimizations
        
        ScreenInfo() : pixelWidth(800.0f), pixelHeight(600.0f), 
                      logicalWidth(800.0f), logicalHeight(600.0f),
                      scaleFactor(1.0f), isPortrait(false), deviceModel("Unknown") {}
    };

    // Enhanced Texture Metadata for Dynamic Asset Management
    struct TextureMetadata {
        int width;               // Actual texture width in pixels
        int height;              // Actual texture height in pixels
        int channels;            // Number of color channels (3=RGB, 4=RGBA)
        std::string format;      // Pixel format (e.g., "RGBA8", "RGB8")
        size_t dataSize;         // Size of texture data in bytes
        bool isLoaded;           // Loading status
        std::string assetPath;   // Original asset path
        uint32_t platformHandle; // Platform-specific texture handle
        
        TextureMetadata() : width(0), height(0), channels(4), format("RGBA8"), 
                           dataSize(0), isLoaded(false), assetPath(""), platformHandle(0) {}
        
        // Explicit destructor to ensure proper cleanup
        ~TextureMetadata() {
            // No need to free platformHandle here - that's managed by the platform layer
            // Just ensure we don't leave any dangling references
            platformHandle = 0;
        }
        
        // Copy constructor
        TextureMetadata(const TextureMetadata& other) : 
            width(other.width), 
            height(other.height),
            channels(other.channels),
            format(other.format),
            dataSize(other.dataSize),
            isLoaded(other.isLoaded),
            assetPath(other.assetPath),
            platformHandle(other.platformHandle) {}
        
        // Move constructor
        TextureMetadata(TextureMetadata&& other) noexcept :
            width(other.width),
            height(other.height),
            channels(other.channels),
            format(std::move(other.format)),
            dataSize(other.dataSize),
            isLoaded(other.isLoaded),
            assetPath(std::move(other.assetPath)),
            platformHandle(other.platformHandle) {
            // Clear the source object's platform handle to prevent double-free
            other.platformHandle = 0;
        }
        
        // Copy assignment operator
        TextureMetadata& operator=(const TextureMetadata& other) {
            if (this != &other) {
                width = other.width;
                height = other.height;
                channels = other.channels;
                format = other.format;
                dataSize = other.dataSize;
                isLoaded = other.isLoaded;
                assetPath = other.assetPath;
                platformHandle = other.platformHandle;
            }
            return *this;
        }
        
        // Move assignment operator
        TextureMetadata& operator=(TextureMetadata&& other) noexcept {
            if (this != &other) {
                width = other.width;
                height = other.height;
                channels = other.channels;
                format = std::move(other.format);
                dataSize = other.dataSize;
                isLoaded = other.isLoaded;
                assetPath = std::move(other.assetPath);
                platformHandle = other.platformHandle;
                
                // Clear the source object's platform handle
                other.platformHandle = 0;
            }
            return *this;
        }
    };

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
        CMD_DRAW_SPRITE_SCALED_CENTERED = 6,
        CMD_DRAW_SPRITE_SCALED_PIVOTED = 7,
        CMD_DRAW_SPRITE_SCALED_WITH_SOURCE = 8,
        CMD_DRAW_PARALLAX_SPRITE = 36,           // Pixel-perfect parallax rendering
        CMD_DRAW_BACKGROUND_SPRITE = 37,         // Integer-position background rendering
        CMD_DRAW_TEXT = 9,
        CMD_DRAW_TEXT_CENTERED = 10,
        CMD_DRAW_RECTANGLE = 11,
        CMD_DRAW_CIRCLE = 12,
        CMD_GET_SCREEN_SIZE = 13,
        CMD_GET_SCREEN_INFO = 38,
        CMD_GET_TEXTURE_METADATA = 39,
        CMD_LOCK_ORIENTATION = 40,
        CMD_UNLOCK_ORIENTATION = 41,
        CMD_LOCK_TO_PORTRAIT = 42,
        CMD_LOCK_TO_LANDSCAPE = 43,

        // Audio commands
        CMD_PLAY_MUSIC = 14,
        CMD_STOP_MUSIC = 15,
        CMD_PLAY_SOUND = 16,
        CMD_STOP_SOUND = 17,
        CMD_SET_MUSIC_VOLUME = 18,
        CMD_SET_SOUND_VOLUME = 19,
        
        // Logging commands
        CMD_LOG_TRACE = 20,
        CMD_LOG_DEBUG = 21,
        CMD_LOG_INFO = 22,
        CMD_LOG_WARN = 23,
        CMD_LOG_ERROR = 24,
        CMD_LOG_FATAL = 25,
        
        // Asset loading commands
        CMD_LOAD_TEXTURE = 26,
        CMD_LOAD_AUDIO = 27,
        CMD_LOAD_FONT = 28,
        CMD_LOAD_DATA = 29,
        // Asset cache management commands
        CMD_PRELOAD_ESSENTIAL_ASSETS = 30,
        CMD_IS_CACHED = 31,
        
        // Enhanced screen and texture info commands - moved to main section
        // Text outline commands
        CMD_DRAW_TEXT_OUTLINED = 34,
        CMD_DRAW_TEXT_CENTERED_OUTLINED = 35
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
        // Pivot point for rotation (normalized coordinates, -0.5 to 0.5)
        float pivotX = 0.0f, pivotY = 0.0f;
        // Outline parameters (for outlined text)
        float outlineR = 0.0f, outlineG = 0.0f, outlineB = 0.0f, outlineA = 1.0f;
        float outlineWidth = 0.0f; // pixels
        
        // Source rectangle for sprite sheets
        float sourceX = 0.0f, sourceY = 0.0f, sourceWidth = 0.0f, sourceHeight = 0.0f;
        
        // Pointer fields
        uint32_t textureHandle = 0;
        std::string text;  // Use std::string for proper Swift interop
        float* screenWidth = nullptr;
        float* screenHeight = nullptr;
        
        // Enhanced screen and texture info fields
        ScreenInfo* screenInfo = nullptr;      // For CMD_GET_SCREEN_INFO
        std::string textureId;                 // For CMD_GET_TEXTURE_METADATA
        TextureMetadata textureMetadata;       // For CMD_GET_TEXTURE_METADATA - OWNED by command
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
        void (*drawSpriteScaledCentered)(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation);
        void (*drawSpriteScaledPivoted)(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float pivotX, float pivotY);
        void (*drawSpriteScaledWithSource)(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float sourceX, float sourceY, float sourceWidth, float sourceHeight);
        
        // Pixel-perfect parallax rendering functions
        void (*drawParallaxSprite)(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float sourceX, float sourceY, float sourceWidth, float sourceHeight);
        void (*drawBackgroundSprite)(uint32_t textureHandle, int pixelX, int pixelY, int pixelWidth, int pixelHeight);
        
        // Text rendering
        void (*drawText)(const std::string& text, float x, float y, float fontSize, float r, float g, float b, float a);
        void (*drawTextCentered)(const std::string& text, float x, float y, float fontSize, float r, float g, float b, float a);
        // Outlined text rendering
        void (*drawTextOutlined)(const std::string& text, float x, float y, float fontSize,
                                 float textR, float textG, float textB, float textA,
                                 float outlineR, float outlineG, float outlineB, float outlineA,
                                 float outlineWidth);
        void (*drawTextCenteredOutlined)(const std::string& text, float x, float y, float fontSize,
                                         float textR, float textG, float textB, float textA,
                                         float outlineR, float outlineG, float outlineB, float outlineA,
                                         float outlineWidth);
        
        // Primitive rendering
        void (*drawRectangle)(float x, float y, float width, float height, float r, float g, float b, float a);
        void (*drawCircle)(float x, float y, float radius, float r, float g, float b, float a);
        
        // Enhanced screen and texture information
        void (*getScreenInfo)(ScreenInfo* info);        // NEW: Get comprehensive screen info
        void (*getScreenSize)(float* width, float* height); // Legacy support
        bool (*getTextureMetadata)(const char* textureId, TextureMetadata* metadata); // NEW: Dynamic texture info

        // Orientation control (iOS specific)
        void (*lockOrientation)();                       // Lock current orientation
        void (*unlockOrientation)();                     // Allow orientation changes
        void (*lockToPortrait)();                        // Lock to portrait only
        void (*lockToLandscape)();                       // Lock to landscape only
        
        // Platform-specific context (iOS: Swift objects, Raylib: global state)
        void* platformContext;
        
        // Initialize to null
        RendererDelegate() : beginFrame(nullptr), endFrame(nullptr), present(nullptr), clearScreen(nullptr),
                           drawSprite(nullptr), drawSpriteScaled(nullptr), drawSpriteScaledCentered(nullptr),
                           drawSpriteScaledPivoted(nullptr),
                           drawSpriteScaledWithSource(nullptr),
                           drawParallaxSprite(nullptr), drawBackgroundSprite(nullptr),
                           drawText(nullptr), drawTextCentered(nullptr), drawTextOutlined(nullptr), drawTextCenteredOutlined(nullptr),
                           drawRectangle(nullptr), drawCircle(nullptr),
                           getScreenInfo(nullptr), getScreenSize(nullptr), getTextureMetadata(nullptr),
                           lockOrientation(nullptr), unlockOrientation(nullptr),
                           lockToPortrait(nullptr), lockToLandscape(nullptr),
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
        bool (*isTouchDown)();
        bool (*isTouchJustPressed)();
        bool (*isTouchJustReleased)();
        
        // Gesture detection (iOS specific)
        bool (*isSwipeLeftDetected)();
        bool (*isSwipeRightDetected)();
        bool (*isSwipeUpDetected)();
        bool (*isSwipeDownDetected)();
        void (*resetGestureState)();
        
        // Input buffer management
        void (*clearInputBuffer)();
        
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
                        isTouchDown(nullptr), isTouchJustPressed(nullptr), isTouchJustReleased(nullptr),
                        isSwipeLeftDetected(nullptr), isSwipeRightDetected(nullptr),
                        isSwipeUpDetected(nullptr), isSwipeDownDetected(nullptr),
                        resetGestureState(nullptr), clearInputBuffer(nullptr),
                        isKeyPressed(nullptr), isKeyJustPressed(nullptr),
                        platformContext(nullptr) {}
    };

    // Audio delegate - platform-agnostic audio interface
    struct AudioDelegate {
        // Sound effects
        void (*playSound)(const char* soundName, float volume);
        void (*stopSound)(const char* soundName);  // Stop specific sound by name

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
        
        // Enhanced texture metadata support
        bool (*getTextureMetadata)(const char* textureId, TextureMetadata* metadata); // NEW: Get texture dimensions dynamically
        void (*cacheTextureMetadata)(const char* textureId, const TextureMetadata* metadata); // NEW: Cache metadata
        
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
                         isAssetLoaded(nullptr), getTextureMetadata(nullptr), cacheTextureMetadata(nullptr),
                         getAssetPath(nullptr), fileExists(nullptr),
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