//
//  PlatformAPI.h
//  FloppyTurd - Modernized Platform API with Native Swift C++ Interop
//
//  This version eliminates legacy C-style bridge functions and uses proper native Swift C++ interop
//

#ifndef PLATFORM_API_MODERN_H
#define PLATFORM_API_MODERN_H

#include <cstdarg>
#include <cstdio>
#include <memory>
#include "PlatformTypes.h"

// ============================================================================
// PLATFORM DETECTION
// ============================================================================

#if defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IOS
        #define PLATFORM_IOS 1
    #else
        #define PLATFORM_DESKTOP 1
    #endif
#else
    #define PLATFORM_DESKTOP 1
#endif

// ============================================================================
// PLATFORM-SPECIFIC INCLUDES
// ============================================================================

#if defined(PLATFORM_IOS)
    // iOS implementation using native Swift interop
    #include <CoreGraphics/CoreGraphics.h>
    #include "GameEngine-Swift.h"
    
    // Swift types are accessed via FloppyTurd:: namespace from GameEngine-Swift.h
    
    // Include the auto-generated Swift-to-C++ interface
    #include "GameEngine-Swift.h"
    
    // Use Swift types with explicit namespace qualification
    // No using declarations to avoid conflicts with PlatformTypes.h
    
    // Forward declaration to avoid circular dependency
    class Game;
    
#else
    // Desktop implementation requires raylib installation
    #error "Desktop build requires raylib - install via 'brew install raylib' and verify include paths"
#endif

// ============================================================================
// MODERNIZED PLATFORM API CLASS
// ============================================================================

class PlatformAPI {
public:
    // Singleton pattern
    static PlatformAPI& GetInstance() {
        static PlatformAPI instance;
        return instance;
    }
    
    // Delete copy constructor and assignment operator
    PlatformAPI(const PlatformAPI&) = delete;
    PlatformAPI& operator=(const PlatformAPI&) = delete;
    
private:
    PlatformAPI() = default;
    ~PlatformAPI() = default;
    
public:

#if defined(PLATFORM_IOS)
    // ============================================================================
    // iOS IMPLEMENTATIONS (using direct Swift interop)
    // ============================================================================
    
    // Platform Functions - Direct Swift calls
    void Initialize(void* nativeView = nullptr) {
        // Get screen dimensions and safe area for InputEngine initialization
        // Using fallback values since Swift Rectangle fields are not accessible from C++
        float screenWidth = 1080.0f;  // iPhone 16 simulator pixel width
        float screenHeight = 1920.0f; // iPhone 16 simulator pixel height
        float safeAreaTop = 0.0f;
        float safeAreaLeft = 0.0f;
        float safeAreaBottom = 0.0f;
        float safeAreaRight = 0.0f;
        
        FloppyTurd::InputEngineCppBridge::initialize(
            screenWidth,
            screenHeight,
            safeAreaTop,
            safeAreaLeft,
            safeAreaBottom,
            safeAreaRight
        );
    }
    
    void Shutdown() {
        // Swift cleanup if needed
    }
    
    void InitializePlatform() {
        // Platform-specific initialization
    }
    
    void ShutdownPlatform() {
        // Platform-specific cleanup
    }
    
    // Window and Screen Functions - Direct Swift calls
    void InitWindow(int width, int height, const char* title) {
        // iOS doesn't need window initialization
    }
    
    void CloseWindow() {
        // iOS doesn't need window closing
    }
    
    bool WindowShouldClose() {
        return false; // iOS apps don't close windows
    }
    
    int GetScreenWidth() {
        // Fallback since Swift Rectangle fields are not accessible from C++
        return 1080; // iPhone 16 simulator pixel width
    }
    
    int GetScreenHeight() {
        // Fallback since Swift Rectangle fields are not accessible from C++
        return 1920; // iPhone 16 simulator pixel height
    }
    
    float GetScreenScale() {
        // Calculate screen scale from pixel/point ratio
        // iPhone 16 simulator: 1080x1920 pixels, 375x667 points = ~2.88 scale
        return 2.88f;
    }
    
    void SetTargetFPS(int fps) {
        FloppyTurd::MetalRendererSwift::setTargetFPS(static_cast<int32_t>(fps));
    }
    
    int GetCurrentFPS() {
        return static_cast<int>(FloppyTurd::MetalRendererSwift::getCurrentFPS());
    }
    
    float GetCurrentFrameTime() {
        return FloppyTurd::MetalRendererSwift::getFrameTime();
    }
    
    // Input Functions - Direct Swift calls
    bool IsKeyPressed(int key) {
        // iOS doesn't have keyboard by default
        return false;
    }
    
    bool IsKeyDown(int key) {
        return false;
    }
    
    bool IsKeyReleased(int key) {
        return false;
    }
    
    bool IsMouseButtonPressed(int button) {
        return FloppyTurd::InputEngineCppBridge::isMouseButtonPressed(static_cast<int32_t>(button));
    }
    
    bool IsMouseButtonDown(int button) {
        return FloppyTurd::InputEngineCppBridge::isMouseButtonDown(static_cast<int32_t>(button));
    }
    
    bool IsMouseButtonReleased(int button) {
        return FloppyTurd::InputEngineCppBridge::isMouseButtonReleased(static_cast<int32_t>(button));
    }
    
    Vector2 GetMousePosition() {
        auto swiftVec = FloppyTurd::InputEngineCppBridge::getMousePosition();
        return {swiftVec.getX(), swiftVec.getY()};
    }
    
    Vector2 GetMouseDelta() {
        auto swiftVec = FloppyTurd::InputEngineCppBridge::getMouseDelta();
        return {swiftVec.getX(), swiftVec.getY()};
    }
    
    Vector2 GetTouchPosition(int index) {
        auto swiftVec = FloppyTurd::InputEngineCppBridge::getTouchPosition(static_cast<int32_t>(index));
        return {swiftVec.getX(), swiftVec.getY()};
    }
    
    int GetTouchCount() {
        return static_cast<int>(FloppyTurd::InputEngineCppBridge::getTouchCount());
    }
    
    bool IsPrimaryInputPressed() {
        return IsMouseButtonPressed(0); // Touch is like left mouse button
    }
    
    bool IsPrimaryInputDown() {
        return IsMouseButtonDown(0);
    }
    
    bool IsPrimaryInputReleased() {
        return IsMouseButtonReleased(0);
    }
    
    Vector2 GetPrimaryInputPosition() {
        return GetMousePosition();
    }
    
    // Rendering Functions - Direct Swift calls
    void BeginDrawing() {
        FloppyTurd::MetalRendererSwift::beginDrawing();
    }
    
    void EndDrawing() {
        FloppyTurd::MetalRendererSwift::endDrawing();
    }
    
    void ClearBackground(Color color) {
        FloppyTurd::MetalRendererSwift::clearBackground(color.r, color.g, color.b, color.a);
    }
    
    void DrawRectangle(int posX, int posY, int width, int height, Color color) {
        FloppyTurd::MetalRendererSwift::drawRectangle(
            static_cast<float>(posX), static_cast<float>(posY), 
            static_cast<float>(width), static_cast<float>(height),
            color.r, color.g, color.b, color.a
        );
    }
    
    void DrawRectangleRec(Rectangle rec, Color color) {
        FloppyTurd::MetalRendererSwift::drawRectangle(
            rec.x, rec.y, rec.width, rec.height,
            color.r, color.g, color.b, color.a
        );
    }
    
