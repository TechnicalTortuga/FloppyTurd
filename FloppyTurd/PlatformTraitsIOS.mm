#import "PlatformTraits.h"
#import "MetalRenderer.h"
#import "MetalTextRenderer.h"
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// External references to the Metal renderers
extern MetalRenderer* g_metalRenderer;
extern MetalTextRenderer* g_textRenderer;

// ============================================================================
// REAL IOS/METAL IMPLEMENTATIONS
// ============================================================================

// ============================================================================
// TEXTURE FUNCTIONS
// ============================================================================

Texture2D IOSTraits::LoadTexture(const char* fileName) {
    if (g_metalRenderer) {
        return g_metalRenderer->LoadTexture(fileName);
    }
    TraceLog(LOG_ERROR, "[IOSTraits] LoadTexture: Metal renderer not initialized");
    return {0, 0, 0, 0, 0, nullptr};
}

void IOSTraits::UnloadTexture(Texture2D texture) {
    if (g_metalRenderer) {
        g_metalRenderer->UnloadTexture(texture);
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] UnloadTexture: Metal renderer not initialized");
    }
}

Image IOSTraits::LoadImage(const char* fileName) {
    // For now, just create a dummy image
    // In a real implementation, this would use Core Graphics to load the image
    TraceLog(LOG_WARNING, "[IOSTraits] LoadImage not fully implemented for file: %s", fileName);
    return {nullptr, 0, 0, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
}

void IOSTraits::UnloadImage(Image image) {
    if (image.data) {
        free(image.data);
    }
}

void IOSTraits::SetTextureWrap(Texture2D texture, int wrap) {
    if (g_metalRenderer) {
        g_metalRenderer->SetTextureWrap(texture, wrap);
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] SetTextureWrap: Metal renderer not initialized");
    }
}

void* IOSTraits::CreateTextureFromImage(void* image, int* width, int* height) {
    if (g_metalRenderer) {
        return g_metalRenderer->CreateTextureFromImage(image, width, height);
    }
    return nullptr;
}

Texture2D IOSTraits::LoadTextureFromImage(Image image) {
    if (g_metalRenderer) {
        return g_metalRenderer->LoadTextureFromImage(image);
    }
    return {0, 0, 0, 0, 0, nullptr};
}

Image IOSTraits::LoadImageFromTexture(Texture2D texture) {
    if (g_metalRenderer) {
        return g_metalRenderer->LoadImageFromTexture(texture);
    }
    return {nullptr, 0, 0, 0, 0};
}

void IOSTraits::SetTextureFilter(Texture2D texture, int filter) {
    if (g_metalRenderer) {
        g_metalRenderer->SetTextureFilter(texture, filter);
    } else {
        TraceLog(LOG_INFO, "[IOSTraits] SetTextureFilter: filter=%d", filter);
    }
}

Rectangle IOSTraits::GetTextureRec(Texture2D texture) {
    if (g_metalRenderer) {
        return g_metalRenderer->GetTextureRec(texture);
    }
    if (texture.id == 0) {
        TraceLog(LOG_WARNING, "[IOSTraits] GetTextureRec: Invalid texture ID");
        return {0, 0, 0, 0};
    }
    return {0, 0, (float)texture.width, (float)texture.height};
}

Image IOSTraits::GenImageColor(int width, int height, Color color) {
    // iOS image generation - create a solid color image
    Image image = {0};
    image.width = width;
    image.height = height;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    image.mipmaps = 1;
    
    // Allocate memory for the image data
    int dataSize = width * height * 4; // 4 bytes per pixel (RGBA)
    image.data = malloc(dataSize);
    
    if (image.data) {
        // Fill with the specified color
        unsigned char* pixels = (unsigned char*)image.data;
        for (int i = 0; i < width * height; i++) {
            pixels[i * 4 + 0] = color.r;     // R
            pixels[i * 4 + 1] = color.g;     // G
            pixels[i * 4 + 2] = color.b;     // B
            pixels[i * 4 + 3] = color.a;     // A
        }
    }
    
    return image;
}

Font IOSTraits::GetFontDefault() {
    // iOS default font - return a basic font structure
    Font font = {0};
    font.baseSize = 10;
    font.glyphCount = 95; // Basic ASCII range
    font.glyphPadding = 0;
    font.texture = {0}; // Will be loaded by MetalTextRenderer
    font.recs = nullptr;
    font.glyphs = nullptr;
    
    // Load the default font using MetalTextRenderer
    if (g_textRenderer) {
        // This would need to be implemented in MetalTextRenderer
        // For now, return the basic structure
    }
    
    return font;
}

// ============================================================================
// FONT FUNCTIONS
// ============================================================================

Font IOSTraits::LoadFont(const char* fileName) {
    if (g_textRenderer) {
        return g_textRenderer->LoadFont(fileName, 16); // Default font size
    }
    Font font = {0};
    font.font = nullptr;
    font.baseSize = 16;
    font.glyphCount = 0;
    font.glyphPadding = 0;
    font.texture = {0, 0, 0, 0, 0, nullptr};
    font.recs = nullptr;
    font.glyphs = nullptr;
    font.fontData = nullptr;
    font.ctFont = nullptr;
    font.size = 16;
    font.name = fileName;
    return font;
}

