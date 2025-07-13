#ifndef PLATFORM_API_H
#define PLATFORM_API_H

// Forward declarations
class Game;  // Forward declaration for Game class

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <TargetConditionals.h>
#include "PlatformTypes.h"
#include "PlatformTraits.h"

// Platform detection macros
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    #ifndef PLATFORM_IOS
        #define PLATFORM_IOS
    #endif
#else
    #ifndef PLATFORM_DESKTOP
        #define PLATFORM_DESKTOP
    #endif
#endif

// ============================================================================
// PLATFORM-SPECIFIC STRUCTS
// ============================================================================

// ============================================================================
// PLATFORM-SPECIFIC STRUCTS
// ============================================================================

// All type definitions are now in PlatformTypes.h
// No duplicate definitions needed here

// ============================================================================
// CONSTANTS
// ============================================================================

// All constants are now defined in PlatformTypes.h
// This section is kept for backward compatibility but constants should be used from PlatformTypes.h

// ============================================================================
// PLATFORM API CLASS 
// ============================================================================

template <typename Traits>
class PlatformAPI {
public:
    static PlatformAPI& GetInstance() {
        static PlatformAPI instance;
        return instance;
    }
    
    // Constructor/Destructor
    PlatformAPI() {
        // No initialization needed in constructor
    }
    ~PlatformAPI() {
        // No cleanup needed in destructor
    }
    
    // Prevent copying
    PlatformAPI(const PlatformAPI&) = delete;
    PlatformAPI& operator=(const PlatformAPI&) = delete;
    
    // ============================================================================
    // INITIALIZATION AND LIFECYCLE
    // ============================================================================
    
    void Initialize(void* nativeView) {
        Traits::Initialize(nativeView);
    }
    void Initialize() {
        Traits::Initialize();
    }
    void Shutdown() {
        Traits::Shutdown();
    }
    
    // ============================================================================
    // RENDERING FUNCTIONS
    // ============================================================================
    
    void BeginDrawing() {
        Traits::BeginDrawing();
    }
    void EndDrawing() {
        Traits::EndDrawing();
    }
    void ClearBackground(Color color) {
        Traits::ClearBackground(color);
    }
    void DrawRectangle(float x, float y, float width, float height, Color color) {
        Traits::DrawRectangle(x, y, width, height, color);
    }
    void DrawRectangleRec(Rectangle rec, Color color) {
        Traits::DrawRectangleRec(rec, color);
    }

