#ifndef PLATFORM_IOS

#include "RaylibPlatformImpl.h"
#include <unordered_map>
#include <string>

// Forward declare Sprite for now
class Sprite {
public:
    uint32_t GetTextureID() const { return 0; }
};

#ifdef RAYLIB_AVAILABLE
#include <raylib.h>
#else
// Stub definitions when raylib is not available
typedef struct { int frameCount; } Sound;
typedef struct { int frameCount; } Music;
typedef struct { unsigned char r, g, b, a; } Color;
typedef struct { float x, y, width, height; } Rectangle;
typedef struct { float x, y; } Vector2;
typedef struct { unsigned int id; } Texture2D;

#define WHITE (Color){255, 255, 255, 255}
#define MOUSE_BUTTON_LEFT 0
#define KEY_SPACE 32

// Stub functions
void InitWindow(int w, int h, const char* t) {}
void InitAudioDevice() {}
void CloseAudioDevice() {}
void CloseWindow() {}
void BeginDrawing() {}
void EndDrawing() {}
void ClearBackground(Color c) {}
void DrawTexturePro(Texture2D t, Rectangle s, Rectangle d, Vector2 o, float r, Color c) {}
void DrawText(const char* t, int x, int y, int s, Color c) {}
void DrawRectangle(int x, int y, int w, int h, Color c) {}
void DrawCircle(int x, int y, float r, Color c) {}
int GetScreenWidth() { return 800; }
int GetScreenHeight() { return 600; }
bool IsMouseButtonDown(int b) { return false; }
bool IsKeyDown(int k) { return false; }
bool IsMouseButtonPressed(int b) { return false; }
bool IsKeyPressed(int k) { return false; }
bool IsMouseButtonReleased(int b) { return false; }
bool IsKeyReleased(int k) { return false; }
Vector2 GetMousePosition() { return {0, 0}; }
int GetTouchPointCount() { return 0; }
Vector2 GetTouchPosition(int i) { return {0, 0}; }
Sound LoadSound(const char* f) { return {0}; }
Music LoadMusicStream(const char* f) { return {0}; }
void UnloadSound(Sound s) {}
void UnloadMusicStream(Music m) {}
void SetSoundVolume(Sound s, float v) {}
void PlaySound(Sound s) {}
void StopSound(Sound s) {}
void PlayMusicStream(Music m) {}
void StopMusicStream(Music m) {}
void PauseMusicStream(Music m) {}
void ResumeMusicStream(Music m) {}
void SetMusicVolume(Music m, float v) {}
void SetMasterVolume(float v) {}
bool IsMusicStreamPlaying(Music m) { return false; }
bool IsSoundPlaying(Sound s) { return false; }
bool WindowShouldClose() { return false; }
void SetTargetFPS(int f) {}
float GetFrameTime() { return 0.016f; }
#endif

namespace FloppyTurd {

    namespace RaylibPlatform {
        
        // Internal state
        static std::unordered_map<std::string, Sound> g_loadedSounds;
        static std::unordered_map<std::string, Music> g_loadedMusic;
        static Music g_currentMusic = {0};
        static float g_masterVolume = 1.0f;
        static float g_sfxVolume = 1.0f;
        static float g_musicVolume = 1.0f;
        
