#pragma once

#include <vector>
#include <mutex>
#include <memory>
#include "../../Engine/Platform/PlatformDelegates.h"

// Forward declaration for TouchData
namespace GameCore {
    struct TouchData;
}

namespace GameCore {
    
    /**
     * @class ThreadingProxy
     * @brief Thread-safe command queue proxy for iOS platform interop
     * 
     * Enqueues rendering commands from C++ game logic and allows Swift to dequeue them safely on the main thread.
     * Follows Apple's C++/Swift interop best practices: Simple POD structs for commands, std::vector for bridging to Array,
     * no unions (flattened data), and mutex protection for thread safety (though C++ game loop is single-threaded).
     * 
     * Usage:
     * - C++: Enqueue via delegates.
     * - Swift: Get vector of commands via getAndClearCommands() (UnsafePointer to call), process on main.
     * 
     * Features:
     * - Batch dequeuing via vector for efficient interop (avoids per-command calls).
     * - Flattened command data to avoid union limitations in interop.
     * - Scalable for optimizations like command sorting.
     */
    class ThreadingProxy {
    public:
        ThreadingProxy();
        ~ThreadingProxy();
        
        // Command queue management
        static bool hasCommands();
        static void clearCommands();
        
        // Rendering commands
        static void enqueueBeginFrame();
        static void enqueueEndFrame();
        static void enqueuePresent();
        static void enqueueClearScreen(float r, float g, float b, float a);
        static void enqueueDrawSprite(uint32_t textureHandle, float x, float y, float rotation);
        static void enqueueDrawSpriteScaled(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation);
        static void enqueueDrawSpriteScaledCentered(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation);
        static void enqueueDrawSpriteScaledPivoted(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float pivotX, float pivotY);
        static void enqueueDrawSpriteScaledWithSource(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float sourceX, float sourceY, float sourceWidth, float sourceHeight);
        static void enqueueDrawSpriteScaledWithSourceCentered(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float sourceX, float sourceY, float sourceWidth, float sourceHeight);
        static void enqueueDrawSpriteScaledWithSourcePivoted(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float pivotX, float pivotY, float sourceX, float sourceY, float sourceWidth, float sourceHeight);
        
        // Batch rendering - draw multiple sprites with same texture in one call
        // NOTE: std::vector auto-bridges to Swift RandomAccessCollection
        static void enqueueDrawSpriteBatch(const std::vector<SpriteBatchData>& sprites);
        
        // Pixel-perfect parallax rendering functions
        static void enqueueDrawParallaxSprite(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float sourceX, float sourceY, float sourceWidth, float sourceHeight);
        static void enqueueDrawBackgroundSprite(uint32_t textureHandle, int pixelX, int pixelY, int pixelWidth, int pixelHeight);
        static void enqueueDrawText(const std::string& text, float x, float y, float fontSize, float r, float g, float b, float a);
        static void enqueueDrawTextCentered(const std::string& text, float x, float y, float fontSize, float r, float g, float b, float a);
        static void enqueueDrawTextOutlined(const std::string& text, float x, float y, float fontSize,
                                            float textR, float textG, float textB, float textA,
                                            float outlineR, float outlineG, float outlineB, float outlineA,
                                            float outlineWidth);
        static void enqueueDrawTextCenteredOutlined(const std::string& text, float x, float y, float fontSize,
                                                    float textR, float textG, float textB, float textA,
                                                    float outlineR, float outlineG, float outlineB, float outlineA,
                                                    float outlineWidth);
        static void enqueueDrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a);
        static void enqueueDrawCircle(float x, float y, float radius, float r, float g, float b, float a);
        static void enqueueDrawFilledCircle(float x, float y, float radius, float r, float g, float b, float a);
        static void enqueueGetScreenSize(float* width, float* height);
        static void enqueueGetScreenInfo(ScreenInfo* screenInfo);
        static void enqueueGetTextureMetadata(const char* textureId, TextureMetadata* metadata);
        static void enqueueLockOrientation();
        static void enqueueUnlockOrientation();
        static void enqueueLockToPortrait();
        static void enqueueLockToLandscape();
        