    void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) {
        Traits::DrawRectangleLinesEx(rec, lineThick, color);
    }

    void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
        Traits::DrawRectangleRounded(rec, roundness, segments, color);
    }

    void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
        Traits::DrawRectangleRoundedLines(rec, roundness, segments, lineThick, color);
    }

    void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
        Traits::DrawRectangleRoundedLinesEx(rec, roundness, segments, lineThick, color);
    }
    void DrawCircle(float centerX, float centerY, float radius, Color color) {
        Traits::DrawCircle(centerX, centerY, radius, color);
    }
    void DrawCircleV(Vector2 center, float radius, Color color) {
        Traits::DrawCircleV(center, radius, color);
    }
    void DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color) {
        Traits::DrawLine(startPosX, startPosY, endPosX, endPosY, color);
    }
    void DrawLineV(Vector2 startPos, Vector2 endPos, Color color) {
        Traits::DrawLineV(startPos, endPos, color);
    }

    void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {
        Traits::DrawLineEx(startPos, endPos, thick, color);
    }
    void DrawText(const char* text, int posX, int posY, int fontSize, Color color) {
        Traits::DrawText(text, static_cast<float>(posX), static_cast<float>(posY), static_cast<float>(fontSize), color);
    }
    void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
        Traits::DrawTextEx(font, text, position, fontSize, spacing, tint);
    }

    int MeasureText(const char* text, int fontSize) {
        return Traits::MeasureText(text, fontSize);
    }

    const char* TextFormat(const char* text, va_list args) {
        return Traits::TextFormat(text, args);
    }
    void DrawTexture(Texture2D texture, int posX, int posY, Color tint) {
        Traits::DrawTexture(texture, static_cast<float>(posX), static_cast<float>(posY), tint);
    }
    void DrawTextureV(Texture2D texture, Vector2 position, Color tint) {
        Traits::DrawTextureV(texture, position, tint);
    }
    void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) {
        Traits::DrawTextureRec(texture, source, position, tint);
    }
    void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
        Traits::DrawTexturePro(texture, source, dest, origin, rotation, tint);
    }

    void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) {
        Traits::DrawTextureEx(texture, position, rotation, scale, tint);
    }
    void DrawTexture(Texture2D texture, float x, float y, float width, float height, Color color) {
        Traits::DrawTexture(texture, x, y, width, height, color);
    }
    
    // ============================================================================
    // WINDOW AND SCREEN FUNCTIONS
    // ============================================================================
    
    void SetTargetFPS(int fps) {
        Traits::SetTargetFPS(fps);
    }
    bool WindowShouldClose() {
        return Traits::WindowShouldClose();
    }
    int GetScreenWidth() {
        return Traits::GetScreenWidth();
    }
    int GetScreenHeight() {
        return Traits::GetScreenHeight();
    }
    Vector2 GetScreenToWorld2D(Vector2 screen, Camera2D camera) {
        return Traits::GetScreenToWorld2D(screen, camera);
    }
    Vector2 GetWorldToScreen2D(Vector2 world, Camera2D camera) {
        return Traits::GetWorldToScreen2D(world, camera);
    }
    float GetScreenScale() {
        return Traits::GetScreenScale();
    }
    void SetScreenSize(int width, int height) {
        Traits::SetScreenSize(width, height);
    }
    void SetScreenScale(float scale) {
        Traits::SetScreenScale(scale);
    }
    
    // ============================================================================
    // INPUT FUNCTIONS
    // ============================================================================
    
    bool IsKeyPressed(int key) {
        return Traits::IsKeyPressed(key);
    }
    bool IsKeyDown(int key) {
        return Traits::IsKeyDown(key);
    }
    bool IsKeyReleased(int key) {
        return Traits::IsKeyReleased(key);
    }
    bool IsMouseButtonPressed(int button) {
        return Traits::IsMouseButtonPressed(button);
    }
    bool IsMouseButtonDown(int button) {
        return Traits::IsMouseButtonDown(button);
    }
    bool IsMouseButtonReleased(int button) {
        return Traits::IsMouseButtonReleased(button);
    }
    Vector2 GetMousePosition() {
        return Traits::GetMousePosition();
    }
    Vector2 GetMouseDelta() {
        return Traits::GetMouseDelta();
    }
    Vector2 GetTouchPosition(int index) {
        return Traits::GetTouchPosition(index);
    }
    bool IsPrimaryInputPressed() {
        return Traits::IsPrimaryInputPressed();
    }
    bool IsPrimaryInputReleased() {
        return Traits::IsPrimaryInputReleased();
    }
    
    // ============================================================================
    // TEXTURE FUNCTIONS
    // ============================================================================
    
    Texture2D LoadTexture(const char* fileName) {
        return Traits::LoadTexture(fileName);
    }
    void UnloadTexture(Texture2D texture) {
        Traits::UnloadTexture(texture);
    }

    Image LoadImage(const char* fileName) {
        return Traits::LoadImage(fileName);
    }

    void UnloadImage(Image image) {
        Traits::UnloadImage(image);
    }

    void SetTextureWrap(Texture2D texture, int wrap) {
        Traits::SetTextureWrap(texture, wrap);
    }

    void* CreateTextureFromImage(void* image, int* width, int* height) {
        return Traits::CreateTextureFromImage(image, width, height);
    }


    Texture2D LoadTextureFromImage(Image image) {
        return Traits::LoadTextureFromImage(image);
    }
    Image LoadImageFromTexture(Texture2D texture) {
        return Traits::LoadImageFromTexture(texture);
    }

    Rectangle GetTextureRec(Texture2D texture) {
        return Traits::GetTextureRec(texture);
    }
    
    // ============================================================================
    // FONT FUNCTIONS
    // ============================================================================
    
    Font LoadFont(const char* fileName) {
        return Traits::LoadFont(fileName);
    }
    Font LoadFontEx(const char* fileName, int fontSize, int* fontChars, int glyphCount) {
        return Traits::LoadFontEx(fileName, fontSize, fontChars, glyphCount);
    }
    void UnloadFont(Font font) {
        Traits::UnloadFont(font);
    }
    void DrawFPS(int posX, int posY) {
        Traits::DrawFPS(posX, posY);
    }

    void BeginScissorMode(int x, int y, int width, int height) {
        Traits::BeginScissorMode(x, y, width, height);
    }

    void EndScissorMode() {
        Traits::EndScissorMode();
    }
    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
        return Traits::MeasureTextEx(font, text, fontSize, spacing);
    }
    
    // ============================================================================
    // AUDIO FUNCTIONS
    // ============================================================================
    
    void InitAudioDevice() {
        Traits::InitAudioDevice();
    }

    void InitializeAudio() {
        Traits::InitializeAudio();
    }
    void CloseAudioDevice() {
        Traits::CloseAudioDevice();
    }

    void ShutdownAudio() {
        Traits::ShutdownAudio();
    }
    bool IsAudioDeviceReady() {
        return Traits::IsAudioDeviceReady();
    }
    void SetMasterVolume(float volume) {
        Traits::SetMasterVolume(volume);
    }
    Sound LoadSound(const char* fileName) {
        return Traits::LoadSound(fileName);
    }
    void UnloadSound(Sound sound) {
        Traits::UnloadSound(sound);
    }
    void PlaySound(Sound sound) {
        Traits::PlaySound(sound);
    }
    void StopSound(Sound sound) {
        Traits::StopSound(sound);
    }
    void PauseSound(Sound sound) {
        Traits::PauseSound(sound);
    }
    void ResumeSound(Sound sound) {
        Traits::ResumeSound(sound);
    }
    bool IsSoundPlaying(Sound sound) {
        return Traits::IsSoundPlaying(sound);
    }
    void SetSoundVolume(Sound sound, float volume) {
        Traits::SetSoundVolume(sound, volume);
    }
    void SetSoundPitch(Sound sound, float pitch) {
        Traits::SetSoundPitch(sound, pitch);
    }
    void SetSoundPan(Sound sound, float pan) {
        Traits::SetSoundPan(sound, pan);
    }
    Music LoadMusicStream(const char* fileName) {
        return Traits::LoadMusicStream(fileName);
    }

    Music LoadMusic(const char* fileName) {
        return Traits::LoadMusic(fileName);
    }
    void UnloadMusicStream(Music music) {
        Traits::UnloadMusicStream(music);
    }

    void UnloadMusic(Music music) {
        Traits::UnloadMusic(music);
    }
    void PlayMusicStream(Music music) {
        Traits::PlayMusicStream(music);
    }

    void PlayMusic(Music music) {
        Traits::PlayMusic(music);
    }
    bool IsMusicStreamPlaying(Music music) {
        return Traits::IsMusicStreamPlaying(music);
    }

    bool IsMusicPlaying(Music music) {
        return Traits::IsMusicPlaying(music);
    }
    void UpdateMusicStream(Music music) {
        Traits::UpdateMusicStream(music);
    }
    void UpdateMusic(Music music) {
        Traits::UpdateMusic(music);
    }
    void StopMusicStream(Music music) {
        Traits::StopMusicStream(music);
    }

    void StopMusic(Music music) {
        Traits::StopMusic(music);
    }
    void PauseMusicStream(Music music) {
        Traits::PauseMusicStream(music);
    }

    void PauseMusic(Music music) {
        Traits::PauseMusic(music);
    }
    void ResumeMusicStream(Music music) {
        Traits::ResumeMusicStream(music);
    }

    void ResumeMusic(Music music) {
        Traits::ResumeMusic(music);
    }
    void SeekMusicStream(Music music, float position) {
        Traits::SeekMusicStream(music, position);
    }
    void SetMusicVolume(Music music, float volume) {
        Traits::SetMusicVolume(music, volume);
    }
    void SetMusicPitch(Music music, float pitch) {
        Traits::SetMusicPitch(music, pitch);
    }
    void SetMusicPan(Music music, float pan) {
        Traits::SetMusicPan(music, pan);
    }
    float GetMusicTimeLength(Music music) {
        return Traits::GetMusicTimeLength(music);
    }
    float GetMusicTimePlayed(Music music) {
        return Traits::GetMusicTimePlayed(music);
    }
    void PlayMusicLoop(Music music) {
        Traits::PlayMusicLoop(music);
    }
    void SetLooping(Music music, bool looping) {
        Traits::SetLooping(music, looping);
    }

    void SetMusicLooping(Music music, bool looping) {
        Traits::SetMusicLooping(music, looping);
    }
    float GetMusicDuration(Music music) {
        return Traits::GetMusicDuration(music);
    }
    
    // ============================================================================
    // RENDER TEXTURE FUNCTIONS
    // ============================================================================
    
    RenderTexture2D LoadRenderTexture(int width, int height) {
        return Traits::LoadRenderTexture(width, height);
    }
    void UnloadRenderTexture(RenderTexture2D target) {
        Traits::UnloadRenderTexture(target);
    }
    void BeginTextureMode(RenderTexture2D target) {
        Traits::BeginTextureMode(target);
    }
    void EndTextureMode() {
        Traits::EndTextureMode();
    }
    
    // ============================================================================
    // TIME FUNCTIONS
    // ============================================================================
    
    double GetTime() {
        return Traits::GetTime();
    }
    float GetFrameTime() {
        return Traits::GetFrameTime();
    }
    int GetCurrentFPS() {
        return Traits::GetCurrentFPS();
    }
    float GetCurrentFrameTime() {
        return Traits::GetCurrentFrameTime();
    }
    
    // ============================================================================
    // UTILITY FUNCTIONS
    // ============================================================================
    
    void TraceLog(int logLevel, const char* text, ...) {
        Traits::TraceLog(logLevel, text);
    }
    void SetTraceLogLevel(int logLevel) {
        Traits::SetTraceLogLevel(logLevel);
    }
    void SetConfigFlags(unsigned int flags) {
        Traits::SetConfigFlags(flags);
    }
    void InitWindow(int width, int height, const char* title) {
        Traits::InitWindow(width, height, title);
    }

    void SetWindowSize(int width, int height) {
        Traits::SetWindowSize(width, height);
    }

    void ToggleFullscreen() {
        Traits::ToggleFullscreen();
    }
    void CloseWindow() {
        Traits::CloseWindow();
    }
    void SetRandomSeed(unsigned int seed) {
        Traits::SetRandomSeed(seed);
    }
    int GetRandomValue(int min, int max) {
        return Traits::GetRandomValue(min, max);
    }
    float GetRandomFloat(float min, float max) {
        return Traits::GetRandomFloat(min, max);
    }
    Vector2 GetRandomVector2(Vector2 min, Vector2 max) {
        return Traits::GetRandomVector2(min, max);
    }
    Color GetRandomColor() {
        return Traits::GetRandomColor();
    }
    Color ColorAlpha(Color color, float alpha) {
        return Traits::ColorAlpha(color, alpha);
    }
    Color ColorAlphaBlend(Color dst, Color src, Color tint) {
        return Traits::ColorAlphaBlend(dst, src, tint);
    }
    Color ColorLerp(Color color1, Color color2, float amount) {
        return Traits::ColorLerp(color1, color2, amount);
    }
    Color Fade(Color color, float alpha) {
        return Traits::Fade(color, alpha);
    }
    float Pow(float base, float exponent) {
        return Traits::Pow(base, exponent);
    }


    Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
        return Traits::Vector2Add(v1, v2);
    }

    Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
        return Traits::Vector2Subtract(v1, v2);
    }

    float Vector2Length(Vector2 v) {
        return Traits::Vector2Length(v);
    }


    float Vector2Distance(Vector2 v1, Vector2 v2) {
        return Traits::Vector2Distance(v1, v2);
    }


    Vector2 Vector2Scale(Vector2 v, float scale) {
        return Traits::Vector2Scale(v, scale);
    }



    Vector2 Vector2Normalize(Vector2 v) {
        return Traits::Vector2Normalize(v);
    }





    Rectangle RectangleNew(float x, float y, float width, float height) {
        return Traits::RectangleNew(x, y, width, height);
    }
    Rectangle RectangleFromVector2(Vector2 position, Vector2 size) {
        return Traits::RectangleFromVector2(position, size);
    }




    bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
        return Traits::CheckCollisionRecs(rec1, rec2);
    }

    bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) {
        return Traits::CheckCollisionCircleRec(center, radius, rec);
    }
    bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
        return Traits::CheckCollisionPointRec(point, rec);
    }




    
    // ============================================================================
    // COMPLEX AUDIO FUNCTIONS
    // ============================================================================
    
    static void StartCrossfade(float duration) {
        Traits::StartCrossfade(duration);
    }
    
    static void UpdateCrossfade(float deltaTime) {
        Traits::UpdateCrossfade(deltaTime);
    }
    
    static void FadeOutMusic(float duration) {
        Traits::FadeOutMusic(duration);
    }
    
    static void FadeInMusic(float duration) {
        Traits::FadeInMusic(duration);
    }
    
    static void UpdateFade(float deltaTime) {
        Traits::UpdateFade(deltaTime);
    }

    // ============================================================================
    // COMPLEX IMAGE FUNCTIONS
    // ============================================================================
    
    

    // ============================================================================
    // PLATFORM-SPECIFIC FUNCTIONS
    // ============================================================================
    
    static void SetOrientation(bool landscape) {
        Traits::SetOrientation(landscape);
    }
    
    static void ShowVirtualKeyboard(bool show) {
        Traits::ShowVirtualKeyboard(show);
    }
    
    static void Vibrate(int milliseconds) {
        Traits::Vibrate(milliseconds);
    }
    
    // UpdateSafeAreaInsets is handled by PlatformTraits directly

    // ============================================================================
    // UTILITY FUNCTIONS
    // ============================================================================
    
    static std::string GetResourcePath(const char* resourceName) {
        return Traits::GetResourcePath(resourceName);
    }
    
    static bool PreferLowPowerMode() {
        return Traits::PreferLowPowerMode();
    }
    
    static int GetRecommendedTextureSize() {
        return Traits::GetRecommendedTextureSize();
    }
    
    static bool IsMobilePlatform() {
        return Traits::IsMobilePlatform();
    }

    // ============================================================================
    // RENDERING FUNCTIONS
    // ============================================================================
    
};