    void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) {
        // Note: drawRectangleLines function may not exist, using basic rectangle outline
        // This is a placeholder - may need to implement proper line drawing
        FloppyTurd::MetalRendererSwift::drawRectangle(
            rec.x, rec.y, rec.width, lineThick, color.r, color.g, color.b, color.a
        ); // Top
        FloppyTurd::MetalRendererSwift::drawRectangle(
            rec.x, rec.y, lineThick, rec.height, color.r, color.g, color.b, color.a
        ); // Left
        FloppyTurd::MetalRendererSwift::drawRectangle(
            rec.x + rec.width - lineThick, rec.y, lineThick, rec.height, color.r, color.g, color.b, color.a
        ); // Right
        FloppyTurd::MetalRendererSwift::drawRectangle(
            rec.x, rec.y + rec.height - lineThick, rec.width, lineThick, color.r, color.g, color.b, color.a
        ); // Bottom
    }
    
    void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
        // Note: drawRectangleRounded is not exposed to C++, using regular rectangle as fallback
        DrawRectangleRec(rec, color);
    }
    
    void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
        // Note: drawRectangleRoundedLines is not exposed to C++, using regular rectangle lines as fallback
        DrawRectangleLinesEx(rec, lineThick, color);
    }
    
    void DrawCircle(int centerX, int centerY, float radius, Color color) {
        FloppyTurd::MetalRendererSwift::drawCircle(
            static_cast<float>(centerX), static_cast<float>(centerY), radius,
            color.r, color.g, color.b, color.a
        );
    }
    
    void DrawCircleV(Vector2 center, float radius, Color color) {
        FloppyTurd::MetalRendererSwift::drawCircle(
            center.x, center.y, radius,
            color.r, color.g, color.b, color.a
        );
    }
    
    void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) {
        FloppyTurd::MetalRendererSwift::drawLine(
            static_cast<float>(startPosX), static_cast<float>(startPosY),
            static_cast<float>(endPosX), static_cast<float>(endPosY),
            color.r, color.g, color.b, color.a
        );
    }
    
    void DrawLineV(Vector2 startPos, Vector2 endPos, Color color) {
        FloppyTurd::MetalRendererSwift::drawLine(
            startPos.x, startPos.y, endPos.x, endPos.y,
            color.r, color.g, color.b, color.a
        );
    }
    
    void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {
        // Note: drawLineEx may not have the exact signature, using basic line as fallback
        DrawLineV(startPos, endPos, color);
    }
    
    void DrawText(const char* text, int posX, int posY, int fontSize, Color color) {
        FloppyTurd::MetalTextRendererCppBridge::drawText(text, static_cast<float>(posX), static_cast<float>(posY), static_cast<float>(fontSize), color.r, color.g, color.b, color.a);
    }
    
    void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
        // Fallback to basic DrawText since drawTextEx is not exposed
        DrawText(text, static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(fontSize), tint);
    }
    
    void BeginScissorMode(int x, int y, int width, int height) {
        // Create Rectangle with proper const reference
        FloppyTurd::Rectangle scissorRect = FloppyTurd::Rectangle::init(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
        FloppyTurd::MetalRendererSwift::beginScissorMode(scissorRect);
    }
    
    void EndScissorMode() {
        FloppyTurd::MetalRendererSwift::endScissorMode();
    }
    
    void DrawFPS(int posX, int posY) {
        int fps = GetCurrentFPS();
        char fpsText[16];
        snprintf(fpsText, sizeof(fpsText), "FPS: %d", fps);
        DrawText(fpsText, posX, posY, 20, {0, 255, 0, 255});
    }
    
    // Texture Functions - Using CppBridge
    Texture2D LoadTexture(const char* fileName) {
        int32_t textureId = FloppyTurd::MetalTextureCppBridge::loadTexture(fileName);
        return Texture2D{static_cast<uint32_t>(textureId), 64, 64, 1, 7}; // Default 64x64 RGBA texture
    }
    
    void UnloadTexture(Texture2D texture) {
        FloppyTurd::MetalTextureCppBridge::unloadTexture(static_cast<int32_t>(texture.id));
    }
    
    Image LoadImage(const char* fileName) {
        // Fallback image loading
        return Image{nullptr, 64, 64, 1, 7}; // Default 64x64 RGBA image
    }
    
    void UnloadImage(Image image) {
        // Image cleanup handled by Swift implementation
    }
    
    void SetTextureWrap(Texture2D texture, int wrap) {
        // Texture wrap setting handled by Swift implementation
    }
    
    Texture2D LoadTextureFromImage(Image image) {
        // Fallback texture from image
        return Texture2D{1, static_cast<int>(image.width), static_cast<int>(image.height), 1, 7};
    }
    
    void DrawTexture(Texture2D texture, int posX, int posY, Color tint) {
        // Fallback texture drawing - use rectangle drawing as placeholder
        DrawRectangle(posX, posY, texture.width, texture.height, tint);
    }
    
    void DrawTextureV(Texture2D texture, Vector2 position, Color tint) {
        // Fallback texture drawing
        DrawRectangle(static_cast<int>(position.x), static_cast<int>(position.y), texture.width, texture.height, tint);
    }
    
    void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) {
        // Fallback texture drawing
        DrawRectangle(static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(source.width), static_cast<int>(source.height), tint);
    }
    
    void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
        // Fallback texture drawing
        DrawRectangle(static_cast<int>(dest.x), static_cast<int>(dest.y), static_cast<int>(dest.width), static_cast<int>(dest.height), tint);
    }
    
    void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) {
        // Fallback texture drawing
        int scaledWidth = static_cast<int>(texture.width * scale);
        int scaledHeight = static_cast<int>(texture.height * scale);
        DrawRectangle(static_cast<int>(position.x), static_cast<int>(position.y), scaledWidth, scaledHeight, tint);
    }
    
    void SetTextureFilter(Texture2D texture, int filter) {
        // Texture filter setting handled by Swift implementation
    }
    
    // Font Functions - Using CppBridge
    Font LoadFont(const char* fileName) {
        int32_t fontId = FloppyTurd::MetalTextRendererCppBridge::loadFont(fileName, 16.0f);
        Texture2D emptyTexture = {0, 0, 0, 1, 0};
        return Font{16, 0, 0, emptyTexture, nullptr, nullptr};
    }
    
    Font LoadFontEx(const char* fileName, int fontSize, int* fontChars, int glyphCount) {
        int32_t fontId = FloppyTurd::MetalTextRendererCppBridge::loadFont(fileName, static_cast<float>(fontSize));
        Texture2D emptyTexture = {0, 0, 0, 1, 0};
        return Font{fontSize, glyphCount, 0, emptyTexture, nullptr, nullptr};
    }
    
    void UnloadFont(Font font) {
        FloppyTurd::MetalTextRendererCppBridge::unloadFont(static_cast<int32_t>(font.baseSize));
    }
    
    int MeasureText(const char* text, int fontSize) {
        auto swiftSize = FloppyTurd::MetalTextRendererCppBridge::measureText(text, static_cast<float>(fontSize));
        return static_cast<int>(swiftSize.getX());
    }
    
    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
        auto swiftSize = FloppyTurd::MetalTextRendererCppBridge::measureText(text, fontSize);
        return {swiftSize.getX(), swiftSize.getY()};
    }
    
    Font GetFontDefault() {
        Texture2D emptyTexture = {0, 0, 0, 1, 0};
        return Font{16, 0, 0, emptyTexture, nullptr, nullptr};
    }
    
    // Audio Functions - Direct Swift calls
    void InitAudioDevice() {
        // Audio initialization handled by AudioManagerSwift
    }
    
    void CloseAudioDevice() {
        // Audio cleanup handled by AudioManagerSwift
    }
    
    bool IsAudioDeviceReady() {
        return true; // AudioManagerSwift handles device readiness
    }
    
    void SetMasterVolume(float volume) {
        FloppyTurd::AudioManagerCppBridge::setMasterVolume(volume);
    }
    
    Sound LoadSound(const char* fileName) {
        int32_t soundId = FloppyTurd::AudioEngineCppBridge::loadSound(fileName);
        return Sound{soundId, nullptr, 44100};
    }
    
    void PlaySound(Sound sound) {
        FloppyTurd::AudioEngineCppBridge::playSound(sound.id);
    }

    void StopSound(Sound sound) {
        FloppyTurd::AudioEngineCppBridge::stopSound(sound.id);
    }

    void PauseSound(Sound sound) {
        FloppyTurd::AudioEngineCppBridge::pauseSound(sound.id);
    }

    void ResumeSound(Sound sound) {
        FloppyTurd::AudioEngineCppBridge::resumeSound(sound.id);
    }

    void SetSoundVolume(Sound sound, float volume) {
        FloppyTurd::AudioEngineCppBridge::setSoundVolume(sound.id, volume);
    }

    void SetSoundPitch(Sound sound, float pitch) {
        FloppyTurd::AudioManagerCppBridge::setSoundPitch(std::to_string(sound.id), pitch);
    }

    void SetSoundPan(Sound sound, float pan) {
        FloppyTurd::AudioManagerCppBridge::setSoundPan(std::to_string(sound.id), pan);
    }

    bool IsSoundPlaying(Sound sound) {
        return FloppyTurd::AudioManagerCppBridge::isSoundPlaying(std::to_string(sound.id));
    }

    void UnloadSound(Sound sound) {
        FloppyTurd::AudioEngineCppBridge::unloadSound(sound.id);
    }
    
    Music LoadMusicStream(const char* fileName) {
        int32_t musicId = FloppyTurd::AudioEngineCppBridge::loadMusicStream(fileName);
        return Music{musicId, nullptr, 44100};
    }
    
    Music LoadMusic(const char* fileName) {
        return LoadMusicStream(fileName);
    }
    
    void PlayMusicStream(Music music) {
        FloppyTurd::AudioEngineCppBridge::playMusicStream(music.id);
    }

    void PlayMusic(Music music) {
        PlayMusicStream(music);
    }

    void StopMusicStream(Music music) {
        FloppyTurd::AudioEngineCppBridge::stopMusicStream(music.id);
    }
    
    void StopMusic() {
        FloppyTurd::AudioManagerCppBridge::stopMusic();
    }
    
    void PauseMusicStream(Music music) {
        FloppyTurd::AudioManagerCppBridge::pauseMusic();
    }
    
    void ResumeMusicStream(Music music) {
        FloppyTurd::AudioManagerCppBridge::resumeMusic();
    }
    
    void UpdateMusicStream(Music music) {
        FloppyTurd::AudioEngineCppBridge::updateMusicStream(static_cast<int32_t>(music.id));
    }
    
    void SetMusicVolume(float volume) {
        FloppyTurd::AudioManagerCppBridge::setMusicVolume(volume);
    }
    
    void SetMusicVolumeForId(Music music, float volume) {
        FloppyTurd::AudioEngineCppBridge::setMusicVolume(static_cast<int32_t>(music.id), volume);
    }
    
    bool IsMusicStreamPlaying(Music music) {
        return FloppyTurd::AudioManagerCppBridge::isMusicPlaying();
    }
    
    bool IsMusicPlaying() {
        return FloppyTurd::AudioManagerCppBridge::isMusicPlaying();
    }
    
    void SetMusicLooping(Music music, bool looping) {
        FloppyTurd::AudioManagerCppBridge::setMusicLooping(looping);
    }
    
    float GetMusicTimeLength(Music music) {
        return FloppyTurd::AudioManagerCppBridge::getMusicTimeLength();
    }
    
    float GetMusicTimePlayed(Music music) {
        return FloppyTurd::AudioManagerCppBridge::getMusicTimePlayed();
    }
    
    float GetMusicDuration(Music music) {
        return GetMusicTimeLength(music);
    }
    
    void UnloadMusicStream(Music music) {
        FloppyTurd::AudioEngineCppBridge::unloadMusicStream(static_cast<int32_t>(music.id));
    }
    
    void UnloadMusic(Music music) {
        UnloadMusicStream(music);
    }
    
    // Time Functions - Direct Swift calls
    double GetTime() {
        return static_cast<double>(FloppyTurd::MetalRendererSwift::getFrameTime());
    }
    
    float GetFrameTime() {
        return GetCurrentFrameTime();
    }
    
    // Math and Utility Functions - Direct Swift calls
    int GetRandomValue(int min, int max) {
        return FloppyTurd::MathUtilsSwift::getRandomValue(static_cast<int32_t>(min), static_cast<int32_t>(max));
    }
    
    float GetRandomFloat(float min, float max) {
        return FloppyTurd::MathUtilsSwift::getRandomFloat(min, max);
    }
    
    void SetRandomSeed(unsigned int seed) {
        FloppyTurd::MathUtilsSwift::setRandomSeed(static_cast<uint32_t>(seed));
    }
    
    Vector2 GetRandomVector2(Vector2 min, Vector2 max) {
        return {
            static_cast<float>(GetRandomValue(static_cast<int>(min.x), static_cast<int>(max.x))),
            static_cast<float>(GetRandomValue(static_cast<int>(min.y), static_cast<int>(max.y)))
        };
    }
    
    Color GetRandomColor() {
        return Color{
            static_cast<unsigned char>(GetRandomValue(0, 255)),
            static_cast<unsigned char>(GetRandomValue(0, 255)),
            static_cast<unsigned char>(GetRandomValue(0, 255)),
            255
        };
    }
    
    void TraceLog(int logLevel, const char* text, ...) {
        va_list args;
        va_start(args, text);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), text, args);
        va_end(args);
        // TODO: Implement LoggingSwift bridge
        // FloppyTurd::LoggingSwift::traceLog(static_cast<int32_t>(logLevel), buffer);
        printf("[LOG %d] %s\n", logLevel, buffer);
    }
    
    void SetTraceLogLevel(int logLevel) {
        // TODO: Implement LoggingSwift bridge
        // FloppyTurd::LoggingSwift::setTraceLogLevel(static_cast<int32_t>(logLevel));
    }
    
    void SetConfigFlags(unsigned int flags) {
        // TODO: Implement ConfigurationSwift bridge
        // FloppyTurd::ConfigurationSwift::setConfigFlags(static_cast<uint32_t>(flags));
    }
    
    // Vector Math Functions - Direct Swift calls
    float Vector2Length(Vector2 v) {
        // Convert to Swift Vector2 type
        FloppyTurd::Vector2 swiftV = FloppyTurd::Vector2::init(v.x, v.y);
        return FloppyTurd::MathUtilsSwift::vector2Length(swiftV);
    }
    
    Vector2 Vector2Normalize(Vector2 v) {
        float length = Vector2Length(v);
        if (length > 0) {
            return Vector2{v.x / length, v.y / length};
        }
        return Vector2{0, 0};
    }
    
    Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
        return Vector2{v1.x + v2.x, v1.y + v2.y};
    }
    
    Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
        return Vector2{v1.x - v2.x, v1.y - v2.y};
    }
    
    Vector2 Vector2Scale(Vector2 v, float scale) {
        return Vector2{v.x * scale, v.y * scale};
    }
    
    float Vector2Distance(Vector2 v1, Vector2 v2) {
        return Vector2Length(Vector2Subtract(v1, v2));
    }
    
    // Collision Detection Functions
    bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
        return (rec1.x < rec2.x + rec2.width && rec1.x + rec1.width > rec2.x &&
                rec1.y < rec2.y + rec2.height && rec1.y + rec1.height > rec2.y);
    }
    
    bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) {
        float dx = center.x - fmaxf(rec.x, fminf(center.x, rec.x + rec.width));
        float dy = center.y - fmaxf(rec.y, fminf(center.y, rec.y + rec.height));
        return (dx * dx + dy * dy) <= (radius * radius);
    }
    
    bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
        return (point.x >= rec.x && point.x <= rec.x + rec.width &&
                point.y >= rec.y && point.y <= rec.y + rec.height);
    }
    
    // Color Functions
    Color ColorAlpha(Color color, float alpha) {
        return Color{color.r, color.g, color.b, static_cast<unsigned char>(alpha * 255)};
    }
    
    Color Fade(Color color, float alpha) {
        return ColorAlpha(color, alpha);
    }
    
    Color ColorLerp(Color color1, Color color2, float amount) {
        return Color{
            static_cast<unsigned char>(color1.r + (color2.r - color1.r) * amount),
            static_cast<unsigned char>(color1.g + (color2.g - color1.g) * amount),
            static_cast<unsigned char>(color1.b + (color2.b - color1.b) * amount),
            static_cast<unsigned char>(color1.a + (color2.a - color1.a) * amount)
        };
    }
    
    // Math Utility Functions
    float Clamp(float value, float min, float max) {
        return fmaxf(min, fminf(max, value));
    }
    
    float Lerp(float start, float end, float amount) {
        return start + (end - start) * amount;
    }
    
    // Text Formatting Functions
    const char* TextFormat(const char* text, ...) {
        static char buffer[1024];
        va_list args;
        va_start(args, text);
        vsnprintf(buffer, sizeof(buffer), text, args);
        va_end(args);
        return buffer;
    }
    
    // Rectangle Utility Functions
    Rectangle RectangleNew(float x, float y, float width, float height) {
        return Rectangle{x, y, width, height};
    }
    
    // Platform-Specific Functions
    void SetOrientation(bool landscape) {
        // TODO: Implement DeviceOrientationSwift bridge
        // FloppyTurd::DeviceOrientationSwift::setOrientation(landscape);
    }
    
    void ShowVirtualKeyboard(bool show) {
        // TODO: Implement VirtualKeyboardSwift bridge
        // FloppyTurd::VirtualKeyboardSwift::showVirtualKeyboard(show);
    }
    
    void Vibrate(int milliseconds) {
        // TODO: Implement HapticsSwift bridge
        // FloppyTurd::HapticsSwift::vibrate(static_cast<int32_t>(milliseconds));
    }
    
    const char* GetResourcePath(const char* resourceName) {
        static std::string result = FloppyTurd::ResourceManagerCppBridge::getResourcePath(std::string(resourceName));
        return result.c_str();
    }
    
    const char* GetSaveDataPath(const char* filename) {
        static std::string result = FloppyTurd::ResourceManagerCppBridge::getSaveDataPath(std::string(filename));
        return result.c_str();
    }
    
    bool IsMobilePlatform() {
        return true;
    }
    
    bool PreferLowPowerMode() {
        return false;
    }
    
    int GetRecommendedTextureSize() {
        return 2048;
    }
    
    // UICoordinateSystem Functions - Already using direct Swift calls!
    Rectangle GetSafeAreaRect(bool includeStatusBar = true) {
        auto swiftRect = FloppyTurd::UICoordinateSystem_GetSafeAreaRect(includeStatusBar);
        return Rectangle{swiftRect.getX(), swiftRect.getY(), swiftRect.getWidth(), swiftRect.getHeight()};
    }
    
    Rectangle GetPixelScreenRect() {
        auto swiftRect = FloppyTurd::UICoordinateSystem_GetPixelScreenRect();
        return Rectangle{swiftRect.getX(), swiftRect.getY(), swiftRect.getWidth(), swiftRect.getHeight()};
    }
    
    // Additional iOS-specific functions
    void SetWindowSize(int width, int height) {
        // iOS doesn't support window resizing
    }
    
    void ToggleFullscreen() {
        // iOS is always fullscreen
    }
    
    bool IsWindowFullscreen() {
        return true;
    }
    
    Vector2 GetScreenCenter() {
        auto rect = GetPixelScreenRect();
        return Vector2{rect.width / 2.0f, rect.height / 2.0f};
    }
    
    Vector2 GetRenderScale() {
        return Vector2{1.0f, 1.0f};
    }
    
    Rectangle GetSafeArea() {
        return GetSafeAreaRect(true);
    }
    
    float GetScreenDensity() {
        return GetScreenScale();
    }
    
    bool IsLandscape() {
        auto rect = GetPixelScreenRect();
        return rect.width > rect.height;
    }
    
    bool IsPortrait() {
        return !IsLandscape();
    }
    
    void SetPreferredOrientation(bool landscape) {
        SetOrientation(landscape);
    }
    
    bool ShouldUseLargerTouchTargets() {
        return true;
    }
    
    int GetRecommendedFontSize() {
        return 20;
    }
    
    void UpdateSafeAreaInsets(float top, float right, float bottom, float left) {
        // TODO: Fix UICoordinateSystem reference
        // FloppyTurd::UICoordinateSystemHelper::UpdateSafeAreaInsets(top, right, bottom, left);
    }
    
    // Monitor Functions (iOS doesn't have multiple monitors)
    int GetCurrentMonitor() {
        return 0;
    }
    
    int GetMonitorWidth(int monitor) {
        return GetScreenWidth();
    }
    
    int GetMonitorHeight(int monitor) {
        return GetScreenHeight();
    }
    
    // Render Texture Functions
    RenderTexture2D LoadRenderTexture(int width, int height) {
        // TODO: Implement LoadRenderTexture in MetalRendererSwift
        RenderTexture2D renderTexture = {0};
        renderTexture.id = 0; // Placeholder
        renderTexture.texture.width = width;
        renderTexture.texture.height = height;
        return renderTexture;
    }
    
    void BeginTextureMode(RenderTexture2D target) {
        FloppyTurd::MetalRendererSwift::beginTextureMode(static_cast<int32_t>(target.id));
    }
    
    void EndTextureMode() {
        FloppyTurd::MetalRendererSwift::endTextureMode();
    }
    
    void UnloadRenderTexture(RenderTexture2D target) {
        FloppyTurd::MetalRendererSwift::unloadRenderTexture(static_cast<int32_t>(target.id));
    }
    
    // iOS-specific game_main implementation
    int game_main_ios(int argc, char* argv[]) {
        // Initialize the game engine
        Game game;
        
        // Set the global game instance
        g_gameInstance = &game;
        
        // Initialize the game
        game.Initialize();
        
        // iOS uses frame-based rendering, not a blocking game loop
        // The actual game loop is handled by the iOS render loop
        // This function just sets up the game and returns
        
        return 0;
     }
     
     // ============================================================================
     // GLOBAL GAME INSTANCE MANAGEMENT IMPLEMENTATIONS
     // ============================================================================
     
     // Implementation of extern "C" functions for iOS
     Game* GetGameInstance_iOS() { return g_gameInstance; }
     void SetGameInstance_iOS(Game* game) { g_gameInstance = game; }
     void OnAppPause_iOS() { if (g_gameInstance) g_gameInstance->OnPause(); }
     void OnAppResume_iOS() { if (g_gameInstance) g_gameInstance->OnResume(); }
     void SetGlobalGameView_iOS(void* gameView) { g_globalGameView = gameView; }
     void* GetGlobalGameView_iOS() { return g_globalGameView; }
     int game_main_iOS(int argc, char* argv[]) { return game_main_ios(argc, argv); }