Font IOSTraits::LoadFontEx(const char* fileName, int fontSize, int* fontChars, int glyphCount) {
    if (g_textRenderer) {
        Font font = g_textRenderer->LoadFont(fileName, fontSize);
        return font;
    }
    Font font = {0};
    font.font = nullptr;
    font.baseSize = fontSize;
    font.glyphCount = glyphCount;
    font.glyphPadding = 0;
    font.texture = {0, 0, 0, 0, 0, nullptr};
    font.recs = nullptr;
    font.glyphs = nullptr;
    font.fontData = nullptr;
    font.ctFont = nullptr;
    font.size = fontSize;
    font.name = fileName;
    return font;
}

void IOSTraits::UnloadFont(Font font) {
    if (g_textRenderer) {
        g_textRenderer->UnloadFont(font);
    }
}

Vector2 IOSTraits::MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
    if (g_textRenderer) {
        return g_textRenderer->MeasureTextEx(font, text, fontSize, spacing);
    }
    return {0.0f, 0.0f};
}

// ============================================================================
// RENDERING FUNCTIONS
// ============================================================================

void IOSTraits::BeginScissorMode(int x, int y, int width, int height) {
    if (g_metalRenderer) {
        g_metalRenderer->BeginScissorMode(x, y, width, height);
    }
}

void IOSTraits::EndScissorMode() {
    if (g_metalRenderer) {
        g_metalRenderer->EndScissorMode();
    }
}

void IOSTraits::DrawFPS(int posX, int posY) {
    if (g_metalRenderer) {
        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS: %d", GetCurrentFPS());
        DrawText(fpsText, posX, posY, 20, WHITE);
    }
}

// ============================================================================
// DRAWING STATE MANAGEMENT
// ============================================================================

void IOSTraits::BeginDrawing() {
    if (g_metalRenderer) {
        g_metalRenderer->BeginDrawing();
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] BeginDrawing: Metal renderer not initialized");
    }
}

void IOSTraits::EndDrawing() {
    if (g_metalRenderer) {
        g_metalRenderer->EndDrawing();
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] EndDrawing: Metal renderer not initialized");
    }
}

void IOSTraits::ClearBackground(Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->ClearBackground(color);
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] ClearBackground: Metal renderer not initialized");
    }
}

// ============================================================================
// AUDIO FUNCTIONS
// ============================================================================

void IOSTraits::InitAudioDevice() {
    @autoreleasepool {
        NSError* error = nil;
        AVAudioSession* session = [AVAudioSession sharedInstance];
        
        // Configure audio session for game audio
        [session setCategory:AVAudioSessionCategoryPlayback 
                withOptions:AVAudioSessionCategoryOptionMixWithOthers 
                      error:&error];
        
        if (error) {
            return;
        }
        
        // Activate the audio session
        [session setActive:YES error:&error];
    }
}

void IOSTraits::InitializeAudio() {
    InitAudioDevice();
}

void IOSTraits::CloseAudioDevice() {
    @autoreleasepool {
        NSError* error = nil;
        AVAudioSession* session = [AVAudioSession sharedInstance];
        [session setActive:NO error:&error];
    }
}

void IOSTraits::ShutdownAudio() {
    CloseAudioDevice();
}

bool IOSTraits::IsAudioDeviceReady() {
    @autoreleasepool {
        AVAudioSession* session = [AVAudioSession sharedInstance];
        return session.isOtherAudioPlaying == NO;
    }
}

// ============================================================================
// RENDER TEXTURE FUNCTIONS
// ============================================================================

RenderTexture2D IOSTraits::LoadRenderTexture(int width, int height) {
    // iOS render texture loading - would need Metal implementation
    return {0, {0, 0, 0, 0, 0, nullptr}, {0, 0, 0, 0, 0, nullptr}};
}

void IOSTraits::UnloadRenderTexture(RenderTexture2D target) {
    // iOS render texture unloading - would need Metal implementation
}

void IOSTraits::BeginTextureMode(RenderTexture2D target) {
    // iOS texture mode - would need Metal implementation
}

void IOSTraits::EndTextureMode() {
    // iOS texture mode - would need Metal implementation
}

// ============================================================================
// IOS-SPECIFIC PLATFORM FUNCTIONS
// ============================================================================

void IOSTraits::SetPreferredOrientation(bool landscape) {
    // iOS-specific orientation setting
    // This would typically call UIKit methods
    TraceLog(LOG_INFO, "[IOSTraits] SetPreferredOrientation: %s", landscape ? "landscape" : "portrait");
}

bool IOSTraits::ShouldUseLargerTouchTargets() {
    // iOS-specific touch target sizing
    return true; // iOS typically needs larger touch targets
}

int IOSTraits::GetRecommendedFontSize() {
    // iOS-specific font size recommendation
    return 16; // Default iOS font size
}