using CurrentPlatformAPI = PlatformAPI<CurrentTraits>;

// ============================================================================
// STANDALONE FUNCTION DECLARATIONS
// ============================================================================

// Window and Screen Functions
void InitWindow(int width, int height, const char* title);
void SetWindowSize(int width, int height);
void ToggleFullscreen();
void CloseWindow();
int GetScreenWidth();
int GetScreenHeight();
float GetScreenScale();
bool WindowShouldClose();

// Rendering Functions
void BeginDrawing();
void EndDrawing();
void ClearBackground(Color color);
void DrawRectangle(float x, float y, float width, float height, Color color);
void DrawRectangleRec(Rectangle rec, Color color);
void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color);
void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);
void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color);
void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color);
void DrawCircle(float centerX, float centerY, float radius, Color color);
void DrawCircleV(Vector2 center, float radius, Color color);
void DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color);
void DrawLineV(Vector2 startPos, Vector2 endPos, Color color);
void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);
void DrawTexture(Texture2D texture, float x, float y, Color tint);
void DrawTextureV(Texture2D texture, Vector2 position, Color tint);
void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
void DrawText(const char* text, float x, float y, float fontSize, Color color);
void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint);
int MeasureText(const char* text, int fontSize);
const char* TextFormat(const char* text, va_list args);