        void SetupDelegates(PlatformDelegates& delegates) {
            // Renderer delegates
            delegates.renderer.beginFrame = BeginFrame;
            delegates.renderer.endFrame = EndFrame;
            delegates.renderer.clearScreen = ClearScreen;
            delegates.renderer.drawSprite = DrawSprite;
            delegates.renderer.drawSpriteScaled = DrawSpriteScaled;
            delegates.renderer.drawSpriteScaledTinted = DrawSpriteScaledTinted;
            delegates.renderer.drawText = DrawText;
            delegates.renderer.drawRectangle = DrawRectangle;
            delegates.renderer.drawCircle = DrawCircle;
            delegates.renderer.getScreenSize = GetScreenSize;
            
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
        
        bool Initialize(int screenWidth, int screenHeight, const char* title) {
            InitWindow(screenWidth, screenHeight, title);
            InitAudioDevice();
            return true;
        }
        
        void Shutdown() {
            // Unload all sounds
            for (auto& pair : g_loadedSounds) {
                UnloadSound(pair.second);
            }
            g_loadedSounds.clear();
            
            // Unload all music
            for (auto& pair : g_loadedMusic) {
                UnloadMusicStream(pair.second);
            }
            g_loadedMusic.clear();
            
            CloseAudioDevice();
            CloseWindow();
        }
        
        // Renderer implementations
        void BeginFrame() {
            BeginDrawing();
        }
        
        void EndFrame() {
            EndDrawing();
        }
        
        void ClearScreen(float r, float g, float b, float a) {
            Color color = {(unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), (unsigned char)(a * 255)};
            ClearBackground(color);
        }
        
        // Internal sprite drawing functions (with Sprite& parameter)
        void DrawSpriteInternal(const Sprite& sprite, float x, float y, float rotation) {
            // TODO: Implement sprite drawing with Raylib
            // This requires the Sprite class to have Raylib texture integration
            Rectangle source = {0, 0, 64, 64}; // Placeholder
            Rectangle dest = {x, y, 64, 64};
            Vector2 origin = {32, 32};
            DrawTexturePro(Texture2D{0}, source, dest, origin, rotation, WHITE);
        }
        
        void DrawSpriteScaledInternal(const Sprite& sprite, float x, float y, float scaleX, float scaleY, float rotation) {
            // TODO: Implement scaled sprite drawing with Raylib
            Rectangle source = {0, 0, 64, 64}; // Placeholder
            Rectangle dest = {x, y, 64 * scaleX, 64 * scaleY};
            Vector2 origin = {32 * scaleX, 32 * scaleY};
            DrawTexturePro(Texture2D{0}, source, dest, origin, rotation, WHITE);
        }
        
        // Delegate-compatible wrapper functions (with void* parameter)
        void DrawSprite(uint32_t textureHandle, float x, float y, float rotation) {
            Sprite& spriteRef = *reinterpret_cast<Sprite*>(textureHandle);
            DrawSpriteInternal(spriteRef, x, y, rotation);
        }
        
        void DrawSpriteScaled(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation) {
            Sprite& spriteRef = *reinterpret_cast<Sprite*>(textureHandle);
            DrawSpriteScaledInternal(spriteRef, x, y, scaleX, scaleY, rotation);
        }

        void DrawSpriteScaledTinted(uint32_t textureHandle, float x, float y, float scaleX, float scaleY, float rotation, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            Sprite& spriteRef = *reinterpret_cast<Sprite*>(textureHandle);
            DrawSpriteScaledTintedInternal(spriteRef, x, y, scaleX, scaleY, rotation, r, g, b, a);
        }
        
        void DrawText(const std::string& text, float x, float y, float fontSize, float r, float g, float b, float a) {
            Color color = {(unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), (unsigned char)(a * 255)};
            ::DrawText(text, (int)x, (int)y, (int)fontSize, color);
        }
        
        void DrawRectangle(float x, float y, float width, float height, float r, float g, float b, float a) {
            Color color = {(unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), (unsigned char)(a * 255)};
            ::DrawRectangle((int)x, (int)y, (int)width, (int)height, color);
        }
        
        void DrawCircle(float x, float y, float radius, float r, float g, float b, float a) {
            Color color = {(unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), (unsigned char)(a * 255)};
            DrawCircle((int)x, (int)y, radius, color);
        }
        
        void GetScreenSize(float* width, float* height) {
            if (width) *width = (float)GetScreenWidth();
            if (height) *height = (float)GetScreenHeight();
        }
        
        // Input implementations
        bool IsActionPressed(int action) {
            // Map actions to keys/mouse buttons
            switch (action) {
                case 0: return IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE);
                default: return false;
            }
        }
        
        bool IsActionJustPressed(int action) {
            switch (action) {
                case 0: return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE);
                default: return false;
            }
        }
        
        bool IsActionReleased(int action) {
            switch (action) {
                case 0: return IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsKeyReleased(KEY_SPACE);
                default: return false;
            }
        }
        
        void GetPrimaryInputPosition(float* x, float* y) {
            Vector2 mousePos = GetMousePosition();
            if (x) *x = mousePos.x;
            if (y) *y = mousePos.y;
        }
        