std::string IOSTraits::GetResourcePath(const char* resourceName) {
    @autoreleasepool {
        NSString* nsResourceName = [NSString stringWithUTF8String:resourceName];
        NSString* path = [[NSBundle mainBundle] pathForResource:nsResourceName ofType:nil];
        if (path) {
            return std::string([path UTF8String]);
        }
        return std::string();
    }
}

bool IOSTraits::PreferLowPowerMode() {
    @autoreleasepool {
        return [[NSProcessInfo processInfo] isLowPowerModeEnabled];
    }
}

int IOSTraits::GetRecommendedTextureSize() {
    @autoreleasepool {
        float scale = [UIScreen mainScreen].scale;
        // Recommend texture sizes based on screen scale
        if (scale >= 3.0f) return 2048; // iPhone with 3x scaling
        if (scale >= 2.0f) return 1024; // iPhone with 2x scaling
        return 512; // Default
    }
}

void IOSTraits::StartCrossfade(float duration) {
    auto& state = GlobalStateManager::GetInstance();
    // For now, just log the crossfade start
    TraceLog(LOG_INFO, "[IOSTraits] StartCrossfade: %.2f seconds", duration);
}

void IOSTraits::UpdateCrossfade(float deltaTime) {
    auto& state = GlobalStateManager::GetInstance();
    // For now, just log the crossfade update
    TraceLog(LOG_INFO, "[IOSTraits] UpdateCrossfade: %.2f seconds", deltaTime);
}

void IOSTraits::FadeOutMusic(float duration) {
    auto& state = GlobalStateManager::GetInstance();
    // For now, just log the fade out
    TraceLog(LOG_INFO, "[IOSTraits] FadeOutMusic: %.2f seconds", duration);
}

void IOSTraits::FadeInMusic(float duration) {
    auto& state = GlobalStateManager::GetInstance();
    // For now, just log the fade in
    TraceLog(LOG_INFO, "[IOSTraits] FadeInMusic: %.2f seconds", duration);
}

void IOSTraits::UpdateFade(float deltaTime) {
    auto& state = GlobalStateManager::GetInstance();
    // For now, just log the fade update
    TraceLog(LOG_INFO, "[IOSTraits] UpdateFade: %.2f seconds", deltaTime);
}

// ============================================================================
// DRAWING FUNCTIONS
// ============================================================================

void IOSTraits::DrawRectangleRec(Rectangle rec, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawRectangle(rec.x, rec.y, rec.width, rec.height, color);
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] DrawRectangleRec: Metal renderer not initialized");
    }
}

void IOSTraits::DrawRectangle(float x, float y, float width, float height, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawRectangle(x, y, width, height, color);
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] DrawRectangle: Metal renderer not initialized");
    }
}

void IOSTraits::DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawRectangleLinesEx(rec, lineThick, color);
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] DrawRectangleLinesEx: Metal renderer not initialized");
    }
}

void IOSTraits::DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawRectangleRounded(rec, roundness, segments, color);
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] DrawRectangleRounded: Metal renderer not initialized");
        // Fallback to drawing a regular rectangle if rounded rect is not available
        DrawRectangleRec(rec, color);
    }
}

void IOSTraits::DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawRectangleRoundedLines(rec, roundness, segments, lineThick, color);
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] DrawRectangleRoundedLines: Metal renderer not initialized");
        // Fallback to drawing regular rectangle lines if rounded version is not available
        DrawRectangleLinesEx(rec, lineThick, color);
    }
}

void IOSTraits::DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawLineEx(startPos.x, startPos.y, endPos.x, endPos.y, thick, color);
    } else {
        TraceLog(LOG_WARNING, "[IOSTraits] DrawLineEx: Metal renderer not initialized");
    }
}

void IOSTraits::DrawTexture(Texture2D texture, float x, float y, Color tint) {
    if (!g_metalRenderer) {
        TraceLog(LOG_WARNING, "[IOSTraits] DrawTexture: Metal renderer not initialized");
        return;
    }
    
    if (texture.id == 0) {
        TraceLog(LOG_ERROR, "[IOSTraits] DrawTexture: Invalid texture ID");
        return;
    }
    
    Rectangle dest = {
        x,
        y,
        (float)texture.width,
        (float)texture.height
    };
    
    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    g_metalRenderer->DrawTexturePro(texture, source, dest, {0,0}, 0.0f, tint);
}

void IOSTraits::DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) {
    if (!g_metalRenderer) {
        TraceLog(LOG_WARNING, "[IOSTraits] DrawTextureEx: Metal renderer not initialized");
        return;
    }
    
    if (texture.id == 0) {
        TraceLog(LOG_ERROR, "[IOSTraits] DrawTextureEx: Invalid texture ID");
        return;
    }
    
    // Extract the Metal texture from the Texture2D
    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.id;
    g_metalRenderer->DrawTextureEx(metalTexture, position, rotation, scale, tint);
}

void IOSTraits::DrawCircle(float centerX, float centerY, float radius, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawCircle(centerX, centerY, radius, color);
    }
}

void IOSTraits::DrawCircleV(Vector2 center, float radius, Color color) {
    DrawCircle(center.x, center.y, radius, color);
}

void IOSTraits::DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawLine(startPosX, startPosY, endPosX, endPosY, color);
    }
}