#else
    // ============================================================================
    // DESKTOP IMPLEMENTATIONS (via raylib)
    // ============================================================================

    // Platform Functions
    void Initialize(void* nativeView = nullptr) { ::InitWindow(800, 600, "Game"); }
    void Shutdown() { ::CloseWindow(); }
    void InitializePlatform() { /* No-op on desktop */ }
    void ShutdownPlatform() { /* No-op on desktop */ }

    // Window and Screen Functions
    void InitWindow(int width, int height, const char* title) { ::InitWindow(width, height, title); }
    void CloseWindow() { ::CloseWindow(); }
    bool WindowShouldClose() { return ::WindowShouldClose(); }
    int GetScreenWidth() { return ::GetScreenWidth(); }
    int GetScreenHeight() { return ::GetScreenHeight(); }
    float GetScreenScale() { return 1.0f; } // No equivalent in raylib, return default
    void SetTargetFPS(int fps) { ::SetTargetFPS(fps); }
    int GetCurrentFPS() { return ::GetFPS(); }
    float GetCurrentFrameTime() { return ::GetFrameTime(); }
    void SetWindowSize(int width, int height) { ::SetWindowSize(width, height); }
    void ToggleFullscreen() { ::ToggleFullscreen(); }
    bool IsWindowFullscreen() { return ::IsWindowFullscreen(); }
    Vector2 GetScreenCenter() { return {(float)::GetScreenWidth() / 2.0f, (float)::GetScreenHeight() / 2.0f}; }
    Vector2 GetRenderScale() { return {1.0f, 1.0f}; } // No equivalent
    Rectangle GetSafeArea() { return {0, 0, (float)::GetScreenWidth(), (float)::GetScreenHeight()}; } // No equivalent
    float GetScreenDensity() { return 1.0f; } // No equivalent
    bool IsLandscape() { return ::GetScreenWidth() > ::GetScreenHeight(); }
    bool IsPortrait() { return ::GetScreenWidth() <= ::GetScreenHeight(); }
    void SetPreferredOrientation(bool landscape) { /* No-op on desktop */ }
    bool ShouldUseLargerTouchTargets() { return false; }
    int GetRecommendedFontSize() { return 20; }
    void UpdateSafeAreaInsets(float top, float right, float bottom, float left) { /* No-op on desktop */ }

    // Monitor Functions
    int GetCurrentMonitor() { return ::GetCurrentMonitor(); }
    int GetMonitorWidth(int monitor) { return ::GetMonitorWidth(monitor); }
    int GetMonitorHeight(int monitor) { return ::GetMonitorHeight(monitor); }

    // Render Texture Functions
    RenderTexture2D LoadRenderTexture(int width, int height) { return ::LoadRenderTexture(width, height); }
    void BeginTextureMode(RenderTexture2D target) { ::BeginTextureMode(target); }
    void EndTextureMode() { ::EndTextureMode(); }
    void UnloadRenderTexture(RenderTexture2D target) { ::UnloadRenderTexture(target); }

    // Input Functions
    bool IsKeyPressed(int key) { return ::IsKeyPressed(key); }
    bool IsKeyDown(int key) { return ::IsKeyDown(key); }
    bool IsKeyReleased(int key) { return ::IsKeyReleased(key); }
    bool IsMouseButtonPressed(int button) { return ::IsMouseButtonPressed(button); }
    bool IsMouseButtonDown(int button) { return ::IsMouseButtonDown(button); }
    bool IsMouseButtonReleased(int button) { return ::IsMouseButtonReleased(button); }
    Vector2 GetMousePosition() { return ::GetMousePosition(); }
    Vector2 GetMouseDelta() { return ::GetMouseDelta(); }
    Vector2 GetTouchPosition(int index) { return ::GetMousePosition(); } // Fallback to mouse
    bool IsPrimaryInputPressed() { return ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT); }
    bool IsPrimaryInputDown() { return ::IsMouseButtonDown(MOUSE_BUTTON_LEFT); }
    bool IsPrimaryInputReleased() { return ::IsMouseButtonReleased(MOUSE_BUTTON_LEFT); }
    Vector2 GetPrimaryInputPosition() { return ::GetMousePosition(); }

    // Rendering Functions
    void BeginDrawing() { ::BeginDrawing(); }
    void EndDrawing() { ::EndDrawing(); }
    void ClearBackground(Color color) { ::ClearBackground(color); }
    void DrawRectangle(int posX, int posY, int width, int height, Color color) { ::DrawRectangle(posX, posY, width, height, color); }
    void DrawRectangleRec(Rectangle rec, Color color) { ::DrawRectangleRec(rec, color); }
    void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) { ::DrawRectangleLinesEx(rec, lineThick, color); }
    void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) { ::DrawRectangleRounded(rec, roundness, segments, color); }
    void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) { ::DrawRectangleRoundedLinesEx(rec, roundness, segments, lineThick, color); }
    void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) { ::DrawLine(startPosX, startPosY, endPosX, endPosY, color); }
    void DrawLineV(Vector2 startPos, Vector2 endPos, Color color) { ::DrawLineV(startPos, endPos, color); }
    void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) { ::DrawLineEx(startPos, endPos, thick, color); }
    void DrawText(const char* text, int posX, int posY, int fontSize, Color color) { ::DrawText(text, posX, posY, fontSize, color); }
    void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) { ::DrawTextEx(font, text, position, fontSize, spacing, tint); }
    void BeginScissorMode(int x, int y, int width, int height) { ::BeginScissorMode(x, y, width, height); }
    void EndScissorMode() { ::EndScissorMode(); }
    void DrawFPS(int posX, int posY) { ::DrawFPS(posX, posY); }

    // Texture Functions
    Texture2D LoadTexture(const char* fileName) { return ::LoadTexture(fileName); }
    void UnloadTexture(Texture2D texture) { ::UnloadTexture(texture); }
    Image LoadImage(const char* fileName) { return ::LoadImage(fileName); }
    void UnloadImage(Image image) { ::UnloadImage(image); }
    void SetTextureWrap(Texture2D texture, int wrap) { ::SetTextureWrap(texture, wrap); }
    Texture2D LoadTextureFromImage(Image image) { return ::LoadTextureFromImage(image); }
    void DrawTexture(Texture2D texture, int posX, int posY, Color tint) { ::DrawTexture(texture, posX, posY, tint); }
    void DrawTextureV(Texture2D texture, Vector2 position, Color tint) { ::DrawTextureV(texture, position, tint); }
    void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) { ::DrawTextureRec(texture, source, position, tint); }
    void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) { ::DrawTexturePro(texture, source, dest, origin, rotation, tint); }
    void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) { ::DrawTextureEx(texture, position, rotation, scale, tint); }
    void SetTextureFilter(Texture2D texture, int filter) { ::SetTextureFilter(texture, filter); }
    
    // Font Functions
    Font LoadFont(const char* fileName) { return ::LoadFont(fileName); }
    Font LoadFontEx(const char* fileName, int fontSize, int* fontChars, int glyphCount) { return ::LoadFontEx(fileName, fontSize, fontChars, glyphCount); }
    void UnloadFont(Font font) { ::UnloadFont(font); }
    int MeasureText(const char* text, int fontSize) { return ::MeasureText(text, fontSize); }
    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) { return ::MeasureTextEx(font, text, fontSize, spacing); }
    Font GetFontDefault() { return ::GetFontDefault(); }
    
    // Audio Functions
    void InitAudioDevice() { ::InitAudioDevice(); }
    void CloseAudioDevice() { ::CloseAudioDevice(); }
    bool IsAudioDeviceReady() { return ::IsAudioDeviceReady(); }
    void SetMasterVolume(float volume) { ::SetMasterVolume(volume); }
    Sound LoadSound(const char* fileName) { return ::LoadSound(fileName); }
    void PlaySound(Sound sound) { ::PlaySound(sound); }
    void StopSound(Sound sound) { ::StopSound(sound); }
    void PauseSound(Sound sound) { ::PauseSound(sound); }
    void ResumeSound(Sound sound) { ::ResumeSound(sound); }
    void SetSoundVolume(Sound sound, float volume) { ::SetSoundVolume(sound, volume); }
    void SetSoundPitch(Sound sound, float pitch) { ::SetSoundPitch(sound, pitch); }
    void SetSoundPan(Sound sound, float pan) { ::SetSoundPan(sound, pan); }
    bool IsSoundPlaying(Sound sound) { return ::IsSoundPlaying(sound); }
    void UnloadSound(Sound sound) { ::UnloadSound(sound); }
    Music LoadMusicStream(const char* fileName) { return ::LoadMusicStream(fileName); }
    Music LoadMusic(const char* fileName) { return ::LoadMusic(fileName); }
    void PlayMusicStream(Music music) { ::PlayMusicStream(music); }
    void PlayMusic(Music music) { ::PlayMusic(music); }
    void StopMusicStream(Music music) { ::StopMusicStream(music); }
    void StopMusic() { ::StopMusic(); }
    void PauseMusicStream(Music music) { ::PauseMusicStream(music); }
    void ResumeMusicStream(Music music) { ::ResumeMusicStream(music); }
    void UpdateMusicStream(Music music) { ::UpdateMusicStream(music); }
    void SetMusicVolume(float volume) { ::SetMusicVolume(volume); }
    void SetMusicVolumeForId(Music music, float volume) { ::SetMusicVolumeForId(music, volume); }
    bool IsMusicStreamPlaying(Music music) { return ::IsMusicStreamPlaying(music); }
    bool IsMusicPlaying() { return ::IsMusicPlaying(); }
    void SetMusicLooping(Music music, bool looping) { ::SetMusicLooping(music, looping); }
    float GetMusicTimeLength(Music music) { return ::GetMusicTimeLength(music); }
    float GetMusicTimePlayed(Music music) { return ::GetMusicTimePlayed(music); }
    float GetMusicDuration(Music music) { return ::GetMusicTimeLength(music); } // Alias for compatibility
    void UnloadMusicStream(Music music) { ::UnloadMusicStream(music); }
    void UnloadMusic(Music music) { ::UnloadMusic(music); }
    
    // Time Functions
    double GetTime() { return ::GetTime(); }
    float GetFrameTime() { return ::GetFrameTime(); }
    
    // Math and Utility Functions
    int GetRandomValue(int min, int max) { return ::GetRandomValue(min, max); }
    float GetRandomFloat(float min, float max) { return (float)GetRandomValue(min * 1000, max * 1000) / 1000.0f; }
    void SetRandomSeed(unsigned int seed) { ::SetRandomSeed(seed); }
    Vector2 GetRandomVector2(Vector2 min, Vector2 max) { return {(float)GetRandomValue((int)min.x, (int)max.x), (float)GetRandomValue((int)min.y, (int)max.y)}; }
    Color GetRandomColor() { return {(unsigned char)::GetRandomValue(0, 255), (unsigned char)::GetRandomValue(0, 255), (unsigned char)::GetRandomValue(0, 255), 255}; }
    void TraceLog(int logLevel, const char* text, ...) { ::TraceLog(logLevel, text); }
    void SetTraceLogLevel(int logLevel) { ::SetTraceLogLevel(logLevel); }
    void SetConfigFlags(unsigned int flags) { ::SetConfigFlags(flags); }

    // Vector Math Functions  
    float Vector2Length(Vector2 v) { return ::Vector2Length(v); }
    Vector2 Vector2Normalize(Vector2 v) { return ::Vector2Normalize(v); }
    Vector2 Vector2Add(Vector2 v1, Vector2 v2) { return ::Vector2Add(v1, v2); }
    Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) { return ::Vector2Subtract(v1, v2); }
    Vector2 Vector2Scale(Vector2 v, float scale) { return ::Vector2Scale(v, scale); }
    float Vector2Distance(Vector2 v1, Vector2 v2) { return ::Vector2Distance(v1, v2); }

    // Collision Detection Functions
    bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) { return ::CheckCollisionRecs(rec1, rec2); }
    bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) { return ::CheckCollisionCircleRec(center, radius, rec); }
    bool CheckCollisionPointRec(Vector2 point, Rectangle rec) { return ::CheckCollisionPointRec(point, rec); }

    // Color Functions
    Color ColorAlpha(Color color, float alpha) { return ::ColorAlpha(color, alpha); }
    Color Fade(Color color, float alpha) { return ::Fade(color, alpha); }
    Color ColorLerp(Color color1, Color color2, float amount) { return ::ColorLerp(color1, color2, amount); }

    // Math Utility Functions
    float Clamp(float value, float min, float max) { return ::Clamp(value, min, max); }
    float Lerp(float start, float end, float amount) { return ::Lerp(start, end, amount); }
    
    // Text Formatting Functions
    const char* TextFormat(const char* text, ...) { 
        static char buffer[1024];
        va_list args;
        va_start(args, text);
        vsnprintf(buffer, sizeof(buffer), text, args);
        va_end(args);
        return buffer;
    }

    // Rectangle Utility Functions
    Rectangle RectangleNew(float x, float y, float width, float height) { return {x, y, width, height}; }

    // Platform-Specific Functions
    void SetOrientation(bool landscape) { /* No-op on desktop */ }
    void ShowVirtualKeyboard(bool show) { /* No-op on desktop */ }
    void Vibrate(int milliseconds) { /* No-op on desktop */ }
    const char* GetResourcePath(const char* resourceName) { return resourceName; }
    const char* GetSaveDataPath(const char* filename) { return filename; }
    bool IsMobilePlatform() { return false; }
    bool PreferLowPowerMode() { return false; }
    int GetRecommendedTextureSize() { return 2048; }
    
    // UICoordinateSystem Functions (desktop fallback)
    Rectangle GetSafeAreaRect(bool includeStatusBar = true) {
        return Rectangle{0, 0, (float)::GetScreenWidth(), (float)::GetScreenHeight()};
    }
    Rectangle GetPixelScreenRect() {
        return Rectangle{0, 0, (float)::GetScreenWidth(), (float)::GetScreenHeight()};
    }
    
    // Desktop-specific implementations
    Game* GetGameInstance_Desktop() { return g_gameInstance; }
    void SetGameInstance_Desktop(Game* game) { g_gameInstance = game; }
    void OnAppPause_Desktop() { /* No-op on desktop */ }
    void OnAppResume_Desktop() { /* No-op on desktop */ }
    void SetGlobalGameView_Desktop(void* gameView) { g_globalGameView = gameView; }
    void* GetGlobalGameView_Desktop() { return g_globalGameView; }
    
    // Desktop game_main implementation (from main.cpp logic)
    int game_main_Desktop(int argc, char* argv[]) {
        Game game;
        g_gameInstance = &game;
        game.RunGame();
        return 0;
    }

