#ifdef PLATFORM_IOS

#include "iOSPlatformImpl.h"
#include "../../iOS/Threading/ThreadingProxy.h"
#include "../Utility/Helper.h"
#include "../Core/GNLog.h"
#include <cstring>

namespace GameCore {

    // Global threading proxy for command queue (extern reference)
    extern ThreadingProxy* g_threadingProxy;

    namespace iOSPlatform {
        
        void SetupDelegates(PlatformDelegates& delegates) {
            // Use ThreadingProxy for logging instead of GNLog
            // ThreadingProxy is already working with SwiftLog bridge
            GameCore::ThreadingProxy::enqueueLogInfo("Initializing iOS platform delegates...", "PLATFORM");
            
            // Initialize threading proxy if not already done
            if (!g_threadingProxy) {
                g_threadingProxy = new ThreadingProxy();
            }
            
            // Use ThreadingProxy to setup delegates with command queue
            g_threadingProxy->setupDelegates(delegates);
            
            // Set up asset loading delegates
            // Asset loading delegates are handled by ThreadingProxy::setupDelegates
            // which assigns the enqueue functions directly
            delegates.asset.getAssetPath = GetAssetPath;
            delegates.asset.fileExists = FileExists;
            delegates.asset.platformContext = nullptr;
            
            GameCore::ThreadingProxy::enqueueLogInfo("iOS platform delegates configured successfully", "PLATFORM");
        }
        
        // SetSwiftComponents removed - Swift components managed entirely on Swift side
        
        // Renderer implementations (using ThreadingProxy static methods)
        void BeginFrame() {
            ThreadingProxy::enqueueBeginFrame();
        }
        
        void EndFrame() {
            ThreadingProxy::enqueueEndFrame();
        }
        
        void Present() {
            ThreadingProxy::enqueuePresent();
        }
        
        void ClearScreen(float r, float g, float b, float a) {
            ThreadingProxy::enqueueClearScreen(r, g, b, a);
        }
        
        void DrawSprite(uint32_t textureHandle, float x, float y, float rotation) {
            if (textureHandle != 0) {
                ThreadingProxy::enqueueDrawSprite(textureHandle, x, y, rotation);
            }
        }
        
        void DrawSpriteScaled(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation) {
            if (textureHandle != 0) {
                ThreadingProxy::enqueueDrawSpriteScaled(textureHandle, x, y, scaleX, scaleY, rotation);
            }
        }
        
        void DrawText(const char* text, float x, float y, float fontSize, float r, float g, float b, float a) {
            if (text) {
                ThreadingProxy::enqueueDrawText(text, x, y, fontSize, r, g, b, a);
            }
        }
        
        void DrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a) {
            ThreadingProxy::enqueueDrawRectangle(x, y, width, height, r, g, b, a);
        }
        
        void DrawCircle(float x, float y, float radius, float r, float g, float b, float a) {
            ThreadingProxy::enqueueDrawCircle(x, y, radius, r, g, b, a);
        }
        
        void GetScreenSize(float* width, float* height) {
            if (width && height) {
                ThreadingProxy::enqueueGetScreenSize(width, height);
            }
        }
        
        // Audio implementations (using ThreadingProxy static methods)
        void PlayMusic(const char* musicName, float volume, int loopCount) {
            ThreadingProxy::enqueuePlayMusic(musicName, volume, loopCount);
        }
        
        void StopMusic() {
            ThreadingProxy::enqueueStopMusic();
        }
        
        void PlaySound(const char* soundName, float volume) {
            ThreadingProxy::enqueuePlaySound(soundName, volume);
        }
        
        void StopSound() {
            ThreadingProxy::enqueueStopSound();
        }
        
        void SetMusicVolume(float volume) {
            ThreadingProxy::enqueueSetMusicVolume(volume);
        }
        
        void SetSoundVolume(float volume) {
            ThreadingProxy::enqueueSetSoundVolume(volume);
        }
        
        // Logging implementations (using ThreadingProxy static methods)
        void LogTrace(const char* message, const char* category) {
            ThreadingProxy::enqueueLogTrace(message, category);
        }
        
        void LogDebug(const char* message, const char* category) {
            ThreadingProxy::enqueueLogDebug(message, category);
        }
        
        void LogInfo(const char* message, const char* category) {
            ThreadingProxy::enqueueLogInfo(message, category);
        }
        
        void LogWarn(const char* message, const char* category) {
            ThreadingProxy::enqueueLogWarn(message, category);
        }
        
        void LogError(const char* message, const char* category) {
            ThreadingProxy::enqueueLogError(message, category);
        }
        
        void LogFatal(const char* message, const char* category) {
            ThreadingProxy::enqueueLogFatal(message, category);
        }
        
        // Asset loading implementations - using modern callback signatures
        void LoadTexture(const std::string& texturePath,
                        void (*callback)(TextureData* texture, const char* error, void* userData),
                        void* userData) {
            if (texturePath.empty() || !callback) return;
            ThreadingProxy::enqueueLoadTexture(texturePath, callback, userData);
        }

        void LoadAudio(const std::string& audioPath,
                      void (*callback)(void* audioData, size_t size, const char* error, void* userData),
                      void* userData) {
            if (audioPath.empty() || !callback) return;
            ThreadingProxy::enqueueLoadAudio(audioPath, callback, userData);
        }

        void LoadFont(const std::string& fontPath, int size,
                     void (*callback)(void* fontData, const char* error, void* userData),
                     void* userData) {
            if (fontPath.empty() || !callback) return;
            ThreadingProxy::enqueueLoadFont(fontPath, size, callback, userData);
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
            ThreadingProxy::enqueueLoadData(dataPath, callback, userData);
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