// Variadic version for compatibility with raylib-style usage
static const char* TextFormat(const char* text, ...);

// Input Functions
bool IsKeyPressed(int key);
bool IsKeyDown(int key);
bool IsKeyReleased(int key);
bool IsMouseButtonDown(int button);
bool IsMouseButtonPressed(int button);
bool IsMouseButtonReleased(int button);
Vector2 GetMousePosition();
Vector2 GetMouseDelta();
Vector2 GetTouchPosition(int index);
bool IsPrimaryInputPressed();
bool IsPrimaryInputDown();
bool IsPrimaryInputReleased();
Vector2 GetPrimaryInputPosition();

// Texture Functions
Texture2D LoadTexture(const char* fileName);
void UnloadTexture(Texture2D texture);
Image LoadImage(const char* fileName);
void UnloadImage(Image image);
void SetTextureWrap(Texture2D texture, int wrap);
Texture2D LoadTextureFromImage(Image image);
Image LoadImageFromTexture(Texture2D texture);
Rectangle GetTextureRec(Texture2D texture);

// Font Functions
Font LoadFont(const char* fileName);
Font LoadFontEx(const char* fileName, int fontSize, int* fontChars, int glyphCount);
void UnloadFont(Font font);
Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing);

// Rendering Functions
void BeginScissorMode(int x, int y, int width, int height);
void EndScissorMode();
void DrawFPS(int posX, int posY);