#endif // PLATFORM_IOS

private:
    // Global game instance management
    static Game* g_gameInstance;
    static void* g_globalGameView;
};

// Static member definitions
Game* PlatformAPI::g_gameInstance = nullptr;
void* PlatformAPI::g_globalGameView = nullptr;

// ============================================================================
// GLOBAL TYPEDEF FOR EASY ACCESS
// ============================================================================

using CurrentPlatformAPI = PlatformAPI;

// ============================================================================
// EXTERN "C" FUNCTION IMPLEMENTATIONS (Platform-agnostic wrappers)
// ============================================================================

extern "C" {
    // Get the current game instance (returns nullptr if not set)
    Game* GetGameInstance() {
#if defined(PLATFORM_IOS)
        return PlatformAPI::GetInstance().GetGameInstance_iOS();
#else
        return PlatformAPI::GetInstance().GetGameInstance_Desktop();
#endif
    }
    
    // Set the current game instance (call from Game constructor)
    void SetGameInstance(Game* game) {
#if defined(PLATFORM_IOS)
        PlatformAPI::GetInstance().SetGameInstance_iOS(game);
#else
        PlatformAPI::GetInstance().SetGameInstance_Desktop(game);
#endif
    }
    
    // App lifecycle functions
    void OnAppPause() {
#if defined(PLATFORM_IOS)
        PlatformAPI::GetInstance().OnAppPause_iOS();
#else
        PlatformAPI::GetInstance().OnAppPause_Desktop();
#endif
    }
    
    void OnAppResume() {
#if defined(PLATFORM_IOS)
        PlatformAPI::GetInstance().OnAppResume_iOS();
#else
        PlatformAPI::GetInstance().OnAppResume_Desktop();
#endif
    }
    
    // Global game view management for iOS
    void SetGlobalGameView(void* gameView) {
#if defined(PLATFORM_IOS)
        PlatformAPI::GetInstance().SetGlobalGameView_iOS(gameView);
#else
        PlatformAPI::GetInstance().SetGlobalGameView_Desktop(gameView);
#endif
    }
    
    void* GetGlobalGameView() {
#if defined(PLATFORM_IOS)
        return PlatformAPI::GetInstance().GetGlobalGameView_iOS();
#else
        return PlatformAPI::GetInstance().GetGlobalGameView_Desktop();
#endif
    }
    
    // Game main function
    int game_main(int argc, char* argv[]) {
#if defined(PLATFORM_IOS)
        return PlatformAPI::GetInstance().game_main_iOS(argc, argv);
#else
        return PlatformAPI::GetInstance().game_main_Desktop(argc, argv);
#endif
    }
}

