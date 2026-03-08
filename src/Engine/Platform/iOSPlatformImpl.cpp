#ifdef PLATFORM_IOS

#include "iOSPlatformImpl.h"
#include "../../iOS/Threading/ThreadingProxy.h"
#include "../Utility/Utils.h"
#include "../Core/GNLog.h"
#include <cstring>

namespace GameCore {

    // Global threading proxy for command queue (extern reference)
    extern ThreadingProxy* g_threadingProxy;

    // Global platform delegates pointer (definition)
    PlatformDelegates* g_platformDelegates = nullptr;


void InitializePlatformDelegates() {
    static PlatformDelegates delegates;
    iOSPlatform::SetupDelegates(delegates);
    g_platformDelegates = &delegates;
}

    namespace iOSPlatform {
        // Asset cache management delegate implementations
        // Note: Removed redundant wrapper functions - delegates now point directly to ThreadingProxy
        
        void SetupDelegates(PlatformDelegates& delegates) {
            // Use ThreadingProxy for logging instead of GNLog
            GameCore::ThreadingProxy::enqueueLogInfo("Initializing iOS platform delegates...", "PLATFORM");

            // Ensure global threading proxy is initialized
            if (!GameCore::g_threadingProxy) {
                GameCore::g_threadingProxy = new GameCore::ThreadingProxy();
            }
            // Setup delegates using ThreadingProxy's command queue
            GameCore::g_threadingProxy->setupDelegates(delegates);

            // Asset loading delegates (direct assignment, consistent with rest of file)
            delegates.asset.getAssetPath = GameCore::iOSPlatform::GetAssetPath;
            delegates.asset.fileExists = GameCore::iOSPlatform::FileExists;
            delegates.asset.platformContext = nullptr;

            // Asset cache management delegates (direct ThreadingProxy assignment)
            delegates.asset.preloadEssentialAssets = GameCore::ThreadingProxy::enqueuePreloadEssentialAssets;
            delegates.asset.isCached = GameCore::ThreadingProxy::enqueueIsCached;
            
            // Renderer delegates - add screen info support
            delegates.renderer.getScreenInfo = GameCore::iOSPlatform::GetScreenInfo;
            delegates.renderer.getScreenSize = GameCore::iOSPlatform::GetScreenSize;

            // Orientation control delegates
            delegates.renderer.lockOrientation = GameCore::iOSPlatform::LockOrientation;
            delegates.renderer.unlockOrientation = GameCore::iOSPlatform::UnlockOrientation;
            delegates.renderer.lockToPortrait = GameCore::iOSPlatform::LockToPortrait;
            delegates.renderer.lockToLandscape = GameCore::iOSPlatform::LockToLandscape;

            GameCore::ThreadingProxy::enqueueLogInfo("iOS platform delegates configured successfully", "PLATFORM");
        }
        
        // SetSwiftComponents removed - Swift components managed entirely on Swift side
        
        // Renderer implementations (using ThreadingProxy static methods)
        void BeginFrame() {
            GameCore::ThreadingProxy::enqueueBeginFrame();
        }
        
        void EndFrame() {
            GameCore::ThreadingProxy::enqueueEndFrame();
        }
        
        void Present() {
            GameCore::ThreadingProxy::enqueuePresent();
        }
        
        void ClearScreen(float r, float g, float b, float a) {
            GameCore::ThreadingProxy::enqueueClearScreen(r, g, b, a);
        }
        
        void DrawSprite(uint32_t textureHandle, float x, float y, float rotation) {
            if (textureHandle != 0) {
                GameCore::ThreadingProxy::enqueueDrawSprite(textureHandle, x, y, rotation);
            }
        }
        
        void DrawSpriteScaled(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation) {
            if (textureHandle != 0) {
                GameCore::ThreadingProxy::enqueueDrawSpriteScaled(textureHandle, x, y, scaleX, scaleY, rotation);
            }
        }
        
        void DrawSpriteScaledPivoted(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, float pivotX, float pivotY) {
            if (textureHandle != 0) {
                GameCore::ThreadingProxy::enqueueDrawSpriteScaledPivoted(textureHandle, x, y, scaleX, scaleY, rotation, pivotX, pivotY);
            }
        }
        
        void DrawText(const std::string& text, float x, float y, float fontSize, float r, float g, float b, float a) {
            if (!text.empty()) {
                GameCore::ThreadingProxy::enqueueDrawText(text, x, y, fontSize, r, g, b, a);
            }
        }
        
        void DrawTextCentered(const std::string& text, float x, float y, float fontSize, float r, float g, float b, float a) {
            if (!text.empty()) {
                GameCore::ThreadingProxy::enqueueDrawTextCentered(text, x, y, fontSize, r, g, b, a);
            }
        }

        void DrawTextOutlined(const std::string& text, float x, float y, float fontSize,
                               float textR, float textG, float textB, float textA,
                               float outlineR, float outlineG, float outlineB, float outlineA,
                               float outlineWidth) {
            if (!text.empty()) {
                GameCore::ThreadingProxy::enqueueDrawTextOutlined(text, x, y, fontSize,
                                                                  textR, textG, textB, textA,
                                                                  outlineR, outlineG, outlineB, outlineA,
                                                                  outlineWidth);
            }
        }

        void DrawTextCenteredOutlined(const std::string& text, float x, float y, float fontSize,
                                      float textR, float textG, float textB, float textA,
                                      float outlineR, float outlineG, float outlineB, float outlineA,
                                      float outlineWidth) {
            if (!text.empty()) {
                GameCore::ThreadingProxy::enqueueDrawTextCenteredOutlined(text, x, y, fontSize,
                                                                          textR, textG, textB, textA,
                                                                          outlineR, outlineG, outlineB, outlineA,
                                                                          outlineWidth);
            }
        }
        
