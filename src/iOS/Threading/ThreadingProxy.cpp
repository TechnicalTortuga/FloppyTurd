#include "ThreadingProxy.h"
#include "../../Engine/Core/GNLog.h"
#include <cstring>

namespace GameCore {
    
    // Static instance for delegate callbacks
    ThreadingProxy* ThreadingProxy::s_instance = nullptr;
    
    // Global instance for C++ interop
    ThreadingProxy* g_threadingProxy = nullptr;

    ThreadingProxy::ThreadingProxy() {
        s_instance = this;
        g_threadingProxy = this;
        GN_LOG_INFO("ThreadingProxy initialized");
    }

    ThreadingProxy::~ThreadingProxy() {
        clearQueue();
        if (s_instance == this) {
            s_instance = nullptr;
        }
        if (g_threadingProxy == this) {
            g_threadingProxy = nullptr;
        }
        GN_LOG_INFO("ThreadingProxy shutdown");
    }
    
    // Command queue management
    bool ThreadingProxy::hasCommands() {
        if (!s_instance) return false;
        std::lock_guard<std::mutex> lock(s_instance->m_queueMutex);
        return !s_instance->m_renderCommandQueue.empty() || 
               !s_instance->m_audioCommandQueue.empty() || 
               !s_instance->m_logCommandQueue.empty() ||
               !s_instance->m_assetCommandQueue.empty();
    }
    
    void ThreadingProxy::clearCommands() {
        if (!s_instance) return;
        std::lock_guard<std::mutex> lock(s_instance->m_queueMutex);
        s_instance->m_renderCommandQueue.clear();
        s_instance->m_audioCommandQueue.clear();
        s_instance->m_logCommandQueue.clear();
        s_instance->m_assetCommandQueue.clear();
    }
    
