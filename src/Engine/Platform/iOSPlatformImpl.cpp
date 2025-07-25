#ifdef __APPLE__
#if TARGET_OS_IPHONE

#include "iOSPlatformImpl.h"
#include "../../iOS/Threading/ThreadingProxy.h"
#include "../Graphics/Sprite.h"
#include <cstring>

namespace FloppyTurd {

    // Global iOS component pointers (native Swift/C++ interop)
    static MetalRenderer* g_metalRenderer = nullptr;
    static TouchInputHandler* g_touchInputHandler = nullptr;
    static AudioManagerSwift* g_audioManager = nullptr;
    
    // Global threading proxy for command queue
    static ThreadingProxy* g_threadingProxy = nullptr;

    namespace iOSPlatform {
        
        void SetupDelegates(PlatformDelegates& delegates) {
            // Initialize threading proxy if not already done
            if (!g_threadingProxy) {
                g_threadingProxy = new ThreadingProxy();
            }
            
            // Use ThreadingProxy to setup delegates with command queue
            g_threadingProxy->setupDelegates(delegates);
            
            // Input delegates
            delegates.input.isActionPressed = IsActionPressed;
            delegates.input.isActionJustPressed = IsActionJustPressed;
            delegates.input.isActionReleased = IsActionReleased;
            delegates.input.getPrimaryInputPosition = GetPrimaryInputPosition;
            delegates.input.isPrimaryInputDown = IsPrimaryInputDown;
            delegates.input.isPrimaryInputJustPressed = IsPrimaryInputJustPressed;
            delegates.input.isPrimaryInputJustReleased = IsPrimaryInputJustReleased;
            delegates.input.getTouchCount = GetTouchCount;
            delegates.input.getTouchPosition = GetTouchPosition;
            delegates.input.isKeyPressed = IsKeyPressed;
            delegates.input.isKeyJustPressed = IsKeyJustPressed;
            
            // Audio delegates
            delegates.audio.playSound = PlaySound;
            delegates.audio.playSoundWithVolume = PlaySoundWithVolume;
            delegates.audio.stopSound = StopSound;
            delegates.audio.playMusic = PlayMusic;
            delegates.audio.stopMusic = StopMusic;
            delegates.audio.pauseMusic = PauseMusic;
            delegates.audio.resumeMusic = ResumeMusic;
            delegates.audio.setMasterVolume = SetMasterVolume;
            delegates.audio.setSFXVolume = SetSFXVolume;
            delegates.audio.setMusicVolume = SetMusicVolume;
            delegates.audio.isMusicPlaying = IsMusicPlaying;
            delegates.audio.isSoundPlaying = IsSoundPlaying;
        }
        
        void SetSwiftComponents(MetalRenderer* renderer, TouchInputHandler* input, AudioManagerSwift* audio) {
            g_metalRenderer = renderer;
            g_touchInputHandler = input;
            g_audioManager = audio;
            
            // Set Swift components in threading proxy for command processing
            if (g_threadingProxy) {
                g_threadingProxy->setSwiftComponents(renderer, input, audio);
            }
        }
        
        // Renderer implementations (native Swift/C++ interop)
        void BeginFrame() {
            if (g_metalRenderer) {
                g_metalRenderer->beginFrame();
            }
        }
        
        void EndFrame() {
            if (g_metalRenderer) {
                g_metalRenderer->endFrame();
            }
        }
        
        void Present() {
            if (g_metalRenderer) {
                g_metalRenderer->present();
            }
        }
        
        void ClearScreen(float r, float g, float b, float a) {
            if (g_metalRenderer) {
                g_metalRenderer->clearScreen(r, g, b, a);
            }
        }
        
        void DrawSprite(void* sprite, float x, float y, float rotation) {
            if (g_metalRenderer && sprite) {
                g_metalRenderer->drawSprite(sprite, x, y, rotation);
            }
        }
        
        void DrawSpriteScaled(void* sprite, float x, float y, float scaleX, float scaleY, float rotation) {
            if (g_metalRenderer && sprite) {
                g_metalRenderer->drawSpriteScaled(sprite, x, y, scaleX, scaleY, rotation);
            }
        }
        
        void DrawText(const char* text, float x, float y, float fontSize, float r, float g, float b, float a) {
            if (g_metalRenderer && text) {
                g_metalRenderer->drawText(text, x, y, fontSize, r, g, b, a);
            }
        }
        
        void DrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a) {
            if (g_metalRenderer) {
                g_metalRenderer->drawRectangle(x, y, width, height, r, g, b, a);
            }
        }
        
        void DrawCircle(float x, float y, float radius, float r, float g, float b, float a) {
            if (g_metalRenderer) {
                g_metalRenderer->drawCircle(x, y, radius, r, g, b, a);
            }
        }
        