// Audio Functions
void InitAudioDevice();
void InitializeAudio();
void CloseAudioDevice();
void ShutdownAudio();
bool IsAudioDeviceReady();
Sound LoadSound(const char* fileName);
void UnloadSound(Sound sound);
void PlaySound(Sound sound);
void StopSound(Sound sound);
void PauseSound(Sound sound);
void ResumeSound(Sound sound);
void SetSoundVolume(Sound sound, float volume);
bool IsSoundPlaying(Sound sound);
Music LoadMusicStream(const char* fileName);
Music LoadMusic(const char* fileName);
void UnloadMusicStream(Music music);
void UnloadMusic(Music music);
void PlayMusicStream(Music music);
void PlayMusic(Music music);
void StopMusicStream(Music music);
void StopMusic(Music music);
void PauseMusicStream(Music music);
void PauseMusic(Music music);
void ResumeMusicStream(Music music);
void ResumeMusic(Music music);
void UpdateMusicStream(Music music);
void UpdateMusic(Music music);
void SetMusicVolume(Music music, float volume);
bool IsMusicStreamPlaying(Music music);
bool IsMusicPlaying(Music music);
void SetMusicLooping(Music music, bool looping);
float GetMusicDuration(Music music);

// Utility Functions
double GetTime();
void TraceLog(int logLevel, const char* text, ...);
int GetRandomValue(int min, int max);
float GetRandomFloat(float min, float max);
void SetTraceLogLevel(int logLevel);
void SetConfigFlags(unsigned int flags);
void SetRandomSeed(unsigned int seed);
Vector2 GetRandomVector2(Vector2 min, Vector2 max);
Color GetRandomColor();
Color ColorAlphaBlend(Color dst, Color src, Color tint);
Color ColorLerp(Color color1, Color color2, float amount);