    // Helper enqueue functions
    void ThreadingProxy::enqueueRenderCommand(const RenderCommand& command) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_renderCommandQueue.push_back(command);
    }
    
    void ThreadingProxy::enqueueAudioCommand(const AudioCommand& command) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_audioCommandQueue.push_back(command);
    }
    
    void ThreadingProxy::enqueueLogCommand(const LogCommand& command) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_logCommandQueue.push_back(command);
    }
    
    void ThreadingProxy::enqueueAssetCommand(const AssetCommand& command) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_assetCommandQueue.push_back(command);
    }
    
    // Rendering command implementations
    void ThreadingProxy::enqueueBeginFrame() {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_BEGIN_FRAME);
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueEndFrame() {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_END_FRAME);
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueuePresent() {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_PRESENT);
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueClearScreen(float r, float g, float b, float a) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_CLEAR_SCREEN);
        cmd.data.r = r;
        cmd.data.g = g;
        cmd.data.b = b;
        cmd.data.a = a;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueDrawSprite(uint32_t textureHandle, float x, float y, float rotation) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_SPRITE);
        cmd.data.textureHandle = textureHandle;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.rotation = rotation;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueDrawSpriteScaled(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_SPRITE_SCALED);
        cmd.data.textureHandle = textureHandle;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.scaleX = scaleX;
        cmd.data.scaleY = scaleY;
        cmd.data.rotation = rotation;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueDrawSpriteScaledWithSource(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float sourceX, float sourceY, float sourceWidth, float sourceHeight) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_SPRITE_SCALED_WITH_SOURCE);
        cmd.data.textureHandle = textureHandle;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.scaleX = scaleX;
        cmd.data.scaleY = scaleY;
        cmd.data.rotation = rotation;
        cmd.data.sourceX = sourceX;
        cmd.data.sourceY = sourceY;
        cmd.data.sourceWidth = sourceWidth;
        cmd.data.sourceHeight = sourceHeight;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueDrawText(const std::string& text, float x, float y, float fontSize, float r, float g, float b, float a) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_TEXT);
        cmd.data.text = text;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.fontSize = fontSize;
        cmd.data.r = r;
        cmd.data.g = g;
        cmd.data.b = b;
        cmd.data.a = a;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueDrawTextCentered(const std::string& text, float x, float y, float fontSize, float r, float g, float b, float a) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_TEXT_CENTERED);
        cmd.data.text = text;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.fontSize = fontSize;
        cmd.data.r = r;
        cmd.data.g = g;
        cmd.data.b = b;
        cmd.data.a = a;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueDrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_RECTANGLE);
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.width = width;
        cmd.data.height = height;
        cmd.data.r = r;
        cmd.data.g = g;
        cmd.data.b = b;
        cmd.data.a = a;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueDrawCircle(float x, float y, float radius, float r, float g, float b, float a) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_CIRCLE);
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.radius = radius;
        cmd.data.r = r;
        cmd.data.g = g;
        cmd.data.b = b;
        cmd.data.a = a;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueGetScreenSize(float* width, float* height) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_GET_SCREEN_SIZE);
        cmd.data.screenWidth = width;
        cmd.data.screenHeight = height;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    // Audio command implementations
    void ThreadingProxy::enqueuePlayMusic(const char* musicName, float volume, int loopCount) {
        if (!s_instance) return;
        AudioCommand cmd(CommandType::CMD_PLAY_MUSIC);
        cmd.data.audioFileName = musicName;
        cmd.data.volume = volume;
        cmd.data.loopCount = loopCount;
        s_instance->enqueueAudioCommand(cmd);
    }
    
    void ThreadingProxy::enqueueStopMusic() {
        if (!s_instance) return;
        AudioCommand cmd(CommandType::CMD_STOP_MUSIC);
        s_instance->enqueueAudioCommand(cmd);
    }
    
    void ThreadingProxy::enqueuePlaySound(const char* soundName, float volume) {
        if (!s_instance) return;
        AudioCommand cmd(CommandType::CMD_PLAY_SOUND);
        cmd.data.audioFileName = soundName;
        cmd.data.volume = volume;
        s_instance->enqueueAudioCommand(cmd);
    }
    
    void ThreadingProxy::enqueueStopSound() {
        if (!s_instance) return;
        AudioCommand cmd(CommandType::CMD_STOP_SOUND);
        s_instance->enqueueAudioCommand(cmd);
    }
    
    void ThreadingProxy::enqueueSetMusicVolume(float volume) {
        if (!s_instance) return;
        AudioCommand cmd(CommandType::CMD_SET_MUSIC_VOLUME);
        cmd.data.volume = volume;
        s_instance->enqueueAudioCommand(cmd);
    }
    
    void ThreadingProxy::enqueueSetSoundVolume(float volume) {
        if (!s_instance) return;
        AudioCommand cmd(CommandType::CMD_SET_SOUND_VOLUME);
        cmd.data.volume = volume;
        s_instance->enqueueAudioCommand(cmd);
    }
    
    // Logging command implementations
    void ThreadingProxy::enqueueLogTrace(const char* message, const char* category) {
        if (!s_instance) return;
        LogCommand cmd(CommandType::CMD_LOG_TRACE);
        cmd.data.logMessage = std::string(message);
        cmd.data.logCategory = std::string(category);
        s_instance->enqueueLogCommand(cmd);
    }
    
    void ThreadingProxy::enqueueLogDebug(const char* message, const char* category) {
        if (!s_instance) return;
        LogCommand cmd(CommandType::CMD_LOG_DEBUG);
        cmd.data.logMessage = std::string(message);
        cmd.data.logCategory = std::string(category);
        s_instance->enqueueLogCommand(cmd);
    }
    
    void ThreadingProxy::enqueueLogInfo(const char* message, const char* category) {
        if (!s_instance) return;
        LogCommand cmd(CommandType::CMD_LOG_INFO);
        cmd.data.logMessage = std::string(message);
        cmd.data.logCategory = std::string(category);
        s_instance->enqueueLogCommand(cmd);
    }
    
    void ThreadingProxy::enqueueLogWarn(const char* message, const char* category) {
        if (!s_instance) return;
        LogCommand cmd(CommandType::CMD_LOG_WARN);
        cmd.data.logMessage = std::string(message);
        cmd.data.logCategory = std::string(category);
        s_instance->enqueueLogCommand(cmd);
    }
    
    void ThreadingProxy::enqueueLogError(const char* message, const char* category) {
        if (!s_instance) return;
        LogCommand cmd(CommandType::CMD_LOG_ERROR);
        cmd.data.logMessage = std::string(message);
        cmd.data.logCategory = std::string(category);
        s_instance->enqueueLogCommand(cmd);
    }
    
    void ThreadingProxy::enqueueLogFatal(const char* message, const char* category) {
        if (!s_instance) return;
        LogCommand cmd(CommandType::CMD_LOG_FATAL);
        cmd.data.logMessage = std::string(message);
        cmd.data.logCategory = std::string(category);
        s_instance->enqueueLogCommand(cmd);
    }
    
    // Asset loading enqueue functions - modern callback signatures with userData
    void ThreadingProxy::enqueueLoadTexture(const std::string& path, void (*callback)(TextureData* texture, const char* error, void* userData), void* userData) {
        if (!s_instance) return;
        AssetCommand cmd(CommandType::CMD_LOAD_TEXTURE);
        cmd.data.assetPath = path;
        cmd.data.callback = reinterpret_cast<void*>(callback);
        cmd.data.userData = userData;
        s_instance->enqueueAssetCommand(cmd);
    }
    
    void ThreadingProxy::enqueueLoadAudio(const std::string& path, void (*callback)(void* audioData, size_t size, const char* error, void* userData), void* userData) {
        if (!s_instance) return;
        AssetCommand cmd(CommandType::CMD_LOAD_AUDIO);
        cmd.data.assetPath = path;
        cmd.data.callback = reinterpret_cast<void*>(callback);
        cmd.data.userData = userData;
        s_instance->enqueueAssetCommand(cmd);
    }
    
    void ThreadingProxy::enqueueLoadFont(const std::string& path, int size, void (*callback)(void* fontData, const char* error, void* userData), void* userData) {
        if (!s_instance) return;
        AssetCommand cmd(CommandType::CMD_LOAD_FONT);
        cmd.data.assetPath = path;
        cmd.data.fontSize = size;
        cmd.data.callback = reinterpret_cast<void*>(callback);
        cmd.data.userData = userData;
        s_instance->enqueueAssetCommand(cmd);
    }
    
    void ThreadingProxy::enqueueLoadData(const std::string& path, void (*callback)(void* data, size_t size, const char* error, void* userData), void* userData) {
        if (!s_instance) return;
        AssetCommand cmd(CommandType::CMD_LOAD_DATA);
        cmd.data.assetPath = path;
        cmd.data.callback = reinterpret_cast<void*>(callback);
        cmd.data.userData = userData;
        s_instance->enqueueAssetCommand(cmd);
    }
    
    // Asset cache management enqueue functions
    void ThreadingProxy::enqueuePreloadEssentialAssets() {
        if (!s_instance) return;
        AssetCommand cmd(CommandType::CMD_PRELOAD_ESSENTIAL_ASSETS);
        s_instance->enqueueAssetCommand(cmd);
    }

    bool ThreadingProxy::enqueueIsCached(const char* assetName, int assetType) {
        if (!s_instance) return false;
        if (!assetName) return false;
        std::string name(assetName);
        if (name.empty()) return false;
        switch (assetType) {
            case 0: // texture
                return name.find("turd") != std::string::npos || 
                       name.find("background") != std::string::npos ||
                       name.find("ui") != std::string::npos;
            case 1: // audio
                return name.find("fart") != std::string::npos ||
                       name.find("music") != std::string::npos;
            case 2: // font
                return name.find("font") != std::string::npos ||
                       name.find("text") != std::string::npos;
            case 3: // data
                return name.find("config") != std::string::npos ||
                       name.find("level") != std::string::npos;
            default:
                return false;
        }
    }
    
    // Input delegate implementations
    void ThreadingProxy::getPrimaryInputPosition(float* x, float* y) {
        if (!s_instance) {
            if (x) *x = 0.0f;
            if (y) *y = 0.0f;
            return;
        }
        if (x) *x = s_instance->m_lastTouchX;
        if (y) *y = s_instance->m_lastTouchY;
    }
    
    bool ThreadingProxy::isPrimaryInputDown() {
        if (!s_instance) return false;
        bool result = s_instance->m_isTouchDown;
        if (result) {
            GN_LOG_DEBUG("ThreadingProxy: isPrimaryInputDown() = true");
        }
        return result;
    }
    
    bool ThreadingProxy::isPrimaryInputJustPressed() {
        if (!s_instance) return false;
        bool result = s_instance->m_isTouchJustPressed;
        if (result) {
            GN_LOG_INFO("🎯 ThreadingProxy: isPrimaryInputJustPressed() = TRUE - C++ should detect input!");
        }
        return result;
    }
    
    bool ThreadingProxy::isPrimaryInputJustReleased() {
        if (!s_instance) return false;
        bool result = s_instance->m_isTouchJustReleased;
        if (result) {
            GN_LOG_DEBUG("ThreadingProxy: isPrimaryInputJustReleased() = true");
        }
        return result;
    }
    
    // Touch input delegate implementations
    int ThreadingProxy::getTouchCount() {
        if (!s_instance) return 0;
        // For now, return 1 if touch is down, 0 otherwise
        // This can be expanded later for multi-touch support
        int count = s_instance->m_isTouchDown ? 1 : 0;
        if (count > 0) {
            GN_LOG_DEBUG("ThreadingProxy: getTouchCount() = %d", count);
        }
        return count;
    }
    
    void ThreadingProxy::getTouchPosition(int touchIndex, float* x, float* y) {
        if (!s_instance || touchIndex != 0) {
            if (x) *x = 0.0f;
            if (y) *y = 0.0f;
            return;
        }
        if (x) *x = s_instance->m_lastTouchX;
        if (y) *y = s_instance->m_lastTouchY;
        GN_LOG_DEBUG("ThreadingProxy: getTouchPosition(%d) = (%f, %f)", touchIndex, s_instance->m_lastTouchX, s_instance->m_lastTouchY);
    }
    
    void ThreadingProxy::clearInputBuffer() {
        if (!s_instance) return;
        
        // Clear all input state to prevent lingering touches from causing auto-shooting
        s_instance->m_isTouchDown = false;
        s_instance->m_isTouchJustPressed = false;
        s_instance->m_isTouchJustReleased = false;
        s_instance->m_lastTouchX = 0.0f;
        s_instance->m_lastTouchY = 0.0f;
        
        // Reset gesture state as well
        resetGestureState();
        
        GN_LOG_INFO("ThreadingProxy: Input buffer cleared - no more lingering touches!");
    }
    
    void ThreadingProxy::updateTouchState(float x, float y, bool isDown, bool justPressed, bool justReleased) {
        if (!s_instance) return;
        s_instance->m_lastTouchX = x;
        s_instance->m_lastTouchY = y;
        s_instance->m_isTouchDown = isDown;
        s_instance->m_isTouchJustPressed = justPressed;
        s_instance->m_isTouchJustReleased = justReleased;
        
        if (justPressed) {
            GN_LOG_INFO("🔥 ThreadingProxy: updateTouchState() - TOUCH PRESSED at (%f, %f)", x, y);
        }
    }
    
    void ThreadingProxy::resetInputFrameState() {
        if (!s_instance) return;
        s_instance->m_isTouchJustPressed = false;
        s_instance->m_isTouchJustReleased = false;
    }
    
    // Gesture detection method implementations
    bool ThreadingProxy::isSwipeLeftDetected() {
        if (!s_instance) return false;
        bool result = s_instance->m_isSwipeLeftDetected;
        if (result) {
            GN_LOG_INFO("🎯 ThreadingProxy: isSwipeLeftDetected() = TRUE - SWIPE LEFT DETECTED!");
        }
        return result;
    }
    
    bool ThreadingProxy::isSwipeRightDetected() {
        if (!s_instance) return false;
        bool result = s_instance->m_isSwipeRightDetected;
        if (result) {
            GN_LOG_INFO("🎯 ThreadingProxy: isSwipeRightDetected() = TRUE - SWIPE RIGHT DETECTED!");
        }
        return result;
    }
    
    bool ThreadingProxy::isSwipeUpDetected() {
        if (!s_instance) return false;
        bool result = s_instance->m_isSwipeUpDetected;
        if (result) {
            GN_LOG_INFO("🎯 ThreadingProxy: isSwipeUpDetected() = TRUE - SWIPE UP DETECTED!");
        }
        return result;
    }
    
    bool ThreadingProxy::isSwipeDownDetected() {
        if (!s_instance) return false;
        bool result = s_instance->m_isSwipeDownDetected;
        if (result) {
            GN_LOG_INFO("🎯 ThreadingProxy: isSwipeDownDetected() = TRUE - SWIPE DOWN DETECTED!");
        }
        return result;
    }
    
    void ThreadingProxy::resetGestureState() {
        if (!s_instance) return;
        s_instance->m_isSwipeLeftDetected = false;
        s_instance->m_isSwipeRightDetected = false;
        s_instance->m_isSwipeUpDetected = false;
        s_instance->m_isSwipeDownDetected = false;
    }
    
    void ThreadingProxy::updateGestureState(bool swipeLeft, bool swipeRight, bool swipeUp, bool swipeDown) {
        if (!s_instance) return;
        s_instance->m_isSwipeLeftDetected = swipeLeft;
        s_instance->m_isSwipeRightDetected = swipeRight;
        s_instance->m_isSwipeUpDetected = swipeUp;
        s_instance->m_isSwipeDownDetected = swipeDown;
        
        if (swipeLeft || swipeRight || swipeUp || swipeDown) {
            GN_LOG_INFO("🔥 ThreadingProxy: updateGestureState() - GESTURE DETECTED: Left=%d, Right=%d, Up=%d, Down=%d", 
                       swipeLeft, swipeRight, swipeUp, swipeDown);
        }
    }
    
    // setSwiftComponents removed - Swift components managed entirely on Swift side
    
    std::vector<RenderCommand> ThreadingProxy::getAndClearRenderCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<RenderCommand> commands = m_renderCommandQueue;
        m_renderCommandQueue.clear();
        return commands;  // Bridges to Array<RenderCommand> in Swift
    }
    
    std::vector<AudioCommand> ThreadingProxy::getAndClearAudioCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<AudioCommand> commands = m_audioCommandQueue;
        m_audioCommandQueue.clear();
        return commands;  // Bridges to Array<AudioCommand> in Swift
    }
    
    std::vector<LogCommand> ThreadingProxy::getAndClearLogCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<LogCommand> commands = m_logCommandQueue;
        m_logCommandQueue.clear();
        return commands;  // Bridges to Array<LogCommand> in Swift
    }
    
    std::vector<AssetCommand> ThreadingProxy::getAndClearAssetCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<AssetCommand> commands = m_assetCommandQueue;
        m_assetCommandQueue.clear();
        return commands;  // Bridges to Array<AssetCommand> in Swift
    }
    
    size_t ThreadingProxy::getCommandCount() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_renderCommandQueue.size() + m_audioCommandQueue.size() + m_logCommandQueue.size() + m_assetCommandQueue.size();
    }
    
    void ThreadingProxy::clearQueue() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_renderCommandQueue.clear();
        m_audioCommandQueue.clear();
        m_logCommandQueue.clear();
        m_assetCommandQueue.clear();
    }

    void ThreadingProxy::setupDelegates(PlatformDelegates& delegates) {
        GN_LOG_INFO("ThreadingProxy: Setting up delegates to use command queue - turds will fly smoothly!");
        
        // Configure renderer delegates to use our enqueue functions
        delegates.renderer.beginFrame = enqueueBeginFrame;
        delegates.renderer.endFrame = enqueueEndFrame;
        delegates.renderer.present = enqueuePresent;
        delegates.renderer.clearScreen = enqueueClearScreen;
        delegates.renderer.drawSprite = enqueueDrawSprite;
        delegates.renderer.drawSpriteScaled = enqueueDrawSpriteScaled;
        delegates.renderer.drawSpriteScaledWithSource = enqueueDrawSpriteScaledWithSource;
        delegates.renderer.drawText = enqueueDrawText;
        delegates.renderer.drawTextCentered = enqueueDrawTextCentered;
        delegates.renderer.drawRectangle = enqueueDrawRectangle;
        delegates.renderer.drawCircle = enqueueDrawCircle;
        delegates.renderer.getScreenSize = enqueueGetScreenSize;
        
        // Configure audio delegates to use our enqueue functions
        delegates.audio.playMusic = enqueuePlayMusic;
        delegates.audio.stopMusic = enqueueStopMusic;
        delegates.audio.playSound = enqueuePlaySound;
        delegates.audio.stopSound = enqueueStopSound;
        delegates.audio.setMusicVolume = enqueueSetMusicVolume;
        delegates.audio.setSFXVolume = enqueueSetSoundVolume;
        
        // Configure logging delegates to use our enqueue functions
        delegates.log.logTrace = enqueueLogTrace;
        delegates.log.logDebug = enqueueLogDebug;
        delegates.log.logInfo = enqueueLogInfo;
        delegates.log.logWarn = enqueueLogWarn;
        delegates.log.logError = enqueueLogError;
        delegates.log.logFatal = enqueueLogFatal;
        
        // Configure asset loading delegates to use our enqueue functions
        delegates.asset.loadTexture = enqueueLoadTexture;
        delegates.asset.loadAudio = enqueueLoadAudio;
        delegates.asset.loadFont = enqueueLoadFont;
        delegates.asset.loadData = enqueueLoadData;

        // Configure asset cache management delegates
        delegates.asset.preloadEssentialAssets = enqueuePreloadEssentialAssets;
        delegates.asset.isCached = enqueueIsCached;
        
        // Configure input delegates to handle touch input
        delegates.input.getPrimaryInputPosition = getPrimaryInputPosition;
        delegates.input.isPrimaryInputDown = isPrimaryInputDown;
        delegates.input.isPrimaryInputJustPressed = isPrimaryInputJustPressed;
        delegates.input.isPrimaryInputJustReleased = isPrimaryInputJustReleased;
        
        // Configure touch input delegates
        delegates.input.getTouchCount = getTouchCount;
        delegates.input.getTouchPosition = getTouchPosition;
        
        // Configure gesture detection delegates
        delegates.input.isSwipeLeftDetected = isSwipeLeftDetected;
        delegates.input.isSwipeRightDetected = isSwipeRightDetected;
        delegates.input.isSwipeUpDetected = isSwipeUpDetected;
        delegates.input.isSwipeDownDetected = isSwipeDownDetected;
        delegates.input.resetGestureState = resetGestureState;
        
        // Configure input buffer management
        delegates.input.clearInputBuffer = clearInputBuffer;
        
        GN_LOG_INFO("ThreadingProxy: Input delegates configured - touch input will flow from iOS->ThreadingProxy->C++");
        GN_LOG_INFO("ThreadingProxy: Delegates configured successfully - ready for turd-tossing action!");
    }

