#include "ThreadingProxy.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Configuration/ConfigManager.h"
#include "../../FloppyTurd/Input/InputManager.h"
#include "../../FloppyTurd/Game/FloppyTurdGame.h"
#include <cstring>

// Import Swift module for direct interop calls
#include "FloppyTurd-Swift.h"

namespace GameCore {
    
    // Static instance for delegate callbacks
    ThreadingProxy* ThreadingProxy::s_instance = nullptr;
    
    // Global instance for C++ interop
    ThreadingProxy* g_threadingProxy = nullptr;
    
    // Static ad ready state - updated by Swift AdManager
    static std::atomic<bool> s_adReadyState(false);
    
    // Static Game Center authentication state - updated by Swift GameCenterManager
    static std::atomic<bool> s_gameCenterAuthState(false);
    static std::string s_gameCenterPlayerName = "";
    static std::string s_gameCenterPlayerID = "";

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
               !s_instance->m_assetCommandQueue.empty() ||
               !s_instance->m_gameCenterCommandQueue.empty();
    }
    
    void ThreadingProxy::clearCommands() {
        if (!s_instance) return;
        std::lock_guard<std::mutex> lock(s_instance->m_queueMutex);
        s_instance->m_renderCommandQueue.clear();
        s_instance->m_audioCommandQueue.clear();
        s_instance->m_logCommandQueue.clear();
        s_instance->m_assetCommandQueue.clear();
        s_instance->m_gameCenterCommandQueue.clear();
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
    
    void ThreadingProxy::enqueueHapticCommand(const HapticCommand& command) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_hapticCommandQueue.push_back(command);
    }
    
    void ThreadingProxy::enqueueSaveCommand(const SaveCommand& command) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_saveCommandQueue.push_back(command);
    }
    
    void ThreadingProxy::enqueueGameCenterCommand(const GameCenterCommand& command) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_gameCenterCommandQueue.push_back(command);
    }
    
    void ThreadingProxy::enqueueAdCommand(const AdCommand& command) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_adCommandQueue.push_back(command);
    }
    
    void ThreadingProxy::enqueueIAPCommand(const IAPCommand& command) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_iapCommandQueue.push_back(command);
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
    
    void ThreadingProxy::enqueueDrawSpriteScaledCentered(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_SPRITE_SCALED_CENTERED);
        cmd.data.textureHandle = textureHandle;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.scaleX = scaleX;
        cmd.data.scaleY = scaleY;
        cmd.data.rotation = rotation;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueDrawSpriteScaledPivoted(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float pivotX, float pivotY) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_SPRITE_SCALED_PIVOTED);
        cmd.data.textureHandle = textureHandle;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.scaleX = scaleX;
        cmd.data.scaleY = scaleY;
        cmd.data.rotation = rotation;
        cmd.data.pivotX = pivotX;
        cmd.data.pivotY = pivotY;
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
    
    void ThreadingProxy::enqueueDrawSpriteScaledWithSourceCentered(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float sourceX, float sourceY, float sourceWidth, float sourceHeight) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_SPRITE_SCALED_WITH_SOURCE_CENTERED);
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
    
    void ThreadingProxy::enqueueDrawSpriteScaledWithSourcePivoted(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float pivotX, float pivotY, float sourceX, float sourceY, float sourceWidth, float sourceHeight) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_SPRITE_SCALED_WITH_SOURCE_PIVOTED);
        cmd.data.textureHandle = textureHandle;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.scaleX = scaleX;
        cmd.data.scaleY = scaleY;
        cmd.data.rotation = rotation;
        cmd.data.pivotX = pivotX;
        cmd.data.pivotY = pivotY;
        cmd.data.sourceX = sourceX;
        cmd.data.sourceY = sourceY;
        cmd.data.sourceWidth = sourceWidth;
        cmd.data.sourceHeight = sourceHeight;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueDrawSpriteBatch(const std::vector<SpriteBatchData>& sprites) {
        if (!s_instance) return;
        if (sprites.empty()) return;  // Don't enqueue empty batches
        
        RenderCommand cmd(CommandType::CMD_DRAW_SPRITE_BATCH);
        cmd.data.batchData = sprites;  // Copy the vector
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueDrawParallaxSprite(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float sourceX, float sourceY, float sourceWidth, float sourceHeight) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_PARALLAX_SPRITE);
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
    
    void ThreadingProxy::enqueueDrawBackgroundSprite(uint32_t textureHandle, int pixelX, int pixelY, int pixelWidth, int pixelHeight) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_BACKGROUND_SPRITE);
        cmd.data.textureHandle = textureHandle;
        cmd.data.x = static_cast<float>(pixelX);
        cmd.data.y = static_cast<float>(pixelY);
        cmd.data.width = static_cast<float>(pixelWidth);
        cmd.data.height = static_cast<float>(pixelHeight);
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

    void ThreadingProxy::enqueueDrawTextOutlined(const std::string& text, float x, float y, float fontSize,
                                                 float textR, float textG, float textB, float textA,
                                                 float outlineR, float outlineG, float outlineB, float outlineA,
                                                 float outlineWidth) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_TEXT_OUTLINED);
        cmd.data.text = text;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.fontSize = fontSize;
        cmd.data.r = textR;
        cmd.data.g = textG;
        cmd.data.b = textB;
        cmd.data.a = textA;
        cmd.data.outlineR = outlineR;
        cmd.data.outlineG = outlineG;
        cmd.data.outlineB = outlineB;
        cmd.data.outlineA = outlineA;
        cmd.data.outlineWidth = outlineWidth;
        s_instance->enqueueRenderCommand(cmd);
    }

    void ThreadingProxy::enqueueDrawTextCenteredOutlined(const std::string& text, float x, float y, float fontSize,
                                                         float textR, float textG, float textB, float textA,
                                                         float outlineR, float outlineG, float outlineB, float outlineA,
                                                         float outlineWidth) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_TEXT_CENTERED_OUTLINED);
        cmd.data.text = text;
        cmd.data.x = x;
        cmd.data.y = y;
        cmd.data.fontSize = fontSize;
        cmd.data.r = textR;
        cmd.data.g = textG;
        cmd.data.b = textB;
        cmd.data.a = textA;
        cmd.data.outlineR = outlineR;
        cmd.data.outlineG = outlineG;
        cmd.data.outlineB = outlineB;
        cmd.data.outlineA = outlineA;
        cmd.data.outlineWidth = outlineWidth;
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
    
    void ThreadingProxy::enqueueDrawFilledCircle(float x, float y, float radius, float r, float g, float b, float a) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_DRAW_FILLED_CIRCLE);
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
    
    void ThreadingProxy::enqueueGetScreenInfo(ScreenInfo* screenInfo) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_GET_SCREEN_INFO);
        cmd.data.screenInfo = screenInfo;
        s_instance->enqueueRenderCommand(cmd);
    }
    
    void ThreadingProxy::enqueueGetTextureMetadata(const char* textureId, TextureMetadata* metadata) {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_GET_TEXTURE_METADATA);
        cmd.data.textureId = textureId;
        // Copy the metadata into the command's owned object
        cmd.data.textureMetadata = *metadata;
        s_instance->enqueueRenderCommand(cmd);
    }

    void ThreadingProxy::enqueueLockOrientation() {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_LOCK_ORIENTATION);
        s_instance->enqueueRenderCommand(cmd);
    }

    void ThreadingProxy::enqueueUnlockOrientation() {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_UNLOCK_ORIENTATION);
        s_instance->enqueueRenderCommand(cmd);
    }

    void ThreadingProxy::enqueueLockToPortrait() {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_LOCK_TO_PORTRAIT);
        s_instance->enqueueRenderCommand(cmd);
    }

    void ThreadingProxy::enqueueLockToLandscape() {
        if (!s_instance) return;
        RenderCommand cmd(CommandType::CMD_LOCK_TO_LANDSCAPE);
        s_instance->enqueueRenderCommand(cmd);
    }
    
    // Audio command implementations
    void ThreadingProxy::enqueuePlayMusic(const char* musicName, float volume, int loopCount) {
        if (!s_instance) return;
        
        // Track music state in game before sending command
        extern FloppyTurdGame* g_Game;
        if (g_Game && musicName) {
            g_Game->SetCurrentMusicTrack(std::string(musicName));
        }
        
        AudioCommand cmd(CommandType::CMD_PLAY_MUSIC);
        cmd.data.audioFileName = musicName;
        cmd.data.volume = volume;
        cmd.data.loopCount = loopCount;
        s_instance->enqueueAudioCommand(cmd);
    }
    
    void ThreadingProxy::enqueueStopMusic() {
        if (!s_instance) return;
        
        // Clear music state in game before sending command
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            g_Game->SetCurrentMusicTrack("");
        }
        
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
    
    void ThreadingProxy::enqueueStopSound(const char* soundName) {
        if (!s_instance) return;
        AudioCommand cmd(CommandType::CMD_STOP_SOUND);
        cmd.data.audioFileName = std::string(soundName);
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
    
    // Haptic feedback command implementations
    void ThreadingProxy::enqueueHapticImpact(HapticStyle style, float intensity) {
        if (!s_instance) return;
        HapticCommand cmd(CommandType::CMD_HAPTIC_IMPACT);
        cmd.data.style = style;
        cmd.data.intensity = intensity;
        s_instance->enqueueHapticCommand(cmd);
    }
    
    void ThreadingProxy::enqueueHapticSelection() {
        if (!s_instance) return;
        HapticCommand cmd(CommandType::CMD_HAPTIC_SELECTION);
        s_instance->enqueueHapticCommand(cmd);
    }
    
    void ThreadingProxy::enqueueHapticNotification(HapticNotificationType type) {
        if (!s_instance) return;
        HapticCommand cmd(CommandType::CMD_HAPTIC_NOTIFICATION);
        cmd.data.notificationType = type;
        s_instance->enqueueHapticCommand(cmd);
    }
    
    void ThreadingProxy::enqueueHapticPattern(const char* patternName) {
        if (!s_instance) return;
        HapticCommand cmd(CommandType::CMD_HAPTIC_PATTERN);
        cmd.data.patternName = std::string(patternName);
        s_instance->enqueueHapticCommand(cmd);
    }
    
    void ThreadingProxy::enqueueHapticPrepare(HapticStyle style) {
        if (!s_instance) return;
        HapticCommand cmd(CommandType::CMD_HAPTIC_PREPARE);
        cmd.data.style = style;
        s_instance->enqueueHapticCommand(cmd);
    }
    
    // Save/Load command implementations
    void ThreadingProxy::enqueueSaveGame(const std::string& jsonData) {
        if (!s_instance) return;
        SaveCommand cmd(CommandType::CMD_SAVE_GAME);
        cmd.data.jsonData = jsonData;
        s_instance->enqueueSaveCommand(cmd);
    }
    
    void ThreadingProxy::enqueueLoadGame() {
        if (!s_instance) return;
        SaveCommand cmd(CommandType::CMD_LOAD_GAME);
        s_instance->enqueueSaveCommand(cmd);
    }
    
    void ThreadingProxy::enqueueSaveSettings() {
        if (!s_instance) return;
        SaveCommand cmd(CommandType::CMD_SAVE_SETTINGS);
        // Settings are read from GameSettings/UserDefaults on the Swift side
        // No need to pass individual parameters
        s_instance->enqueueSaveCommand(cmd);
    }
    
    void ThreadingProxy::enqueueLoadSettings() {
        if (!s_instance) return;
        SaveCommand cmd(CommandType::CMD_LOAD_SETTINGS);
        s_instance->enqueueSaveCommand(cmd);
    }
    
    // Game Center command implementations
    void ThreadingProxy::enqueueGameCenterAuthenticate() {
        if (!s_instance) return;
        GameCenterCommand cmd(CommandType::CMD_GAME_CENTER_AUTHENTICATE);
        s_instance->enqueueGameCenterCommand(cmd);
    }
    
    bool ThreadingProxy::isGameCenterAuthenticated() {
        // Return the Game Center authentication state that Swift GameCenterManager updates
        return s_gameCenterAuthState.load();
    }
    
    void ThreadingProxy::enqueueGameCenterSubmitScore(const char* leaderboardID, int64_t score) {
        if (!s_instance) return;
        GameCenterCommand cmd(CommandType::CMD_GAME_CENTER_SUBMIT_SCORE);
        cmd.data.leaderboardID = leaderboardID ? leaderboardID : "";
        cmd.data.score = score;
        s_instance->enqueueGameCenterCommand(cmd);
    }
    
    void ThreadingProxy::enqueueGameCenterShowLeaderboard(const char* leaderboardID) {
        if (!s_instance) return;
        GameCenterCommand cmd(CommandType::CMD_GAME_CENTER_SHOW_LEADERBOARD);
        cmd.data.leaderboardID = leaderboardID ? leaderboardID : "";
        s_instance->enqueueGameCenterCommand(cmd);
    }
    
    void ThreadingProxy::enqueueGameCenterShowAllLeaderboards() {
        if (!s_instance) return;
        GameCenterCommand cmd(CommandType::CMD_GAME_CENTER_SHOW_ALL_LEADERBOARDS);
        s_instance->enqueueGameCenterCommand(cmd);
    }
    
    void ThreadingProxy::enqueueGameCenterLoadLeaderboardEntries(const char* leaderboardID,
                                                                   void (*completion)(const LeaderboardEntry*, int, bool)) {
        if (!s_instance) return;
        GameCenterCommand cmd(CommandType::CMD_GAME_CENTER_LOAD_LEADERBOARD_ENTRIES);
        cmd.data.leaderboardID = leaderboardID ? leaderboardID : "";
        cmd.data.leaderboardEntriesCallback = reinterpret_cast<void*>(completion); // Cast to void* for Swift
        s_instance->enqueueGameCenterCommand(cmd);
    }
    
    void ThreadingProxy::enqueueGameCenterLoadLocalPlayerEntry(const char* leaderboardID,
                                                                 void (*completion)(int, int64_t, bool)) {
        if (!s_instance) return;
        GameCenterCommand cmd(CommandType::CMD_GAME_CENTER_LOAD_LOCAL_PLAYER_ENTRY);
        cmd.data.leaderboardID = leaderboardID ? leaderboardID : "";
        cmd.data.localPlayerEntryCallback = reinterpret_cast<void*>(completion); // Cast to void* for Swift
        s_instance->enqueueGameCenterCommand(cmd);
    }
    
    
    // Ad command implementations
    void ThreadingProxy::enqueueAdPreload() {
        if (!s_instance) return;
        AdCommand cmd(CommandType::CMD_AD_PRELOAD);
        s_instance->enqueueAdCommand(cmd);
    }
    
    void ThreadingProxy::enqueueAdShow() {
        if (!s_instance) return;
        AdCommand cmd(CommandType::CMD_AD_SHOW);
        s_instance->enqueueAdCommand(cmd);
    }
    
    bool ThreadingProxy::isAdReady() {
        // Return the ad ready state that Swift AdManager updates
        return s_adReadyState.load();
    }
    
    // Function for Swift to update the ad ready state
    void setAdReadyState(bool isReady) {
        s_adReadyState.store(isReady);
    }
    
    // Function for Swift to update the Game Center authentication state
    void setGameCenterAuthState(bool isAuthenticated) {
        s_gameCenterAuthState.store(isAuthenticated);
    }
    
    // Functions for Swift to update/get Game Center player info
    void setGameCenterPlayerInfo(const char* playerName, const char* playerID) {
        if (playerName) s_gameCenterPlayerName = playerName;
        if (playerID) s_gameCenterPlayerID = playerID;
    }
    
    const char* getGameCenterPlayerName() {
        return s_gameCenterPlayerName.empty() ? "You" : s_gameCenterPlayerName.c_str();
    }
    
    const char* getGameCenterPlayerID() {
        return s_gameCenterPlayerID.c_str();
    }
    
    void ThreadingProxy::enqueueAdSetEnabled(bool enabled) {
        if (!s_instance) return;
        AdCommand cmd(CommandType::CMD_AD_SET_ENABLED);
        cmd.data.adsEnabled = enabled;
        s_instance->enqueueAdCommand(cmd);
    }
    
    // IAP command implementations
    void ThreadingProxy::enqueueIAPPurchase(const char* productID) {
        if (!s_instance) return;
        IAPCommand cmd(CommandType::CMD_IAP_PURCHASE);
        cmd.data.productID = productID;
        s_instance->enqueueIAPCommand(cmd);
    }
    
    void ThreadingProxy::enqueueIAPRestore() {
        if (!s_instance) return;
        IAPCommand cmd(CommandType::CMD_IAP_RESTORE);
        s_instance->enqueueIAPCommand(cmd);
    }
    
    bool ThreadingProxy::hasIAPPurchased(const char* productID) {
        // This will be implemented via Swift interop
        // For now, return false - will be wired up to StoreManager later
        return false;
    }
    
    const char* ThreadingProxy::getIAPPrice(const char* productID) {
        // This will be implemented via Swift interop
        // For now, return placeholder - will be wired up to StoreManager later
        return "$2.00";
    }
    
    // Asset loading command implementations
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
        int count = static_cast<int>(s_instance->m_touchData.size());
        if (count > 0) {
            GN_LOG_DEBUG("ThreadingProxy: getTouchCount() = %d", count);
        }
        return count;
    }

    void ThreadingProxy::getTouchPosition(int touchIndex, float* x, float* y) {
        if (!s_instance || touchIndex < 0 || touchIndex >= static_cast<int>(s_instance->m_touchData.size())) {
            if (x) *x = 0.0f;
            if (y) *y = 0.0f;
            return;
        }
        const auto& touch = s_instance->m_touchData[touchIndex];
        if (x) *x = touch.x;  // Normalized coordinates
        if (y) *y = touch.y;  // Normalized coordinates
        GN_LOG_DEBUG("ThreadingProxy: getTouchPosition(%d) = (%f, %f)", touchIndex, touch.x, touch.y);
    }
    
    bool ThreadingProxy::isTouchDown() {
        if (!s_instance) return false;
        return s_instance->m_isTouchDown;
    }
    
    bool ThreadingProxy::isTouchJustPressed() {
        if (!s_instance) return false;
        return s_instance->m_isTouchJustPressed;
    }
    
    bool ThreadingProxy::isTouchJustReleased() {
        if (!s_instance) return false;
        return s_instance->m_isTouchJustReleased;
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

        GN_LOG_INFO("🔗 ThreadingProxy: Touch update - pos(" + std::to_string(x) + ", " + std::to_string(y) + 
                    ") down=" + std::to_string(isDown) + " pressed=" + std::to_string(justPressed) + 
                    " released=" + std::to_string(justReleased));

        // Store touch state for delegate-based access by InputManager
        // NOTE: x,y are PIXEL coordinates from Swift, not normalized!
        s_instance->m_lastTouchX = x;
        s_instance->m_lastTouchY = y;
        s_instance->m_isTouchDown = isDown;
        s_instance->m_isTouchJustPressed = justPressed;
        s_instance->m_isTouchJustReleased = justReleased;

        // Store touch data in a format that can be accessed by delegates
        // Clear existing touches and add the current one
        s_instance->m_touchData.clear();
        if (isDown || justPressed || justReleased) {
            GameCore::TouchData touchData;
            touchData.touchId = 0;  // Single touch for now

            // Swift already sends device pixel coordinates, no scaling needed
            auto& configManager = GameCore::ConfigManager::Instance();
            const GameCore::ScreenInfo& screenInfo = configManager.GetCurrentScreenInfo();

            // Swift coordinates are already in device pixels - use directly
            float deviceX = x;
            float deviceY = y;

            // Store pixel coordinates directly (no normalization)
            touchData.x = deviceX;  // Pixel X coordinate
            touchData.y = deviceY;  // Pixel Y coordinate
            touchData.rawX = deviceX;  // Device pixel X
            touchData.rawY = deviceY;  // Device pixel Y

            // Determine touch state
            if (justPressed) {
                touchData.state = GameCore::TouchState::PRESSED;
            } else if (justReleased) {
                touchData.state = GameCore::TouchState::RELEASED;
            } else if (isDown) {
                touchData.state = GameCore::TouchState::HELD;
            } else {
                touchData.state = GameCore::TouchState::NONE;
            }

            touchData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            s_instance->m_touchData.push_back(touchData);

            GN_LOG_INFO("🔗 ThreadingProxy: Touch data stored - pixel(" + std::to_string(touchData.x) +
                        ", " + std::to_string(touchData.y) + ") state=" + std::to_string((int)touchData.state) +
                        " [PIXEL COORDS DIRECT]");
        } else {
            GN_LOG_INFO("🔗 ThreadingProxy: Touch data cleared - no active touch");
        }

        if (justPressed) {
            GN_LOG_INFO("🔥 ThreadingProxy: TOUCH PRESSED at (" + std::to_string(x) + ", " + std::to_string(y) + ") pixels");
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
        std::vector<AssetCommand> commands = std::move(m_assetCommandQueue);
        m_assetCommandQueue.clear();
        return commands;
    }
    
    std::vector<HapticCommand> ThreadingProxy::getAndClearHapticCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<HapticCommand> commands = std::move(m_hapticCommandQueue);
        m_hapticCommandQueue.clear();
        return commands;
    }
    
    std::vector<SaveCommand> ThreadingProxy::getAndClearSaveCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<SaveCommand> commands = std::move(m_saveCommandQueue);
        m_saveCommandQueue.clear();
        return commands;
    }
    
    std::vector<GameCenterCommand> ThreadingProxy::getAndClearGameCenterCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<GameCenterCommand> commands = std::move(m_gameCenterCommandQueue);
        m_gameCenterCommandQueue.clear();
        return commands;
    }
    
    std::vector<AdCommand> ThreadingProxy::getAndClearAdCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<AdCommand> commands = std::move(m_adCommandQueue);
        m_adCommandQueue.clear();
        return commands;
    }
    
    std::vector<IAPCommand> ThreadingProxy::getAndClearIAPCommands() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<IAPCommand> commands = std::move(m_iapCommandQueue);
        m_iapCommandQueue.clear();
        return commands;
    }
    
    size_t ThreadingProxy::getCommandCount() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_renderCommandQueue.size() + m_audioCommandQueue.size() + m_logCommandQueue.size() + m_assetCommandQueue.size() + m_hapticCommandQueue.size() + m_saveCommandQueue.size();
    }
    
    void ThreadingProxy::clearQueue() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_renderCommandQueue.clear();
        m_audioCommandQueue.clear();
        m_logCommandQueue.clear();
        m_assetCommandQueue.clear();
        m_hapticCommandQueue.clear();
        m_saveCommandQueue.clear();
        m_gameCenterCommandQueue.clear();
        m_adCommandQueue.clear();
        m_iapCommandQueue.clear();
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
        delegates.renderer.drawSpriteScaledCentered = enqueueDrawSpriteScaledCentered;
        delegates.renderer.drawSpriteScaledPivoted = enqueueDrawSpriteScaledPivoted;
        delegates.renderer.drawSpriteScaledWithSource = enqueueDrawSpriteScaledWithSource;
        delegates.renderer.drawSpriteScaledWithSourceCentered = enqueueDrawSpriteScaledWithSourceCentered;
        delegates.renderer.drawSpriteScaledWithSourcePivoted = enqueueDrawSpriteScaledWithSourcePivoted;
        delegates.renderer.drawSpriteBatch = enqueueDrawSpriteBatch;  // Batch rendering
        delegates.renderer.drawText = enqueueDrawText;
        delegates.renderer.drawTextCentered = enqueueDrawTextCentered;
        // Outlined text
        delegates.renderer.drawTextOutlined = enqueueDrawTextOutlined;
        delegates.renderer.drawTextCenteredOutlined = enqueueDrawTextCenteredOutlined;
        delegates.renderer.drawRectangle = enqueueDrawRectangle;
        delegates.renderer.drawCircle = enqueueDrawCircle;
        delegates.renderer.drawFilledCircle = enqueueDrawFilledCircle;
        delegates.renderer.getScreenSize = enqueueGetScreenSize;
        
        // NEW: Enhanced screen and texture information delegates
        delegates.renderer.getScreenInfo = getScreenInfoDelegate;
        delegates.renderer.getTextureMetadata = getTextureMetadataDelegate;
        
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

        // Configure asset texture metadata delegate (USES SAME FUNCTION AS RENDERER)
        delegates.asset.getTextureMetadata = getTextureMetadataDelegate;
        
        // Configure input delegates to handle touch input
        delegates.input.getPrimaryInputPosition = getPrimaryInputPosition;
        delegates.input.isPrimaryInputDown = isPrimaryInputDown;
        delegates.input.isPrimaryInputJustPressed = isPrimaryInputJustPressed;
        delegates.input.isPrimaryInputJustReleased = isPrimaryInputJustReleased;
        
        // Configure touch input delegates
        delegates.input.getTouchCount = getTouchCount;
        delegates.input.getTouchPosition = getTouchPosition;
        delegates.input.isTouchDown = isTouchDown;
        delegates.input.isTouchJustPressed = isTouchJustPressed;
        delegates.input.isTouchJustReleased = isTouchJustReleased;
        
        // Configure gesture detection delegates
        delegates.input.isSwipeLeftDetected = isSwipeLeftDetected;
        delegates.input.isSwipeRightDetected = isSwipeRightDetected;
        delegates.input.isSwipeUpDetected = isSwipeUpDetected;
        delegates.input.isSwipeDownDetected = isSwipeDownDetected;
        delegates.input.resetGestureState = resetGestureState;
        
        // Configure input buffer management
        delegates.input.clearInputBuffer = clearInputBuffer;
        
        // Configure haptic feedback delegates
        // Note: Actual implementations will be provided by Swift HapticManager
        // These are just placeholders that enqueue commands
        delegates.haptic.triggerImpact = [](HapticStyle style, float intensity) {
            ThreadingProxy::enqueueHapticImpact(style, intensity);
        };
        delegates.haptic.triggerSelection = []() {
            ThreadingProxy::enqueueHapticSelection();
        };
        delegates.haptic.triggerNotification = [](HapticNotificationType type) {
            ThreadingProxy::enqueueHapticNotification(type);
        };
        delegates.haptic.triggerPattern = [](const char* patternName) {
            ThreadingProxy::enqueueHapticPattern(patternName);
        };
        delegates.haptic.prepare = [](HapticStyle style) {
            ThreadingProxy::enqueueHapticPrepare(style);
        };
        // setEnabled, isEnabled, and isSupported will be handled by Swift directly
        // as they need to query/modify state
        
        // Configure Save/Load delegates
        // Note: These enqueue commands that will be processed by Swift SaveManager
        delegates.save.saveGameData = [](const char* jsonData) -> bool {
            ThreadingProxy::enqueueSaveGame(jsonData);
            return true; // Queued successfully
        };
        delegates.save.loadGameData = [](const char** outJsonData) -> bool {
            GN_LOG_INFO("🔍 loadGameData delegate called - calling loadGameDataSync()");
            // Synchronous load - call Swift directly via C++ interop
            static std::string loadedData;
            loadedData = loadGameDataSync();
            
            GN_LOG_INFO("🔍 loadGameDataSync returned " + std::to_string(loadedData.length()) + " chars");
            
            if (!loadedData.empty()) {
                *outJsonData = loadedData.c_str();
                GN_LOG_INFO("✅ Load succeeded - returning JSON data");
                return true; // Load succeeded
            }
            GN_LOG_WARN("⚠️ No save data - returning false");
            return false; // No save data
        };
        delegates.save.saveSettings = []() {
            ThreadingProxy::enqueueSaveSettings();
        };
        delegates.save.loadSettings = []() {
            ThreadingProxy::enqueueLoadSettings();
            // Note: Settings are automatically applied from UserDefaults/GameSettings
        };
        delegates.save.hasLegacySaveFile = []() -> bool {
            return false; // Will be handled by Swift side
        };
        delegates.save.deleteSaveData = []() {
            // Will be handled by Swift side if needed
        };
        
        // Configure Game Center delegates
        // Note: Actual implementations will be provided by Swift GameCenterManager
        // These enqueue commands that will be processed by Swift CommandProcessor
        delegates.gameCenter.authenticate = [](void (*completion)(bool success)) {
            ThreadingProxy::enqueueGameCenterAuthenticate();
            // Completion callback will be handled by Swift side
            if (completion) completion(false); // Placeholder - actual auth is async
        };
        delegates.gameCenter.isAuthenticated = []() -> bool {
            return ThreadingProxy::isGameCenterAuthenticated();
        };
        delegates.gameCenter.submitScore = [](const char* leaderboardID, int64_t score, void (*completion)(bool success)) {
            ThreadingProxy::enqueueGameCenterSubmitScore(leaderboardID, score);
            // Completion callback will be handled by Swift side
            if (completion) completion(true); // Placeholder - queued successfully
        };
        delegates.gameCenter.showLeaderboard = [](const char* leaderboardID) {
            ThreadingProxy::enqueueGameCenterShowLeaderboard(leaderboardID);
        };
        delegates.gameCenter.showAllLeaderboards = []() {
            ThreadingProxy::enqueueGameCenterShowAllLeaderboards();
        };
        delegates.gameCenter.loadLeaderboardEntries = [](
            const char* leaderboardID,
            void (*completion)(const LeaderboardEntry* entries, int count, bool success)
        ) {
            // Pass the completion callback through to Swift via the command
            ThreadingProxy::enqueueGameCenterLoadLeaderboardEntries(leaderboardID, completion);
        };
        delegates.gameCenter.loadLocalPlayerEntry = [](
            const char* leaderboardID,
            void (*completion)(int rank, int64_t score, bool success)
        ) {
            // Pass the completion callback through to Swift via the command
            ThreadingProxy::enqueueGameCenterLoadLocalPlayerEntry(leaderboardID, completion);
        };
        delegates.gameCenter.getPlayerName = []() -> const char* {
            return GameCore::getGameCenterPlayerName();
        };
        delegates.gameCenter.getPlayerID = []() -> const char* {
            return GameCore::getGameCenterPlayerID();
        };
        
        // Configure Ad delegates
        // Note: Actual implementations will be provided by Swift AdManager
        // These enqueue commands that will be processed by Swift CommandProcessor
        delegates.ad.preloadAd = []() {
            ThreadingProxy::enqueueAdPreload();
        };
        delegates.ad.showAd = []() {
            ThreadingProxy::enqueueAdShow();
        };
        delegates.ad.isAdReady = []() -> bool {
            return ThreadingProxy::isAdReady();
        };
        delegates.ad.setAdsEnabled = [](bool enabled) {
            ThreadingProxy::enqueueAdSetEnabled(enabled);
        };
        
        // Configure IAP delegates
        delegates.iap.purchase = [](const char* productID, void (*completion)(bool, const char*)) {
            GN_LOG_INFO("IAP purchase requested for: " + std::string(productID));
            ThreadingProxy::enqueueIAPPurchase(productID);
            // Note: completion callback will be invoked by Swift StoreManager after processing
            (void)completion; // Callback handled by Swift bridge
        };
        delegates.iap.restore = [](void (*completion)(bool, const char*)) {
            GN_LOG_INFO("IAP restore requested");
            ThreadingProxy::enqueueIAPRestore();
            // Note: completion callback will be invoked by Swift StoreManager after processing
            (void)completion; // Callback handled by Swift bridge
        };
        delegates.iap.hasPurchased = [](const char* productID) -> bool {
            return ThreadingProxy::hasIAPPurchased(productID);
        };
        delegates.iap.getPrice = [](const char* productID) -> const char* {
            return ThreadingProxy::getIAPPrice(productID);
        };
        
        GN_LOG_INFO("ThreadingProxy: Input delegates configured - touch input will flow from iOS->ThreadingProxy->C++");
        GN_LOG_INFO("ThreadingProxy: Haptic delegates configured - haptic feedback commands will flow through queue");
        GN_LOG_INFO("ThreadingProxy: Save/Load delegates configured - save operations will flow through queue");
        GN_LOG_INFO("ThreadingProxy: Game Center delegates configured - leaderboard commands will flow through queue");
        GN_LOG_INFO("ThreadingProxy: Ad delegates configured - advertising commands will flow through queue");
        GN_LOG_INFO("ThreadingProxy: IAP delegates configured - in-app purchase commands ready");
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