// ============================================================================
// GLOBAL STANDALONE FUNCTIONS (for compatibility)
// ============================================================================

// Math and Utility Functions
inline int GetRandomValue(int min, int max) { return PlatformAPI::GetInstance().GetRandomValue(min, max); }
inline float GetRandomFloat(float min, float max) { return PlatformAPI::GetInstance().GetRandomFloat(min, max); }
inline void SetRandomSeed(unsigned int seed) { PlatformAPI::GetInstance().SetRandomSeed(seed); }
inline Vector2 GetRandomVector2(Vector2 min, Vector2 max) { return PlatformAPI::GetInstance().GetRandomVector2(min, max); }
inline Color GetRandomColor() { return PlatformAPI::GetInstance().GetRandomColor(); }
inline void TraceLog(int logLevel, const char* text, ...) { 
    va_list args;
    va_start(args, text);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);
    PlatformAPI::GetInstance().TraceLog(logLevel, buffer);
}

// Texture Functions  
inline Texture2D LoadTexture(const char* fileName) { return PlatformAPI::GetInstance().LoadTexture(fileName); }
inline void UnloadTexture(Texture2D texture) { PlatformAPI::GetInstance().UnloadTexture(texture); }
inline Image LoadImage(const char* fileName) { return PlatformAPI::GetInstance().LoadImage(fileName); }
inline void UnloadImage(Image image) { PlatformAPI::GetInstance().UnloadImage(image); }
inline void SetTextureWrap(Texture2D texture, int wrap) { PlatformAPI::GetInstance().SetTextureWrap(texture, wrap); }
inline void SetTextureFilter(Texture2D texture, int filter) { PlatformAPI::GetInstance().SetTextureFilter(texture, filter); }