// Swift interop functions - these will be available as FloppyTurd.initializeThreadingSystem()
void initializeThreadingSystem() {
    if (!g_threadingProxy) {
        g_threadingProxy = new ThreadingProxy();
        GN_LOG_INFO("ThreadingProxy: Threading system initialized from Swift!");
    }
}

void shutdownThreadingSystem() {
    if (g_threadingProxy) {
        delete g_threadingProxy;
        g_threadingProxy = nullptr;
        GN_LOG_INFO("ThreadingProxy: Threading system shutdown from Swift!");
    }
}

std::vector<RenderCommand> getAndClearRenderCommandsFromProxy() {
    if (g_threadingProxy) {
        return g_threadingProxy->getAndClearRenderCommands();
    }
    return std::vector<RenderCommand>();
}

std::vector<AudioCommand> getAndClearAudioCommandsFromProxy() {
    if (g_threadingProxy) {
        return g_threadingProxy->getAndClearAudioCommands();
    }
    return std::vector<AudioCommand>();
}

std::vector<LogCommand> getAndClearLogCommandsFromProxy() {
    if (g_threadingProxy) {
        return g_threadingProxy->getAndClearLogCommands();
    }
    return std::vector<LogCommand>();
}

std::vector<AssetCommand> getAndClearAssetCommandsFromProxy() {
    if (g_threadingProxy) {
        return g_threadingProxy->getAndClearAssetCommands();
    }
    return std::vector<AssetCommand>();
}