void IOSTraits::DrawLineV(Vector2 startPos, Vector2 endPos, Color color) {
    DrawLine(startPos.x, startPos.y, endPos.x, endPos.y, color);
}



void IOSTraits::DrawTextureV(Texture2D texture, Vector2 position, Color tint) {
    DrawTexture(texture, position.x, position.y, tint);
}

void IOSTraits::DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawTextureRec(texture, source, position, tint);
    }
}

void IOSTraits::DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawTexturePro(texture, source, dest, origin, rotation, tint);
    }
}



void IOSTraits::DrawText(const char* text, float x, float y, float fontSize, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawText(text, x, y, fontSize, color);
    }
}

void IOSTraits::DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawTextEx(font, text, position, fontSize, spacing, tint);
    }
}

int IOSTraits::MeasureText(const char* text, int fontSize) {
    if (g_textRenderer) {
        Vector2 size = g_textRenderer->MeasureText(text, fontSize);
        return (int)size.x;
    }
    return 0;
}

const char* IOSTraits::TextFormat(const char* text, va_list args) {
    static char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);
    return buffer;
}

// ============================================================================
// INPUT FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

bool IOSTraits::IsKeyPressed(int key) {
    // iOS doesn't have traditional keyboard input in games
    // This would need to be implemented via virtual keyboard or external keyboard
    return false;
}

bool IOSTraits::IsKeyDown(int key) {
    // iOS doesn't have traditional keyboard input in games
    return false;
}

bool IOSTraits::IsKeyReleased(int key) {
    // iOS doesn't have traditional keyboard input in games
    return false;
}

bool IOSTraits::IsMouseButtonDown(int button) {
    // iOS touch input - treat as mouse button
    // This would need to be connected to touch state management
    return false;
}

bool IOSTraits::IsMouseButtonPressed(int button) {
    // iOS touch input - treat as mouse button
    return false;
}

bool IOSTraits::IsMouseButtonReleased(int button) {
    // iOS touch input - treat as mouse button
    return false;
}

Vector2 IOSTraits::GetMousePosition() {
    // iOS touch position - would need to be connected to touch state
    return {0.0f, 0.0f};
}

Vector2 IOSTraits::GetTouchPosition(int index) {
    // iOS touch position - would need to be connected to touch state
    return {0.0f, 0.0f};
}

// ============================================================================
// AUDIO FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

Sound IOSTraits::LoadSound(const char* fileName) {
    // iOS AVAudioPlayer implementation
    // This would need to be connected to the existing audio system
    return {nullptr, 0};
}

void IOSTraits::PlaySound(Sound sound) {
    // iOS AVAudioPlayer implementation
    // This would need to be connected to the existing audio system
}

Music IOSTraits::LoadMusicStream(const char* fileName) {
    // iOS AVAudioPlayer implementation
    return {nullptr, 0};
}

Music IOSTraits::LoadMusic(const char* fileName) {
    // iOS music loading - same as LoadMusicStream for iOS
    return LoadMusicStream(fileName);
}

void IOSTraits::PlayMusicStream(Music music) {
    // iOS AVAudioPlayer implementation
    // This would need to be connected to the existing audio system
}

void IOSTraits::PlayMusic(Music music) {
    // iOS music playing - same as PlayMusicStream for iOS
    PlayMusicStream(music);
}

// ============================================================================
// MISSING AUDIO FUNCTIONS - IMPLEMENTATIONS
// ============================================================================

void IOSTraits::UnloadSound(Sound sound) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound.player;
    if (player) {
        [player stop];
        sound.player = nil;
    }
}

void IOSTraits::StopSound(Sound sound) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound.player;
    if (player) {
        [player stop];
    }
}

void IOSTraits::PauseSound(Sound sound) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound.player;
    if (player) {
        [player pause];
    }
}

void IOSTraits::ResumeSound(Sound sound) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound.player;
    if (player) {
        [player play];
    }
}

void IOSTraits::SetSoundVolume(Sound sound, float volume) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound.player;
    if (player) {
        player.volume = volume;
    }
}

bool IOSTraits::IsSoundPlaying(Sound sound) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound.player;
    return player && player.isPlaying;
}

void IOSTraits::UnloadMusicStream(Music music) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
    if (player) {
        [player stop];
        music.player = nil;
    }
}

void IOSTraits::UnloadMusic(Music music) {
    UnloadMusicStream(music);
}

void IOSTraits::StopMusicStream(Music music) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
    if (player) {
        [player stop];
    }
}

void IOSTraits::StopMusic(Music music) {
    StopMusicStream(music);
}

void IOSTraits::PauseMusicStream(Music music) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
    if (player) {
        [player pause];
    }
}

void IOSTraits::PauseMusic(Music music) {
    PauseMusicStream(music);
}

void IOSTraits::ResumeMusicStream(Music music) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
    if (player) {
        [player play];
    }
}

void IOSTraits::ResumeMusic(Music music) {
    ResumeMusicStream(music);
}

void IOSTraits::UpdateMusicStream(Music music) {
    // iOS AVAudioPlayer handles updates automatically
    // No manual update needed
}