// Render Texture Functions
inline RenderTexture2D LoadRenderTexture(int width, int height) { return PlatformAPI::GetInstance().LoadRenderTexture(width, height); }
inline void BeginTextureMode(RenderTexture2D target) { PlatformAPI::GetInstance().BeginTextureMode(target); }
inline void EndTextureMode() { PlatformAPI::GetInstance().EndTextureMode(); }
inline void UnloadRenderTexture(RenderTexture2D target) { PlatformAPI::GetInstance().UnloadRenderTexture(target); }

// Audio Functions
inline Sound LoadSound(const char* fileName) { return PlatformAPI::GetInstance().LoadSound(fileName); }
inline void PlaySound(Sound sound) { PlatformAPI::GetInstance().PlaySound(sound); }
inline void StopSound(Sound sound) { PlatformAPI::GetInstance().StopSound(sound); }
inline void UnloadSound(Sound sound) { PlatformAPI::GetInstance().UnloadSound(sound); }
inline void SetSoundVolume(Sound sound, float volume) { PlatformAPI::GetInstance().SetSoundVolume(sound, volume); }
inline Music LoadMusic(const char* fileName) { return PlatformAPI::GetInstance().LoadMusic(fileName); }
inline void PlayMusic(Music music) { PlatformAPI::GetInstance().PlayMusic(music); }
inline void StopMusic() { PlatformAPI::GetInstance().StopMusic(); }
inline void StopMusic(Music music) { PlatformAPI::GetInstance().StopMusicStream(music); }
inline void UnloadMusic(Music music) { PlatformAPI::GetInstance().UnloadMusic(music); }
inline void PauseMusic(Music music) { PlatformAPI::GetInstance().PauseMusicStream(music); }
inline void ResumeMusic(Music music) { PlatformAPI::GetInstance().ResumeMusicStream(music); }
inline void UpdateMusic(Music music) { PlatformAPI::GetInstance().UpdateMusicStream(music); }
inline bool IsMusicPlaying() { return PlatformAPI::GetInstance().IsMusicPlaying(); }
inline bool IsMusicPlaying(Music music) { return PlatformAPI::GetInstance().IsMusicStreamPlaying(music); }
inline void SetMusicVolume(float volume) { PlatformAPI::GetInstance().SetMusicVolume(volume); }
inline void SetMusicVolume(Music music, float volume) { PlatformAPI::GetInstance().SetMusicVolumeForId(music, volume); }
inline void SetMusicLooping(Music music, bool looping) { PlatformAPI::GetInstance().SetMusicLooping(music, looping); }

