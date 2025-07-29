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
    
    void ThreadingProxy::enqueueDrawText(const char* text, float x, float y, float fontSize, float r, float g, float b, float a) {
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
        delegates.renderer.drawText = enqueueDrawText;
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