void IOSTraits::UpdateMusic(Music music) {
    UpdateMusicStream(music);
}

void IOSTraits::SetMusicVolume(Music music, float volume) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
    if (player) {
        player.volume = volume;
    }
}

bool IOSTraits::IsMusicStreamPlaying(Music music) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
    return player && player.isPlaying;
}

bool IOSTraits::IsMusicPlaying(Music music) {
    return IsMusicStreamPlaying(music);
}

void IOSTraits::SetMusicLooping(Music music, bool looping) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
    if (player) {
        player.numberOfLoops = looping ? -1 : 0; // -1 = infinite loops, 0 = no loops
    }
}

float IOSTraits::GetMusicDuration(Music music) {
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.player;
    if (player) {
        return (float)player.duration;
    }
    return 0.0f;
}

// ============================================================================
// UTILITY FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

double IOSTraits::GetTime() {
    // iOS-specific time - use C++ time for now
    // TODO: Implement proper iOS time when needed
    return static_cast<double>(std::time(nullptr));
}

void IOSTraits::TraceLog(int logLevel, const char* text, ...) {
    // iOS-specific logging - use C-style logging for now
    // TODO: Implement proper iOS logging when needed
    (void)logLevel;
    (void)text;
}

int IOSTraits::GetRandomValue(int min, int max) {
    // iOS random implementation
    return min + (arc4random_uniform(max - min + 1));
}

float IOSTraits::GetRandomFloat(float min, float max) {
    // iOS random float implementation
    return min + (static_cast<float>(arc4random_uniform(1000000)) / 1000000.0f) * (max - min);
}

// ============================================================================
// ADDITIONAL UTILITY FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

void IOSTraits::SetTraceLogLevel(int logLevel) {
    // iOS trace log level implementation
    // Store in global state for cross-platform access
    auto& globalState = GlobalStateManager::GetInstance();
    globalState.SetTraceLogLevel(logLevel);
    TraceLog(LOG_INFO, "[IOSTraits] SetTraceLogLevel: Log level set to %d", logLevel);
}

void IOSTraits::SetConfigFlags(unsigned int flags) {
    // iOS config flags implementation
    // Store in global state for cross-platform access
    auto& globalState = GlobalStateManager::GetInstance();
    globalState.SetConfigFlags(flags);
    TraceLog(LOG_INFO, "[IOSTraits] SetConfigFlags: Config flags set to 0x%08X", flags);
}

void IOSTraits::InitWindow(int width, int height, const char* title) {
    // iOS doesn't have traditional windows
}

void IOSTraits::SetWindowSize(int width, int height) {
    // iOS doesn't have traditional windows, but we can update screen metrics
    auto& globalState = GlobalStateManager::GetInstance();
    globalState.UpdateScreenDimensions(width, height);
}

void IOSTraits::ToggleFullscreen() {
    // iOS doesn't have traditional fullscreen toggle
    // This would need to be implemented via orientation changes or view controller
}

void IOSTraits::CloseWindow() {
    // iOS doesn't have traditional windows
}

void IOSTraits::SetRandomSeed(unsigned int seed) {
    // iOS random seed implementation using GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    globalState.SetRandomSeed(seed);
    srand(seed); // Also set C standard library seed for compatibility
    TraceLog(LOG_INFO, "[IOSTraits] SetRandomSeed: Random seed set to %u", seed);
}

Vector2 IOSTraits::GetRandomVector2(Vector2 min, Vector2 max) {
    return {GetRandomFloat(min.x, max.x), GetRandomFloat(min.y, max.y)};
}

Color IOSTraits::GetRandomColor() {
    return {static_cast<unsigned char>(GetRandomValue(0, 255)),
            static_cast<unsigned char>(GetRandomValue(0, 255)),
            static_cast<unsigned char>(GetRandomValue(0, 255)),
            255};
}

Color IOSTraits::ColorAlphaBlend(Color dst, Color src, Color tint) {
    // iOS color alpha blend implementation
    float alpha = tint.a / 255.0f;
    return {
        static_cast<unsigned char>(dst.r * (1.0f - alpha) + src.r * alpha),
        static_cast<unsigned char>(dst.g * (1.0f - alpha) + src.g * alpha),
        static_cast<unsigned char>(dst.b * (1.0f - alpha) + src.b * alpha),
        static_cast<unsigned char>(dst.a * (1.0f - alpha) + src.a * alpha)
    };
}

Color IOSTraits::Fade(Color color, float alpha) {
    return {
        color.r,
        color.g,
        color.b,
        static_cast<unsigned char>(static_cast<float>(color.a) * alpha)
    };
}

// ============================================================================
// ADDITIONAL INPUT FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

Vector2 IOSTraits::GetMouseDelta() {
    // iOS mouse delta using GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.GetMouseDelta();
}

bool IOSTraits::IsPrimaryInputPressed() {
    // iOS primary input pressed using GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.IsPrimaryInputPressed();
}

bool IOSTraits::IsPrimaryInputReleased() {
    // iOS primary input released using GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.IsPrimaryInputReleased();
}