// Math and Utility Functions  
inline float Clamp(float value, float min, float max) { return PlatformAPI::GetInstance().Clamp(value, min, max); }
inline float Lerp(float start, float end, float amount) { return PlatformAPI::GetInstance().Lerp(start, end, amount); }
inline Image GenImageColor(int width, int height, Color color) { 
    // For now, return an invalid image - this function needs proper implementation in PlatformAPI
    return Image{0}; 
}
inline Texture2D LoadTextureFromImage(Image image) { return PlatformAPI::GetInstance().LoadTextureFromImage(image); }

// Additional functions
inline Color ColorLerp(Color color1, Color color2, float amount) { return PlatformAPI::GetInstance().ColorLerp(color1, color2, amount); }
inline Color ColorAlpha(Color color, float alpha) { return PlatformAPI::GetInstance().ColorAlpha(color, alpha); }
inline Color Fade(Color color, float alpha) { return PlatformAPI::GetInstance().Fade(color, alpha); }
inline void BeginScissorMode(int x, int y, int width, int height) { PlatformAPI::GetInstance().BeginScissorMode(x, y, width, height); }
inline void EndScissorMode() { PlatformAPI::GetInstance().EndScissorMode(); }
inline Vector2 GetMouseDelta() { return PlatformAPI::GetInstance().GetMouseDelta(); }
inline bool IsMobilePlatform() { return PlatformAPI::GetInstance().IsMobilePlatform(); }

// Constants that are missing
#ifndef MOUSE_LEFT_BUTTON
#define MOUSE_LEFT_BUTTON 0
#endif

// Font Functions
inline Font LoadFont(const char* fileName) { return PlatformAPI::GetInstance().LoadFont(fileName); }
inline void UnloadFont(Font font) { PlatformAPI::GetInstance().UnloadFont(font); }
inline int MeasureText(const char* text, int fontSize) { return PlatformAPI::GetInstance().MeasureText(text, fontSize); }
inline Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) { return PlatformAPI::GetInstance().MeasureTextEx(font, text, fontSize, spacing); }
inline const char* TextFormat(const char* text, ...) { 
    static char buffer[1024];
    va_list args;
    va_start(args, text);
    vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);
    return buffer;
}