        void DrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a) {
            GameCore::ThreadingProxy::enqueueDrawRectangle(x, y, width, height, r, g, b, a);
        }
        
        void DrawCircle(float x, float y, float radius, float r, float g, float b, float a) {
            GameCore::ThreadingProxy::enqueueDrawCircle(x, y, radius, r, g, b, a);
        }
        
        void GetScreenSize(float* width, float* height) {
            if (width && height) {
                GameCore::ThreadingProxy::enqueueGetScreenSize(width, height);
            }
        }
        
        void GetScreenInfo(ScreenInfo* screenInfo) {
            if (screenInfo) {
                GameCore::ThreadingProxy::enqueueGetScreenInfo(screenInfo);
            }
        }

        void LockOrientation() {
            // Enqueue orientation lock command to Swift side
            GameCore::ThreadingProxy::enqueueLockOrientation();
        }

        void UnlockOrientation() {
            // Enqueue orientation unlock command to Swift side
            GameCore::ThreadingProxy::enqueueUnlockOrientation();
        }

        void LockToPortrait() {
            // Enqueue lock to portrait command to Swift side
            GameCore::ThreadingProxy::enqueueLockToPortrait();
        }

        void LockToLandscape() {
            // Enqueue lock to landscape command to Swift side
            GameCore::ThreadingProxy::enqueueLockToLandscape();
        }
        
        // Audio implementations (using ThreadingProxy static methods)
        void PlayMusic(const char* musicName, float volume, int loopCount) {
            GameCore::ThreadingProxy::enqueuePlayMusic(musicName, volume, loopCount);
        }
        
        void StopMusic() {
            GameCore::ThreadingProxy::enqueueStopMusic();
        }
        
        void PlaySound(const char* soundName, float volume) {
            GameCore::ThreadingProxy::enqueuePlaySound(soundName, volume);
        }
        
        void StopSound(const char* soundName) {
            if (!soundName) return;
            GameCore::ThreadingProxy::enqueueStopSound(soundName);
        }
        
        void SetMusicVolume(float volume) {
            GameCore::ThreadingProxy::enqueueSetMusicVolume(volume);
        }
        
        void SetSoundVolume(float volume) {
            GameCore::ThreadingProxy::enqueueSetSoundVolume(volume);
        }
        
        // Logging implementations (using ThreadingProxy static methods)
        void LogTrace(const char* message, const char* category) {
            GameCore::ThreadingProxy::enqueueLogTrace(message, category);
        }
        
        void LogDebug(const char* message, const char* category) {
            GameCore::ThreadingProxy::enqueueLogDebug(message, category);
        }
        
        void LogInfo(const char* message, const char* category) {
            GameCore::ThreadingProxy::enqueueLogInfo(message, category);
        }
        
        void LogWarn(const char* message, const char* category) {
            GameCore::ThreadingProxy::enqueueLogWarn(message, category);
        }
        
        void LogError(const char* message, const char* category) {
            GameCore::ThreadingProxy::enqueueLogError(message, category);
        }
        
        void LogFatal(const char* message, const char* category) {
            GameCore::ThreadingProxy::enqueueLogFatal(message, category);
        }
        
        // Asset loading implementations - using modern callback signatures
        void LoadTexture(const std::string& texturePath,
                        void (*callback)(GameCore::TextureData* texture, const char* error, void* userData),
                        void* userData) {
            if (texturePath.empty() || !callback) return;
            GameCore::ThreadingProxy::enqueueLoadTexture(texturePath, callback, userData);
        }

        void LoadAudio(const std::string& audioPath,
                      void (*callback)(void* audioData, size_t size, const char* error, void* userData),
                      void* userData) {
            if (audioPath.empty() || !callback) return;
            GameCore::ThreadingProxy::enqueueLoadAudio(audioPath, callback, userData);
        }

        void LoadFont(const std::string& fontPath, int size,
                     void (*callback)(void* fontData, const char* error, void* userData),
                     void* userData) {
            if (fontPath.empty() || !callback) return;
            GameCore::ThreadingProxy::enqueueLoadFont(fontPath, size, callback, userData);
        }

        void LoadShader(const std::string& vertexPath, const std::string& fragmentPath,
                       void (*callback)(void* shaderProgram, const char* error, void* userData),
                       void* userData) {
            if (vertexPath.empty() || fragmentPath.empty() || !callback) return;
            // For iOS, shaders are handled by Metal - call success immediately
            callback(nullptr, nullptr, userData);
        }

        void LoadData(const std::string& dataPath,
                     void (*callback)(void* data, size_t size, const char* error, void* userData),
                     void* userData) {
            if (dataPath.empty() || !callback) return;
            GameCore::ThreadingProxy::enqueueLoadData(dataPath, callback, userData);
        }

        const char* GetAssetPath(const char* relativePath) {
            if (!relativePath) return "";
            
            // For iOS asset catalogs, return the asset name as-is
            // The Swift AssetManager will handle the actual asset loading
            return relativePath;
        }

        bool FileExists(const char* relativePath) {
            if (!relativePath) return false;
            
            // For iOS asset catalogs, always return true since the asset catalog system
            // handles file existence internally. The actual loading will be handled by
            // the asset loading system which can properly access bundled assets.
            return true;
            
            // Legacy filesystem check (commented out for iOS asset catalogs)
            // return GameCore::FileExists(relativePath);
        }
        
        // Input is handled by ThreadingProxy's setupDelegates method
        // No need for manual implementations here
        
    } // namespace iOSPlatform
} // namespace GameCore
#endif // PLATFORM_IOS