        // Audio commands
        static void enqueuePlayMusic(const char* musicName, float volume, int loopCount);
        static void enqueueStopMusic();
        static void enqueuePlaySound(const char* soundName, float volume);
        static void enqueueStopSound(const char* soundName);
        static void enqueueSetMusicVolume(float volume);
        static void enqueueSetSoundVolume(float volume);
        
        // Logging commands
        static void enqueueLogTrace(const char* message, const char* category = "C++");
        static void enqueueLogDebug(const char* message, const char* category = "C++");
        static void enqueueLogInfo(const char* message, const char* category = "C++");
        static void enqueueLogWarn(const char* message, const char* category = "C++");
        static void enqueueLogError(const char* message, const char* category = "C++");
        static void enqueueLogFatal(const char* message, const char* category = "C++");
        
        // Haptic feedback commands
        static void enqueueHapticImpact(HapticStyle style, float intensity = 1.0f);
        static void enqueueHapticSelection();
        static void enqueueHapticNotification(HapticNotificationType type);
        static void enqueueHapticPattern(const char* patternName);
        static void enqueueHapticPrepare(HapticStyle style);
        
        // Save/Load commands
        static void enqueueSaveGame(const std::string& jsonData);
        static void enqueueLoadGame();
        static void enqueueSaveSettings(float masterVol, float musicVol, float sfxVol, bool debug);
        static void enqueueLoadSettings();
        
        // Game Center commands
        static void enqueueGameCenterAuthenticate();
        static bool isGameCenterAuthenticated();
        static void enqueueGameCenterSubmitScore(const char* leaderboardID, int64_t score);
        static void enqueueGameCenterShowLeaderboard(const char* leaderboardID);
        static void enqueueGameCenterShowAllLeaderboards();
        static const char* getGameCenterPlayerName();
        static const char* getGameCenterPlayerID();
        
        // Asset loading commands - modern callback signatures with userData
        static void enqueueLoadTexture(const std::string& path, void (*callback)(TextureData* texture, const char* error, void* userData), void* userData);
        static void enqueueLoadAudio(const std::string& path, void (*callback)(void* audioData, size_t size, const char* error, void* userData), void* userData);
        static void enqueueLoadFont(const std::string& path, int size, void (*callback)(void* fontData, const char* error, void* userData), void* userData);
        static void enqueueLoadData(const std::string& path, void (*callback)(void* data, size_t size, const char* error, void* userData), void* userData);
        
        // Asset cache management commands
        static void enqueuePreloadEssentialAssets();
        static bool enqueueIsCached(const char* assetName, int assetType);
        
        // Input delegate implementations
        static void getPrimaryInputPosition(float* x, float* y);
        static bool isPrimaryInputDown();
        static bool isPrimaryInputJustPressed();
        static bool isPrimaryInputJustReleased();
        
        // Touch input delegate implementations
        static int getTouchCount();
        static void getTouchPosition(int touchIndex, float* x, float* y);
        static bool isTouchDown();
        static bool isTouchJustPressed();
        static bool isTouchJustReleased();
        
        // Input buffer management
        static void clearInputBuffer();
        
        // Gesture detection methods
        static bool isSwipeLeftDetected();
        static bool isSwipeRightDetected();
        static bool isSwipeUpDetected();
        static bool isSwipeDownDetected();
        static void resetGestureState();
        
        // Setup function to configure delegates to use this proxy
        void setupDelegates(PlatformDelegates& delegates);
        
        // NEW: Static delegate functions for enhanced screen and texture info
        static void getScreenInfoDelegate(ScreenInfo* info);
        static bool getTextureMetadataDelegate(const char* textureId, TextureMetadata* metadata);
        
        // Swift components are managed entirely on the Swift side
        