// Rendering Functions
inline void BeginDrawing() { PlatformAPI::GetInstance().BeginDrawing(); }
inline void EndDrawing() { PlatformAPI::GetInstance().EndDrawing(); }
inline void ClearBackground(Color color) { PlatformAPI::GetInstance().ClearBackground(color); }
inline void DrawRectangle(int posX, int posY, int width, int height, Color color) { PlatformAPI::GetInstance().DrawRectangle(posX, posY, width, height, color); }
inline void DrawRectangleRec(Rectangle rec, Color color) { PlatformAPI::GetInstance().DrawRectangleRec(rec, color); }
inline void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) { PlatformAPI::GetInstance().DrawRectangleRounded(rec, roundness, segments, color); }
inline void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) { PlatformAPI::GetInstance().DrawRectangleLinesEx(rec, lineThick, color); }
inline void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) { PlatformAPI::GetInstance().DrawLine(startPosX, startPosY, endPosX, endPosY, color); }
inline void DrawTexture(Texture2D texture, int posX, int posY, Color tint) { PlatformAPI::GetInstance().DrawTexture(texture, posX, posY, tint); }
inline void DrawTextureV(Texture2D texture, Vector2 position, Color tint) { PlatformAPI::GetInstance().DrawTextureV(texture, position, tint); }
inline void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) { PlatformAPI::GetInstance().DrawTextureEx(texture, position, rotation, scale, tint); }
inline void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) { PlatformAPI::GetInstance().DrawTextureRec(texture, source, position, tint); }
inline void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) { PlatformAPI::GetInstance().DrawTexturePro(texture, source, dest, origin, rotation, tint); }
inline void DrawText(const char* text, int posX, int posY, int fontSize, Color color) { PlatformAPI::GetInstance().DrawText(text, posX, posY, fontSize, color); }
inline void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) { PlatformAPI::GetInstance().DrawTextEx(font, text, position, fontSize, spacing, tint); }
inline void DrawCircle(int centerX, int centerY, float radius, Color color) { PlatformAPI::GetInstance().DrawCircle(centerX, centerY, radius, color); }
inline void DrawCircleV(Vector2 center, float radius, Color color) { PlatformAPI::GetInstance().DrawCircleV(center, radius, color); }

// Window and Screen Functions
inline void InitWindow(int width, int height, const char* title) { PlatformAPI::GetInstance().InitWindow(width, height, title); }
inline void CloseWindow() { PlatformAPI::GetInstance().CloseWindow(); }
inline bool WindowShouldClose() { return PlatformAPI::GetInstance().WindowShouldClose(); }
inline int GetScreenWidth() { return PlatformAPI::GetInstance().GetScreenWidth(); }
inline int GetScreenHeight() { return PlatformAPI::GetInstance().GetScreenHeight(); }
inline void SetTargetFPS(int fps) { PlatformAPI::GetInstance().SetTargetFPS(fps); }
inline void SetWindowSize(int width, int height) { PlatformAPI::GetInstance().SetWindowSize(width, height); }
inline void ToggleFullscreen() { PlatformAPI::GetInstance().ToggleFullscreen(); }

// Input Functions
inline bool IsKeyPressed(int key) { return PlatformAPI::GetInstance().IsKeyPressed(key); }
inline bool IsKeyDown(int key) { return PlatformAPI::GetInstance().IsKeyDown(key); }
inline bool IsMouseButtonPressed(int button) { return PlatformAPI::GetInstance().IsMouseButtonPressed(button); }
inline bool IsMouseButtonDown(int button) { return PlatformAPI::GetInstance().IsMouseButtonDown(button); }
inline bool IsMouseButtonReleased(int button) { return PlatformAPI::GetInstance().IsMouseButtonReleased(button); }
inline bool IsPrimaryInputPressed() { return PlatformAPI::GetInstance().IsPrimaryInputPressed(); }
inline bool IsPrimaryInputReleased() { return PlatformAPI::GetInstance().IsPrimaryInputReleased(); }
inline Vector2 GetMousePosition() { return PlatformAPI::GetInstance().GetMousePosition(); }
inline Vector2 GetTouchPosition(int index) { return PlatformAPI::GetInstance().GetTouchPosition(index); }

// Time Functions
inline double GetTime() { return PlatformAPI::GetInstance().GetTime(); }
inline float GetFrameTime() { return PlatformAPI::GetInstance().GetFrameTime(); }

// Vector Math Functions
inline float Vector2Length(Vector2 v) { return PlatformAPI::GetInstance().Vector2Length(v); }
inline Vector2 Vector2Normalize(Vector2 v) { return PlatformAPI::GetInstance().Vector2Normalize(v); }
inline Vector2 Vector2Add(Vector2 v1, Vector2 v2) { return PlatformAPI::GetInstance().Vector2Add(v1, v2); }
inline Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) { return PlatformAPI::GetInstance().Vector2Subtract(v1, v2); }
inline Vector2 Vector2Scale(Vector2 v, float scale) { return PlatformAPI::GetInstance().Vector2Scale(v, scale); }
inline float Vector2Distance(Vector2 v1, Vector2 v2) { return PlatformAPI::GetInstance().Vector2Distance(v1, v2); }

// Collision Detection Functions
inline bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) { return PlatformAPI::GetInstance().CheckCollisionRecs(rec1, rec2); }
inline bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) { return PlatformAPI::GetInstance().CheckCollisionCircleRec(center, radius, rec); }
inline bool CheckCollisionPointRec(Vector2 point, Rectangle rec) { return PlatformAPI::GetInstance().CheckCollisionPointRec(point, rec); }

// Platform-Specific Functions
inline void SetOrientation(bool landscape) { PlatformAPI::GetInstance().SetOrientation(landscape); }
inline void ShowVirtualKeyboard(bool show) { PlatformAPI::GetInstance().ShowVirtualKeyboard(show); }
inline void Vibrate(int milliseconds) { PlatformAPI::GetInstance().Vibrate(milliseconds); }
inline const char* GetResourcePath(const char* resourceName) { return PlatformAPI::GetInstance().GetResourcePath(resourceName); }
inline const char* GetSaveDataPath(const char* filename) { return PlatformAPI::GetInstance().GetSaveDataPath(filename); }
inline bool PreferLowPowerMode() { return PlatformAPI::GetInstance().PreferLowPowerMode(); }
inline int GetRecommendedTextureSize() { return PlatformAPI::GetInstance().GetRecommendedTextureSize(); }

// UICoordinateSystem Functions (using direct Swift interop!)
inline Rectangle GetSafeAreaRect(bool includeStatusBar = true) { return PlatformAPI::GetInstance().GetSafeAreaRect(includeStatusBar); }
inline Rectangle GetPixelScreenRect() { return PlatformAPI::GetInstance().GetPixelScreenRect(); }

// Additional iOS-specific functions
inline Vector2 GetScreenCenter() { return PlatformAPI::GetInstance().GetScreenCenter(); }
inline Vector2 GetRenderScale() { return PlatformAPI::GetInstance().GetRenderScale(); }
inline Rectangle GetSafeArea() { return PlatformAPI::GetInstance().GetSafeArea(); }
inline float GetScreenDensity() { return PlatformAPI::GetInstance().GetScreenDensity(); }
inline bool IsLandscape() { return PlatformAPI::GetInstance().IsLandscape(); }
inline bool IsPortrait() { return PlatformAPI::GetInstance().IsPortrait(); }
inline void SetPreferredOrientation(bool landscape) { PlatformAPI::GetInstance().SetPreferredOrientation(landscape); }
inline bool ShouldUseLargerTouchTargets() { return PlatformAPI::GetInstance().ShouldUseLargerTouchTargets(); }
inline int GetRecommendedFontSize() { return PlatformAPI::GetInstance().GetRecommendedFontSize(); }

// Include Game class definition after global functions to avoid circular dependency
#if defined(PLATFORM_IOS)
#include "Game.h"
#endif

#endif // PLATFORM_API_MODERN_H