bool IOSTraits::IsPrimaryInputDown() {
    // iOS primary input down using GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.IsPrimaryInputDown();
}

Vector2 IOSTraits::GetPrimaryInputPosition() {
    // iOS primary input position using GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.GetPrimaryInputPosition();
}

// ============================================================================
// WINDOW/SCREEN FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

int IOSTraits::GetScreenWidth() {
    // Get from GlobalStateManager instead of hardcoded values
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.GetScreenWidth();
}

int IOSTraits::GetScreenHeight() {
    // Get from GlobalStateManager instead of hardcoded values
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.GetScreenHeight();
}

float IOSTraits::GetScreenScale() {
    // Get from GlobalStateManager instead of hardcoded values
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.GetScreenScale();
}

Vector2 IOSTraits::GetScreenCenter() {
    // Get from GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.GetScreenCenter();
}

Vector2 IOSTraits::GetRenderScale() {
    // Get from GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.GetRenderScale();
}

Rectangle IOSTraits::GetSafeArea() {
    // Get from GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.GetSafeArea();
}

float IOSTraits::GetScreenDensity() {
    // Get from GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.GetScreenDensity();
}

bool IOSTraits::IsLandscape() {
    // Get from GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.IsLandscape();
}

bool IOSTraits::IsPortrait() {
    // Get from GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    return globalState.IsPortrait();
}

void IOSTraits::SetTargetFPS(int fps) {
    // Update GlobalStateManager
    auto& globalState = GlobalStateManager::GetInstance();
    globalState.UpdateTargetFPS(fps);
    
    // iOS-specific implementation would go here
    // For now, just log the change
    TraceLog(LOG_INFO, "[IOSTraits] SetTargetFPS: %d", fps);
}

void IOSTraits::SetScreenSize(int width, int height) {
    // Update GlobalStateManager when screen size changes
    auto& globalState = GlobalStateManager::GetInstance();
    globalState.UpdateScreenDimensions(width, height);
    
    // iOS-specific implementation would go here
    TraceLog(LOG_INFO, "[IOSTraits] SetScreenSize: %dx%d", width, height);
}

void IOSTraits::SetScreenScale(float scale) {
    // Update GlobalStateManager when screen scale changes
    auto& globalState = GlobalStateManager::GetInstance();
    globalState.UpdateScreenScale(scale);
    
    // iOS-specific implementation would go here
    TraceLog(LOG_INFO, "[IOSTraits] SetScreenScale: %.2f", scale);
}

void IOSTraits::UpdateSafeAreaInsets(float top, float right, float bottom, float left) {
    // Update GlobalStateManager with safe area
    auto& globalState = GlobalStateManager::GetInstance();
    Rectangle safeArea = {left, top, 
                         static_cast<float>(globalState.GetScreenWidth()) - left - right,
                         static_cast<float>(globalState.GetScreenHeight()) - top - bottom};
    globalState.UpdateSafeArea(safeArea);
    
    // iOS-specific implementation would go here
    TraceLog(LOG_INFO, "[IOSTraits] UpdateSafeAreaInsets: %.1f, %.1f, %.1f, %.1f", top, right, bottom, left);
}

// ============================================================================
// VECTOR MATH FUNCTIONS - IMPLEMENTATIONS
// ============================================================================

Vector2 IOSTraits::Vector2Add(Vector2 v1, Vector2 v2) {
    return {v1.x + v2.x, v1.y + v2.y};
}

Vector2 IOSTraits::Vector2Subtract(Vector2 v1, Vector2 v2) {
    return {v1.x - v2.x, v1.y - v2.y};
}

Vector2 IOSTraits::Vector2Scale(Vector2 v, float scale) {
    return {v.x * scale, v.y * scale};
}

float IOSTraits::Vector2Length(Vector2 v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

Vector2 IOSTraits::Vector2Normalize(Vector2 v) {
    float len = IOSTraits::Vector2Length(v);
    return (len > 0) ? Vector2{v.x / len, v.y / len} : Vector2{0, 0};
}

float IOSTraits::Vector2Distance(Vector2 v1, Vector2 v2) {
    return IOSTraits::Vector2Length(IOSTraits::Vector2Subtract(v1, v2));
}

// ============================================================================
// COLLISION DETECTION FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

bool IOSTraits::CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
    return (rec1.x < rec2.x + rec2.width &&
            rec1.x + rec1.width > rec2.x &&
            rec1.y < rec2.y + rec2.height &&
            rec1.y + rec1.height > rec2.y);
}

bool IOSTraits::CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) {
    Vector2 closest = {
        std::max(rec.x, std::min(center.x, rec.x + rec.width)),
        std::max(rec.y, std::min(center.y, rec.y + rec.height))
    };
    return Vector2Distance(center, closest) <= radius;
}

bool IOSTraits::CheckCollisionPointRec(Vector2 point, Rectangle rec) {
    return (point.x >= rec.x && point.x <= rec.x + rec.width &&
            point.y >= rec.y && point.y <= rec.y + rec.height);
}

// ============================================================================
// COLOR FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