// Vector Math Functions
float Vector2Length(Vector2 v);
Vector2 Vector2Normalize(Vector2 v);
Vector2 Vector2Add(Vector2 v1, Vector2 v2);
Vector2 Vector2Subtract(Vector2 v1, Vector2 v2);
Vector2 Vector2Scale(Vector2 v, float scale);
float Vector2Distance(Vector2 v1, Vector2 v2);

// Collision Detection Functions
bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2);
bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec);
bool CheckCollisionPointRec(Vector2 point, Rectangle rec);

// Color Functions
Color ColorAlpha(Color color, float alpha);
Color ColorLerp(Color color1, Color color2, float amount);
Color Fade(Color color, float alpha);

// Math Utility Functions
float Clamp(float value, float min, float max);
float Lerp(float start, float end, float amount);
float Min(float a, float b);
float Max(float a, float b);
float Abs(float value);
float Sin(float angle);
float Cos(float angle);
float Atan2(float y, float x);
float Sqrt(float value);
float Pow(float base, float exponent);

// Time and Performance Functions
float GetFrameTime();
int GetCurrentFPS();
float GetCurrentFrameTime();

// Rectangle Utility Functions
Rectangle RectangleNew(float x, float y, float width, float height);
Rectangle RectangleFromVector2(Vector2 position, Vector2 size);