        bool IsPrimaryInputDown() {
            return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        }
        
        bool IsPrimaryInputJustPressed() {
            return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        }
        
        bool IsPrimaryInputJustReleased() {
            return IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
        }
        
        int GetTouchCount() {
            return GetTouchPointCount();
        }
        
        void GetTouchPosition(int touchIndex, float* x, float* y) {
            Vector2 touchPos = ::GetTouchPosition(touchIndex);
            if (x) *x = touchPos.x;
            if (y) *y = touchPos.y;
        }
        
        bool IsKeyPressed(int keyCode) {
            return ::IsKeyDown(keyCode);
        }
        
        bool IsKeyJustPressed(int keyCode) {
            return ::IsKeyPressed(keyCode);
        }
        
        // Audio implementations
        void PlaySound(const char* soundName) {
            PlaySoundWithVolume(soundName, 1.0f);
        }
        
        void PlaySoundWithVolume(const char* soundName, float volume) {
            if (!soundName) return;
            
            std::string name(soundName);
            auto it = g_loadedSounds.find(name);
            if (it == g_loadedSounds.end()) {
                // Try to load the sound
                Sound sound = LoadSound(soundName);
                if (sound.frameCount > 0) {
                    g_loadedSounds[name] = sound;
                    it = g_loadedSounds.find(name);
                } else {
                    return; // Failed to load
                }
            }
            
            SetSoundVolume(it->second, volume * g_sfxVolume * g_masterVolume);
            ::PlaySound(it->second);
        }
        
        void StopSound(const char* soundName) {
            if (!soundName) return;
            
            std::string name(soundName);
            auto it = g_loadedSounds.find(name);
            if (it != g_loadedSounds.end()) {
                StopSound(it->second);
            }
        }
        
        void PlayMusic(const char* musicName) {
            if (!musicName) return;
            
            std::string name(musicName);
            auto it = g_loadedMusic.find(name);
            if (it == g_loadedMusic.end()) {
                // Try to load the music
                Music music = LoadMusicStream(musicName);
                if (music.frameCount > 0) {
                    g_loadedMusic[name] = music;
                    it = g_loadedMusic.find(name);
                } else {
                    return; // Failed to load
                }
            }
            
            if (IsMusicStreamPlaying(g_currentMusic)) {
                StopMusicStream(g_currentMusic);
            }
            
            g_currentMusic = it->second;
            SetMusicVolume(g_currentMusic, g_musicVolume * g_masterVolume);
            PlayMusicStream(g_currentMusic);
        }
        
        void StopMusic() {
            if (IsMusicStreamPlaying(g_currentMusic)) {
                StopMusicStream(g_currentMusic);
            }
        }
        
        void PauseMusic() {
            if (IsMusicStreamPlaying(g_currentMusic)) {
                PauseMusicStream(g_currentMusic);
            }
        }
        
        void ResumeMusic() {
            ResumeMusicStream(g_currentMusic);
        }
        
        void SetMasterVolume(float volume) {
            g_masterVolume = volume;
            SetMasterVolume(volume);
        }
        
        void SetSFXVolume(float volume) {
            g_sfxVolume = volume;
        }
        
        void SetMusicVolume(float volume) {
            g_musicVolume = volume;
            if (g_currentMusic.frameCount > 0) {
                SetMusicVolume(g_currentMusic, volume * g_masterVolume);
            }
        }
        
        bool IsMusicPlaying() {
            return IsMusicStreamPlaying(g_currentMusic);
        }
        
        bool IsSoundPlaying(const char* soundName) {
            if (!soundName) return false;
            
            std::string name(soundName);
            auto it = g_loadedSounds.find(name);
            if (it != g_loadedSounds.end()) {
                return IsSoundPlaying(it->second);
            }
            return false;
        }
        
        // Utility functions
        bool ShouldClose() {
            return WindowShouldClose();
        }
        
        void SetTargetFPS(int fps) {
            ::SetTargetFPS(fps);
        }
        
        float GetFrameTime() {
            return ::GetFrameTime();
        }
        
    } // namespace RaylibPlatform

} // namespace FloppyTurd

#endif // !PLATFORM_IOS