Color IOSTraits::ColorAlpha(Color color, float alpha) {
    return {color.r, color.g, color.b, static_cast<unsigned char>(alpha * 255.0f)};
}

Color IOSTraits::ColorLerp(Color color1, Color color2, float amount) {
    return {
        static_cast<unsigned char>(Lerp(static_cast<float>(color1.r), static_cast<float>(color2.r), amount)),
        static_cast<unsigned char>(Lerp(static_cast<float>(color1.g), static_cast<float>(color2.g), amount)),
        static_cast<unsigned char>(Lerp(static_cast<float>(color1.b), static_cast<float>(color2.b), amount)),
        static_cast<unsigned char>(Lerp(static_cast<float>(color1.a), static_cast<float>(color2.a), amount))
    };
}

// ============================================================================
// MATH UTILITY FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

float IOSTraits::Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float IOSTraits::Lerp(float start, float end, float amount) {
    return start + (end - start) * amount;
}

float IOSTraits::Min(float a, float b) {
    return (a < b) ? a : b;
}

float IOSTraits::Max(float a, float b) {
    return (a > b) ? a : b;
}

float IOSTraits::Abs(float value) {
    return fabsf(value);
}

float IOSTraits::Sin(float angle) {
    return sinf(angle);
}

float IOSTraits::Cos(float angle) {
    return cosf(angle);
}

float IOSTraits::Atan2(float y, float x) {
    return atan2f(y, x);
}

float IOSTraits::Sqrt(float value) {
    return sqrtf(value);
}

float IOSTraits::Pow(float base, float exponent) {
    return powf(base, exponent);
}

// ============================================================================
// TIME AND PERFORMANCE FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

float IOSTraits::GetFrameTime() {
    // iOS frame time implementation
    static double lastTime = GetTime();
    double currentTime = GetTime();
    float frameTime = static_cast<float>(currentTime - lastTime);
    lastTime = currentTime;
    return frameTime;
}

int IOSTraits::GetCurrentFPS() {
    // iOS FPS calculation
    static float frameTime = GetFrameTime();
    if (frameTime > 0.0f) {
        return static_cast<int>(1.0f / frameTime);
    }
    return 60; // Default FPS
}

float IOSTraits::GetCurrentFrameTime() {
    return GetFrameTime();
}

// ============================================================================
// RECTANGLE UTILITY FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

Rectangle IOSTraits::RectangleNew(float x, float y, float width, float height) {
    return {x, y, width, height};
}

Rectangle IOSTraits::RectangleFromVector2(Vector2 position, Vector2 size) {
    return {position.x, position.y, size.x, size.y};
}

// ============================================================================
// PLATFORM-SPECIFIC UTILITY FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

bool IOSTraits::IsMobilePlatform() {
    return true; // iOS is always mobile
}

// ============================================================================
// INITIALIZATION AND LIFECYCLE FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

void IOSTraits::Initialize(void* nativeView) {
    // iOS initialization with native view
    // This would need to be connected to the existing initialization system
}

void IOSTraits::Initialize() {
    // iOS initialization
    // This would need to be connected to the existing initialization system
}

void IOSTraits::Shutdown() {
    // iOS shutdown
    // This would need to be connected to the existing shutdown system
}

// ============================================================================
// COMPLEX IMAGE FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

void IOSTraits::ImageResize(Image* image, int newWidth, int newHeight) {
    if (!image || !image->data) return;
    
    // Simple nearest neighbor resize
    Image newImage = {};
    newImage.width = newWidth;
    newImage.height = newHeight;
    newImage.format = image->format;
    newImage.mipmaps = 1;
    
    size_t newDataSize = newWidth * newHeight * 4;
    newImage.data = malloc(newDataSize);
    
    if (newImage.data) {
        unsigned char* srcPixels = (unsigned char*)image->data;
        unsigned char* dstPixels = (unsigned char*)newImage.data;
        
        for (int y = 0; y < newHeight; y++) {
            for (int x = 0; x < newWidth; x++) {
                int srcX = (x * image->width) / newWidth;
                int srcY = (y * image->height) / newHeight;
                int srcIndex = (srcY * image->width + srcX) * 4;
                int dstIndex = (y * newWidth + x) * 4;
                
                dstPixels[dstIndex + 0] = srcPixels[srcIndex + 0];
                dstPixels[dstIndex + 1] = srcPixels[srcIndex + 1];
                dstPixels[dstIndex + 2] = srcPixels[srcIndex + 2];
                dstPixels[dstIndex + 3] = srcPixels[srcIndex + 3];
            }
        }
        
        // Replace old image data
        free(image->data);
        image->data = newImage.data;
        image->width = newWidth;
        image->height = newHeight;
    }
}