        // Command retrieval (for Swift) - returns vectors that bridge to Swift Arrays
        std::vector<GameCore::RenderCommand> getAndClearRenderCommands();
        std::vector<GameCore::AudioCommand> getAndClearAudioCommands();
        std::vector<GameCore::LogCommand> getAndClearLogCommands();
        std::vector<GameCore::AssetCommand> getAndClearAssetCommands();
        std::vector<GameCore::HapticCommand> getAndClearHapticCommands();
        std::vector<GameCore::SaveCommand> getAndClearSaveCommands();
        std::vector<GameCore::GameCenterCommand> getAndClearGameCenterCommands();
        
        // Queue management - Thread-safe
        size_t getCommandCount() const;
        void clearQueue();
        
        // Global function for Swift to update touch state
        static void updateTouchStateGlobal(float x, float y, bool isDown, bool justPressed, bool justReleased);
        
        // Touch state management
        void updateTouchState(float x, float y, bool isDown, bool justPressed, bool justReleased);
        
        // Gesture state management
        void updateGestureState(bool swipeLeft, bool swipeRight, bool swipeUp, bool swipeDown);
        void resetInputFrameState();
        
    private:
        std::vector<RenderCommand> m_renderCommandQueue;
        std::vector<AudioCommand> m_audioCommandQueue;
        std::vector<LogCommand> m_logCommandQueue;
        std::vector<AssetCommand> m_assetCommandQueue;
        std::vector<HapticCommand> m_hapticCommandQueue;
        std::vector<SaveCommand> m_saveCommandQueue;
        std::vector<GameCenterCommand> m_gameCenterCommandQueue;
        mutable std::mutex m_queueMutex;  // mutable for const methods
        
        // Touch input state
        float m_lastTouchX = 0.0f;
        float m_lastTouchY = 0.0f;
        bool m_isTouchDown = false;
        bool m_isTouchJustPressed = false;
        bool m_isTouchJustReleased = false;
        std::vector<GameCore::TouchData> m_touchData;
        
        // Gesture input state
        bool m_isSwipeLeftDetected = false;
        bool m_isSwipeRightDetected = false;
        bool m_isSwipeUpDetected = false;
        bool m_isSwipeDownDetected = false;
        
        // Swift components are managed entirely on the Swift side
        // No C++ references needed
        
        // Static instance for delegate callbacks
        static ThreadingProxy* s_instance;
        
        // Helper to enqueue commands
        void enqueueRenderCommand(const RenderCommand& command);
        void enqueueAudioCommand(const AudioCommand& command);
        void enqueueLogCommand(const LogCommand& command);
        void enqueueAssetCommand(const AssetCommand& command);
        void enqueueHapticCommand(const HapticCommand& command);
        void enqueueSaveCommand(const SaveCommand& command);
        void enqueueGameCenterCommand(const GameCenterCommand& command);
    };
    
    // Global instance accessor for C++ interop
    extern ThreadingProxy* g_threadingProxy;
    
    // Swift-accessible functions for threading system management
    void initializeThreadingSystem();
    void shutdownThreadingSystem();
    ThreadingProxy* getThreadingProxy();
    
    // Helper function for Swift to get commands without dealing with C++ method calls
    std::vector<RenderCommand> getAndClearRenderCommandsFromProxy();
    std::vector<AudioCommand> getAndClearAudioCommandsFromProxy();
    std::vector<LogCommand> getAndClearLogCommandsFromProxy();
    std::vector<AssetCommand> getAndClearAssetCommandsFromProxy();
    std::vector<HapticCommand> getAndClearHapticCommandsFromProxy();
    std::vector<SaveCommand> getAndClearSaveCommandsFromProxy();
    std::vector<GameCenterCommand> getAndClearGameCenterCommandsFromProxy();
    
    bool isAssetCachedFromProxy(const char* assetName, int assetType);

    // Touch input state management - global functions for Swift
    void updateTouchState(float x, float y, bool isDown, bool justPressed, bool justReleased);
    void updateGestureState(bool swipeLeft, bool swipeRight, bool swipeUp, bool swipeDown);
    void resetInputFrameState();
    
    // Swift CXX Interop Function Declarations
    // Modern Swift functions that can be called directly from C++
    void setScreenInfoDirect(const ScreenInfo& screenInfo);
    
    // Save/Load synchronous interop
    std::string loadGameDataSync();
}