// Platform-Specific Utility Functions
std::string GetResourcePath(const char* resourceName);
bool PreferLowPowerMode();
int GetRecommendedTextureSize();
bool IsMobilePlatform();

// Initialization and Lifecycle Functions
void Initialize(void* nativeView);
void Initialize();
void Shutdown();

// Complex Audio Functions
void StartCrossfade(float duration);
void UpdateCrossfade(float deltaTime);
void FadeOutMusic(float duration);
void FadeInMusic(float duration);
void UpdateFade(float deltaTime);

// Complex Image Functions
void ImageResize(Image* image, int newWidth, int newHeight);
void ImageDraw(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);
Image ImageCopy(Image image);
Image ImageFromImage(Image image, Rectangle rec);
void ImageFlipVertical(Image* image);
void ImageFlipHorizontal(Image* image);

// Platform-Specific Functions
void SetOrientation(bool landscape);
void ShowVirtualKeyboard(bool show);
void Vibrate(int milliseconds);
void UpdateSafeAreaInsets(float top, float right, float bottom, float left);

// Render Texture Functions
RenderTexture2D LoadRenderTexture(int width, int height);
void UnloadRenderTexture(RenderTexture2D target);
void BeginTextureMode(RenderTexture2D target);
void EndTextureMode();

// Additional functions
void SetTargetFPS(int fps);
void SetScreenSize(int width, int height);
void SetScreenScale(float scale);
Vector2 GetScreenCenter();
Vector2 GetRenderScale();
Rectangle GetSafeArea();
float GetScreenDensity();
bool IsLandscape();
bool IsPortrait();
void SetPreferredOrientation(bool landscape);
bool ShouldUseLargerTouchTargets();
int GetRecommendedFontSize();

// Additional functions that might be missing
bool IsWindowFullscreen();
void SetWindowFocused();
void SetExitKey(int key);
int GetCurrentMonitor();
int GetMonitorWidth(int monitor);
int GetMonitorHeight(int monitor);
void SetWindowPosition(int x, int y);
void* CreateTextureFromImage(void* image, int* width, int* height);

// ============================================================================
// CONSTANTS (For backward compatibility)
// ============================================================================

#define TEXTURE_WRAP_CLAMP 1
#define TEXTURE_FILTER_POINT 0
#define TEXTURE_FILTER_BILINEAR 1
#define MOUSE_LEFT_BUTTON 0
#define KEY_E 69
#define KEY_F11 290
#define KEY_ESCAPE 256

void SetTextureFilter(Texture2D texture, int filter);
Image GenImageColor(int width, int height, Color color);
Font GetFontDefault();

// ============================================================================
// GLOBAL GAME INSTANCE MANAGEMENT (for iOS integration)
// ============================================================================
// These functions allow iOS Objective-C++ code to access the C++ Game instance

extern "C" {
    // Get the current game instance (returns nullptr if not set)
    Game* GetGameInstance();
    
    // Set the current game instance (call from Game constructor)
    void SetGameInstance(Game* game);
    
    // App lifecycle functions
    void OnAppPause();
    void OnAppResume();
    
    // Global game view management for iOS
    void SetGlobalGameView(void* gameView);
    void* GetGlobalGameView();
    
    // Game main function
    int game_main(int argc, char* argv[]);
}

#endif // PLATFORM_API_H 