bool isAssetCachedFromProxy(const char* assetName, int assetType) {
    if (g_threadingProxy) {
        return g_threadingProxy->enqueueIsCached(assetName, assetType);
    }
    return false;
}

void updateTouchState(float x, float y, bool isDown, bool justPressed, bool justReleased) {
    if (g_threadingProxy) {
        g_threadingProxy->updateTouchState(x, y, isDown, justPressed, justReleased);
    }
}

void updateGestureState(bool swipeLeft, bool swipeRight, bool swipeUp, bool swipeDown) {
    if (g_threadingProxy) {
        g_threadingProxy->updateGestureState(swipeLeft, swipeRight, swipeUp, swipeDown);
    }
}

void resetInputFrameState() {
    if (g_threadingProxy) {
        g_threadingProxy->resetInputFrameState();
    }
}

} // namespace GameCore// Logging interface implementation for GNLog.h
namespace GameCore {
    void LogToThreadingProxy(const char* message, const char* category, int level) {
        switch (level) {
            case 0: ThreadingProxy::enqueueLogTrace(message, category); break;
            case 1: ThreadingProxy::enqueueLogDebug(message, category); break;
            case 2: ThreadingProxy::enqueueLogInfo(message, category); break;
            case 3: ThreadingProxy::enqueueLogWarn(message, category); break;
            case 4: ThreadingProxy::enqueueLogError(message, category); break;
            case 5: ThreadingProxy::enqueueLogFatal(message, category); break;
        }
    }
}