        void GetScreenSize(float* width, float* height) {
            if (g_metalRenderer && width && height) {
                auto size = g_metalRenderer->getScreenSize();
                *width = size.width;
                *height = size.height;
            }
        }
        
        // Input implementations (native Swift/C++ interop)
        bool IsActionPressed(int action) {
            if (g_touchInputHandler) {
                return g_touchInputHandler->isActionPressed(action);
            }
            return false;
        }
        
        bool IsActionJustPressed(int action) {
            if (g_touchInputHandler) {
                return g_touchInputHandler->isActionJustPressed(action);
            }
            return false;
        }
        
        bool IsActionReleased(int action) {
            if (g_touchInputHandler) {
                return g_touchInputHandler->isActionReleased(action);
            }
            return false;
        }
        
        void GetPrimaryInputPosition(float* x, float* y) {
            if (g_touchInputHandler && x && y) {
                auto position = g_touchInputHandler->getPrimaryInputPosition();
                *x = position.x;
                *y = position.y;
            }
        }
        
        bool IsPrimaryInputDown() {
            if (g_touchInputHandler) {
                return g_touchInputHandler->isPrimaryInputDown();
            }
            return false;
        }
        
        bool IsPrimaryInputJustPressed() {
            if (g_touchInputHandler) {
                return g_touchInputHandler->isPrimaryInputJustPressed();
            }
            return false;
        }
        
        bool IsPrimaryInputJustReleased() {
            if (g_touchInputHandler) {
                return g_touchInputHandler->isPrimaryInputJustReleased();
            }
            return false;
        }
        
        int GetTouchCount() {
            if (g_touchInputHandler) {
                return g_touchInputHandler->getTouchCount();
            }
            return 0;
        }
        
        void GetTouchPosition(int touchIndex, float* x, float* y) {
            if (g_touchInputHandler && x && y) {
                auto position = g_touchInputHandler->getTouchPosition(touchIndex);
                *x = position.x;
                *y = position.y;
            }
        }
        
        bool IsKeyPressed(int keyCode) {
            // iOS doesn't have keyboard input in typical game scenarios
            return false;
        }
        
        bool IsKeyJustPressed(int keyCode) {
            // iOS doesn't have keyboard input in typical game scenarios
            return false;
        }
        
        // Audio implementations (native Swift/C++ interop)
        void PlaySound(const char* soundName) {
            if (g_audioManager && soundName) {
                g_audioManager->playSound(soundName);
            }
        }
        
        void PlaySoundWithVolume(const char* soundName, float volume) {
            if (g_audioManager && soundName) {
                g_audioManager->playSoundWithVolume(soundName, volume);
            }
        }
        
        void StopSound(const char* soundName) {
            if (g_audioManager && soundName) {
                g_audioManager->stopSound(soundName);
            }
        }
        
        void PlayMusic(const char* musicName) {
            if (g_audioManager && musicName) {
                g_audioManager->playMusic(musicName);
            }
        }
        
        void StopMusic() {
            if (g_audioManager) {
                g_audioManager->stopMusic();
            }
        }
        
        void PauseMusic() {
            if (g_audioManager) {
                g_audioManager->pauseMusic();
            }
        }
        
        void ResumeMusic() {
            if (g_audioManager) {
                g_audioManager->resumeMusic();
            }
        }
        
        void SetMasterVolume(float volume) {
            if (g_audioManager) {
                g_audioManager->setMasterVolume(volume);
            }
        }
        
        void SetSFXVolume(float volume) {
            if (g_audioManager) {
                g_audioManager->setSFXVolume(volume);
            }
        }
        
        void SetMusicVolume(float volume) {
            if (g_audioManager) {
                g_audioManager->setMusicVolume(volume);
            }
        }
        
        bool IsMusicPlaying() {
            if (g_audioManager) {
                return g_audioManager->isMusicPlaying();
            }
            return false;
        }
        
        bool IsSoundPlaying(const char* soundName) {
            if (g_audioManager && soundName) {
                return g_audioManager->isSoundPlaying(soundName);
            }
            return false;
        }
        
    } // namespace iOSPlatform

} // namespace FloppyTurd

// Native C++ interface for Swift interop (no extern C needed)
namespace FloppyTurd {
    namespace iOSPlatform {
        
        // Swift-accessible function to set iOS components
        void setIOSComponents(MetalRenderer* renderer, TouchInputHandler* input, AudioManagerSwift* audio) {
            SetSwiftComponents(renderer, input, audio);
        }
        
    } // namespace iOSPlatform
} // namespace FloppyTurd

#endif // TARGET_OS_IPHONE
#endif // __APPLE__