void IOSTraits::ImageDraw(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint) {
    if (!dst || !dst->data || !src.data) return;
    
    // Simple implementation - copy pixels from src to dst
    unsigned char* srcPixels = (unsigned char*)src.data;
    unsigned char* dstPixels = (unsigned char*)dst->data;
    
    for (int y = 0; y < (int)dstRec.height; y++) {
        for (int x = 0; x < (int)dstRec.width; x++) {
            int srcX = (int)(srcRec.x + x);
            int srcY = (int)(srcRec.y + y);
            int dstX = (int)(dstRec.x + x);
            int dstY = (int)(dstRec.y + y);
            
            if (srcX >= 0 && srcX < src.width && srcY >= 0 && srcY < src.height &&
                dstX >= 0 && dstX < dst->width && dstY >= 0 && dstY < dst->height) {
                
                int srcIndex = (srcY * src.width + srcX) * 4;
                int dstIndex = (dstY * dst->width + dstX) * 4;
                
                // Apply tint
                dstPixels[dstIndex + 0] = (unsigned char)(srcPixels[srcIndex + 0] * tint.r / 255);
                dstPixels[dstIndex + 1] = (unsigned char)(srcPixels[srcIndex + 1] * tint.g / 255);
                dstPixels[dstIndex + 2] = (unsigned char)(srcPixels[srcIndex + 2] * tint.b / 255);
                dstPixels[dstIndex + 3] = (unsigned char)(srcPixels[srcIndex + 3] * tint.a / 255);
            }
        }
    }
}

Image IOSTraits::ImageCopy(Image image) {
    Image copy = {};
    copy.width = image.width;
    copy.height = image.height;
    copy.format = image.format;
    copy.mipmaps = image.mipmaps;
    
    size_t dataSize = image.width * image.height * 4;
    copy.data = malloc(dataSize);
    if (copy.data && image.data) {
        memcpy(copy.data, image.data, dataSize);
    }
    
    return copy;
}

Image IOSTraits::ImageFromImage(Image image, Rectangle rec) {
    Image subImage = {};
    subImage.width = (int)rec.width;
    subImage.height = (int)rec.height;
    subImage.format = image.format;
    subImage.mipmaps = 1;
    
    size_t dataSize = subImage.width * subImage.height * 4;
    subImage.data = malloc(dataSize);
    
    if (subImage.data) {
        unsigned char* srcPixels = (unsigned char*)image.data;
        unsigned char* dstPixels = (unsigned char*)subImage.data;
        
        for (int y = 0; y < subImage.height; y++) {
            for (int x = 0; x < subImage.width; x++) {
                int srcX = (int)(rec.x + x);
                int srcY = (int)(rec.y + y);
                
                if (srcX >= 0 && srcX < image.width && srcY >= 0 && srcY < image.height) {
                    int srcIndex = (srcY * image.width + srcX) * 4;
                    int dstIndex = (y * subImage.width + x) * 4;
                    
                    dstPixels[dstIndex + 0] = srcPixels[srcIndex + 0];
                    dstPixels[dstIndex + 1] = srcPixels[srcIndex + 1];
                    dstPixels[dstIndex + 2] = srcPixels[srcIndex + 2];
                    dstPixels[dstIndex + 3] = srcPixels[srcIndex + 3];
                }
            }
        }
    }
    
    return subImage;
}

void IOSTraits::ImageFlipVertical(Image* image) {
    if (!image || !image->data) return;
    
    unsigned char* pixels = (unsigned char*)image->data;
    int rowSize = image->width * 4;
    unsigned char* tempRow = (unsigned char*)malloc(rowSize);
    
    for (int y = 0; y < image->height / 2; y++) {
        int topRow = y * rowSize;
        int bottomRow = (image->height - 1 - y) * rowSize;
        
        // Swap rows
        memcpy(tempRow, pixels + topRow, rowSize);
        memcpy(pixels + topRow, pixels + bottomRow, rowSize);
        memcpy(pixels + bottomRow, tempRow, rowSize);
    }
    
    free(tempRow);
}

void IOSTraits::ImageFlipHorizontal(Image* image) {
    if (!image || !image->data) return;
    
    unsigned char* pixels = (unsigned char*)image->data;
    
    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width / 2; x++) {
            int leftPixel = (y * image->width + x) * 4;
            int rightPixel = (y * image->width + (image->width - 1 - x)) * 4;
            
            // Swap pixels
            for (int c = 0; c < 4; c++) {
                unsigned char temp = pixels[leftPixel + c];
                pixels[leftPixel + c] = pixels[rightPixel + c];
                pixels[rightPixel + c] = temp;
            }
        }
    }
}

// ============================================================================
// PLATFORM-SPECIFIC FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================

void IOSTraits::SetOrientation(bool landscape) {
    // iOS-specific orientation setting
    // This would typically call UIKit methods
    TraceLog(LOG_INFO, "[IOSTraits] SetOrientation: %s", landscape ? "landscape" : "portrait");
}

void IOSTraits::ShowVirtualKeyboard(bool show) {
    // iOS-specific keyboard management
    TraceLog(LOG_INFO, "[IOSTraits] ShowVirtualKeyboard: %s", show ? "show" : "hide");
}

void IOSTraits::Vibrate(int milliseconds) {
    // iOS-specific vibration
    TraceLog(LOG_INFO, "[IOSTraits] Vibrate: %d ms", milliseconds);
}

// ============================================================================
// TEXTURE FUNCTIONS - MISSING IMPLEMENTATIONS
// ============================================================================ 