std::vector<HapticCommand> getAndClearHapticCommandsFromProxy() {
    if (g_threadingProxy) {
        return g_threadingProxy->getAndClearHapticCommands();
    }
    return std::vector<HapticCommand>();
}

std::vector<SaveCommand> getAndClearSaveCommandsFromProxy() {
    if (g_threadingProxy) {
        return g_threadingProxy->getAndClearSaveCommands();
    }
    return std::vector<SaveCommand>();
}

std::vector<GameCenterCommand> getAndClearGameCenterCommandsFromProxy() {
    if (g_threadingProxy) {
        return g_threadingProxy->getAndClearGameCenterCommands();
    }
    return std::vector<GameCenterCommand>();
}

std::vector<AdCommand> getAndClearAdCommandsFromProxy() {
    if (g_threadingProxy) {
        return g_threadingProxy->getAndClearAdCommands();
    }
    return std::vector<AdCommand>();
}

std::vector<IAPCommand> getAndClearIAPCommandsFromProxy() {
    if (g_threadingProxy) {
        return g_threadingProxy->getAndClearIAPCommands();
    }
    return std::vector<IAPCommand>();
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

// Enhanced screen and texture information delegates - following existing command system design
void ThreadingProxy::getScreenInfoDelegate(ScreenInfo* info) {
    if (!info) {
        GN_LOG_ERROR("getScreenInfoDelegate: info parameter is null");
        return;
    }
    
    if (!s_instance) {
        GN_LOG_ERROR("getScreenInfoDelegate: No ThreadingProxy instance available");
        return;
    }
    
    // Follow the existing command system design pattern - enqueue command for processing
    s_instance->enqueueGetScreenInfo(info);
    
    GN_LOG_DEBUG("getScreenInfoDelegate: Enqueued screen info request following command system design");
}

bool ThreadingProxy::getTextureMetadataDelegate(const char* textureId, TextureMetadata* metadata) {
    if (!textureId || !metadata) {
        GN_LOG_ERROR("getTextureMetadataDelegate: null parameters");
        return false;
    }
    
    if (!s_instance) {
        GN_LOG_ERROR("getTextureMetadataDelegate: No ThreadingProxy instance available");
        return false;
    }
    
    // Follow the existing command system design pattern - enqueue command for processing
    s_instance->enqueueGetTextureMetadata(textureId, metadata);
    
    GN_LOG_DEBUG("getTextureMetadataDelegate: Enqueued texture metadata request for: " + std::string(textureId));
    
    // Return true indicating command was enqueued (actual result processed via command system)
    return true;
}

// Logging interface implementation for GNLog.h
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

// Swift CXX Interop Implementation
void setScreenInfoDirect(const ScreenInfo& screenInfo) {
    GN_LOG_INFO("setScreenInfoDirect: Receiving screen info from Swift - " +
                std::to_string((int)screenInfo.pixelWidth) + "x" + 
                std::to_string((int)screenInfo.pixelHeight) + " pixels, " +
                std::to_string(screenInfo.logicalWidth) + "x" + 
                std::to_string(screenInfo.logicalHeight) + " logical");
    
    // Set screen info directly on ConfigManager
    auto& configManager = ConfigManager::Instance();
    configManager.SetScreenInfoDirect(screenInfo);
    
    GN_LOG_INFO("setScreenInfoDirect: Screen info updated successfully");
}

std::string loadGameDataSync() {
    GN_LOG_INFO("🔍 C++ loadGameDataSync() called - about to call Swift");
    
    // Call Swift function via C++ interop
    auto swiftString = FloppyTurd::loadGameDataSync();
    
    // Convert Swift.String to std::string
    std::string result = std::string(swiftString);
    
    GN_LOG_INFO("✅ Swift loadGameDataSync() returned " + std::to_string(result.length()) + " chars");
    return result;
}

} // namespace GameCore

