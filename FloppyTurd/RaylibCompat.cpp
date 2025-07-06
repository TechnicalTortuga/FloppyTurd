#include "PlatformLayer.h"
#include <cstdarg>
#include <cstdlib>
#include "RaylibCompat.h"
#include "Game.h"
#include <cstdint>
#include <ctime>
// Removed Metal include to keep this file pure C++
#include <cmath>
#ifdef PLATFORM_IOS
#include "RaylibCompat_iOS.h"
#include "PlatformLayer.h"  // Make sure PlatformLayer is available for iOS
#endif

#ifdef PLATFORM_IOS
// iOS-specific implementations are compiled separately in RaylibCompat_iOS.mm
#endif

// Global state
static bool g_shouldClose = false;
static int g_targetFPS = 60;
static uint32_t g_frameStartTime = 0;
static Vector2 g_mousePosition = {0, 0};
static Vector2 g_mouseDelta = {0, 0};
static bool g_mousePressed[3] = {false, false, false};
static bool g_mouseReleased[3] = {false, false, false};
static Font g_defaultFont = {nullptr, 16};
static bool g_audioInitialized = false;
static bool g_windowFullscreen = false;
static double g_startTime = 0.0;
static double g_lastFrameTime = 0.0;



// ========== UTILITY MATH FUNCTIONS ==========

static inline Vector2 MakeVector2(float x, float y) { Vector2 v; v.x = x; v.y = y; return v; }

Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
    return MakeVector2(v1.x + v2.x, v1.y + v2.y);
}

Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
    return MakeVector2(v1.x - v2.x, v1.y - v2.y);
}

Vector2 Vector2Scale(Vector2 v, float scale) {
    return MakeVector2(v.x * scale, v.y * scale);
}

float Vector2Length(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }

// Clamp value between min and max
float Clamp(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// Linearly interpolate between two colors
Color ColorLerp(Color a, Color b, float t) {
    Color result;
    result.r = (unsigned char)(a.r + (b.r - a.r) * t);
    result.g = (unsigned char)(a.g + (b.g - a.g) * t);
    result.b = (unsigned char)(a.b + (b.b - a.b) * t);
    result.a = (unsigned char)(a.a + (b.a - a.a) * t);
    return result;
}

float Vector2Distance(Vector2 v1, Vector2 v2) { return Vector2Length(Vector2Subtract(v2, v1)); }

Vector2 Vector2Normalize(Vector2 v) {
    float len = Vector2Length(v);
    if (len < 0.0001f) return MakeVector2(0.0f, 0.0f);
    float inv = 1.0f / len;
    return MakeVector2(v.x * inv, v.y * inv);
}

// ========== MOUSE INPUT FUNCTIONS ==========

bool IsMouseButtonDown(int button)
{
#ifdef PLATFORM_IOS
    // On iOS, treat touch as left mouse button
    if (button == 0) {
        return PlatformLayer::GetInstance().IsPrimaryInputPressed();
    }
    return false;
#else
    return g_mousePressed[button];
#endif
}

bool IsMouseButtonReleased(int button)
{
#ifdef PLATFORM_IOS
    // On iOS, treat touch release as left mouse button release
    if (button == 0) {
        // For now, stub returns false - could implement touch release detection
        return false;
    }
    return false;
#else
    if (button >= 0 && button < 3) {
        return g_mouseReleased[button];
    }
    return false;
#endif
}

bool IsKeyPressed(int key)
{
#ifdef PLATFORM_IOS
    // iOS doesn't have keyboard input - stub
    (void)key;
    return false;
#else
    // Non-iOS implementation placeholder
    (void)key;
    return false;
#endif
}

bool IsKeyDown(int key)
{
#ifdef PLATFORM_IOS
    // iOS doesn't have keyboard input - stub
    (void)key;
    return false;
#else
    // Non-iOS implementation placeholder
    (void)key;
    return false;
#endif
}

Vector2 GetMouseDelta() {
#ifdef PLATFORM_IOS
    // TODO: Compute delta from touch gesture if desired
    return MakeVector2(0.0f, 0.0f);
#else
    return g_mouseDelta;
#endif
}

// ========== WINDOW FUNCTIONS ==========

void InitWindow(int width, int height, const char* title) {
    #ifdef PLATFORM_IOS
        PlatformLayer::GetInstance().Initialize(nullptr);
    #else
        // Non-iOS implementation will be handled separately
    #endif
}

void CloseWindow() {
    #ifdef PLATFORM_IOS
        PlatformLayer::GetInstance().Shutdown();
    #else
        // Non-iOS implementation will be handled separately
    #endif
}

bool WindowShouldClose() {
    #ifdef PLATFORM_IOS
        // For iOS, we never want to close the app automatically.
        // The OS handles app lifecycle events, and we'll respond to those.
        return false;
    #else
        // Non-iOS implementation will be handled separately
        return g_shouldClose;
    #endif
}

void SetTargetFPS(int fps) {
    #ifdef PLATFORM_IOS
        // Implement if needed for iOS
    #else
        // Non-iOS implementation will be handled separately
        g_targetFPS = fps;
    #endif
}

int GetScreenWidth() {
    #ifdef PLATFORM_IOS
        return PlatformLayer::GetInstance().GetScreenWidth();
    #else
        // Non-iOS implementation will be handled separately
        return 0;
    #endif
}

int GetScreenHeight() {
    #ifdef PLATFORM_IOS
        return PlatformLayer::GetInstance().GetScreenHeight();
    #else
        // Non-iOS implementation will be handled separately
        return 0;
    #endif
}

void SetWindowIcon(Image image)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // Stub for iOS Metal implementation
    // No window icon support on iOS
#else
    raylib::SetWindowIcon(image);
#endif
}

// ========== DRAWING FUNCTIONS ==========

void BeginDrawing() {
    #ifdef PLATFORM_IOS
        PlatformLayer::GetInstance().BeginDrawing(nullptr);
    #else
        // Non-iOS implementation will be handled separately
    #endif
}

void EndDrawing() {
    #ifdef PLATFORM_IOS
        PlatformLayer::GetInstance().EndDrawing(nullptr);
    #else
        // Non-iOS implementation will be handled separately
    #endif
}

void ClearBackground(Color color) {
    #ifdef PLATFORM_IOS
        // Draw a full-screen rectangle with the specified color
        auto& platform = PlatformLayer::GetInstance();
        unsigned int colorInt = ((unsigned int)color.r << 24) | ((unsigned int)color.g << 16) | ((unsigned int)color.b << 8) | (unsigned int)color.a;
        platform.DrawRectangle(0, 0, 320, 180, colorInt); // Use game resolution
    #else
        // Non-iOS implementation will be handled separately
    #endif
}

// ========== RENDER TEXTURE FUNCTIONS ==========

RenderTexture2D LoadRenderTexture(int width, int height)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // Stub for iOS Metal implementation
    RenderTexture2D result = {0};
    result.texture.width = width;
    result.texture.height = height;
    return result;
#else
    return raylib::LoadRenderTexture(width, height);
#endif
}

void BeginTextureMode(RenderTexture2D target)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // Stub for iOS Metal implementation
#else
    raylib::BeginTextureMode(target);
#endif
}

void EndTextureMode(void)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // Stub for iOS Metal implementation
#else
    raylib::EndTextureMode();
#endif
}

void UnloadRenderTexture(RenderTexture2D target)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // TODO: Hook into PlatformLayer when Metal render textures are supported
    (void)target;
#else
    raylib::UnloadRenderTexture(target);
#endif
}

// ========== TEXTURE FUNCTIONS ==========

void SetTextureFilter(Texture2D texture, int filter)
{
    // Currently a no-op on iOS; filtering handled at sprite creation time.
    // Stubbed so code that sets filters compiles.
    (void)texture;
    (void)filter;
}

Texture2D LoadTexture(const char *fileName)
{
#ifdef PLATFORM_IOS
    return LoadTexture_iOS(fileName);
#else
    // Non-iOS implementation will be handled separately
    Texture2D texture = { 0 };
    return texture;
#endif
}

void UnloadTexture(Texture2D texture)
{
#ifdef PLATFORM_IOS
    UnloadTexture_iOS(texture);
#else
    // Non-iOS implementation will be handled separately
#endif
}

Texture2D LoadTextureFromImage(Image image)
{
#ifdef PLATFORM_IOS
    return LoadTextureFromImage_iOS(image);
#else
    // Non-iOS implementation will be handled separately
    Texture2D texture = { 0 };
    return texture;
#endif
}

void DrawTexture(Texture2D texture, int posX, int posY, Color tint)
{
#ifdef PLATFORM_IOS
    DrawTexture_iOS(texture, posX, posY, tint);
#else
    // Non-iOS implementation will be handled separately
#endif
}

void DrawTextureV(Texture2D texture, Vector2 position, Color tint)
{
#ifdef PLATFORM_IOS
    DrawTextureV_iOS(texture, position, tint);
#else
    // Non-iOS implementation will be handled separately
#endif
}

void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint)
{
#ifdef PLATFORM_IOS
    DrawTextureEx_iOS(texture, position, rotation, scale, tint);
#else
    // Non-iOS implementation will be handled separately
#endif
}

void DrawTextureRec(Texture2D texture, Rectangle source, Rectangle dest, Color tint)
{
#ifdef PLATFORM_IOS
    DrawTextureRec_iOS(texture, source, dest, tint);
#else
    // Non-iOS implementation will be handled separately
#endif
}

void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint)
{
    Rectangle dest = { position.x, position.y, source.width, source.height };
    DrawTextureRec(texture, source, dest, tint);
}

void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint)
{
#ifdef PLATFORM_IOS
    DrawTexturePro_iOS(texture, source, dest, origin, rotation, tint);
#else
    // Non-iOS implementation will be handled separately
#endif
}

Texture2D GetTextureRegion(Texture2D texture, Rectangle region) {
    Texture2D result = texture;
    result.width = (int)region.width;
    result.height = (int)region.height;
    result.id = texture.id;
    result.format = texture.format;
    result.mipmaps = texture.mipmaps;


    return result;
}

// ========== IMAGE FUNCTIONS ==========

Image LoadImage(const char* fileName)
{
#ifdef PLATFORM_IOS
    return LoadImage_iOS(fileName);
#else
    // Non-iOS implementation will be handled separately
    Image image = { 0 };
    return image;
#endif
}

void UnloadImage(Image image)
{
#ifdef PLATFORM_IOS
    UnloadImage_iOS(image);
#else
    // Non-iOS implementation will be handled separately
#endif
}

Image GenImageColor(int width, int height, Color color)
{
#ifdef PLATFORM_IOS
    return GenImageColor_iOS(width, height, color);
#else
    // Non-iOS implementation will be handled separately
    Image image = { 0 };
    return image;
#endif
}

void ImageToPOT(Image *image, Color fill) {
    #ifdef PLATFORM_IOS
        // Implement if needed for iOS
    #else
        // Non-iOS implementation will be handled separately
    #endif
}

void ImageFormat(Image *image, int newFormat) {
    #ifdef PLATFORM_IOS
        // Implement if needed for iOS
    #else
        // Non-iOS implementation will be handled separately
        if (image) {
            image->format = newFormat;
        }
    #endif
}

bool ExportImage(Image image, const char *fileName) {
    #ifdef PLATFORM_IOS
        // Implement if needed for iOS
    #else
        // Non-iOS implementation will be handled separately
        return false;
    #endif
}

void ImageDraw(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint)
{
#ifdef PLATFORM_IOS
    ImageDraw_iOS(dst, src, srcRec, dstRec, tint);
#else
    // Non-iOS implementation will be handled separately
#endif
}

void ImageResize(Image* image, int newWidth, int newHeight)
{
#ifdef PLATFORM_IOS
    ImageResize_iOS(image, newWidth, newHeight);
#else
    // Non-iOS implementation will be handled separately
#endif
}

// ========== TEXT FUNCTIONS ==========

const char* TextFormat(const char* text, ...)
{
    static char buffer[1024];
    va_list args;
    va_start(args, text);
    vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);
    return buffer;
}

// ========== PRIMITIVE DRAWING FUNCTIONS ==========

void DrawCircle(int centerX, int centerY, float radius, Color color) {
#ifdef PLATFORM_IOS
    // TODO: Implement iOS Metal circle drawing
    (void)centerX; (void)centerY; (void)radius; (void)color;
#else
    // Stub for other platforms
    (void)centerX; (void)centerY; (void)radius; (void)color;
#endif
}

void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) {
#ifdef PLATFORM_IOS
    // TODO: Implement iOS Metal rectangle lines drawing
    (void)rec; (void)lineThick; (void)color;
#else
    // Stub for other platforms
    (void)rec; (void)lineThick; (void)color;
#endif
}

// ========== COLOR FUNCTIONS ==========

Color ColorAlpha(Color color, float alpha) {
    Color result = color;
    result.a = (unsigned char)(255.0f * alpha);
    return result;
}

// ========== MUSIC FUNCTIONS ==========

float GetMusicTimeLength(Music music)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // Hardcoded duration for end credits (2:03) since no native Metal support
    return 123.0f;
#else
    return raylib::GetMusicTimeLength(music);
#endif
}

Vector2 GetMonitorPosition(int monitor)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // Stub for iOS Metal implementation
    return (Vector2){0, 0};
#else
    return raylib::GetMonitorPosition(monitor);
#endif
}

// ========== INPUT FUNCTIONS ==========

Vector2 GetMousePosition(void)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // Stub for iOS Metal implementation
    return (Vector2){0, 0};
#else
    return raylib::GetMousePosition();
#endif
}

// ========== IOS INPUT HANDLING FUNCTIONS ==========

void UpdateSafeAreaInsets(float top, float right, float bottom, float left)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // Stub for iOS Metal implementation
    // Store safe area insets if needed
#endif
}

void UpdateTouchState(int touchId, float x, float y, bool pressed)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // Stub for iOS Metal implementation
    // Update touch state for given touch ID
#endif
}

void ClearAllTouchStates(void)
{
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // Stub for iOS Metal implementation
    // Clear all touch states
#endif
}

// ========== MISSING RAYLIB FUNCTIONS ==========

// Color functions
Color Fade(Color color, float alpha) {
    Color result = color;
    result.a = (unsigned char)(255.0f * Clamp(alpha, 0.0f, 1.0f));
    return result;
}

// Monitor functions
int GetCurrentMonitor() {
    return 0; // Default monitor
}

int GetMonitorWidth(int monitor) {
    auto& platform = PlatformLayer::GetInstance();
    return platform.GetScreenWidth();
}

int GetMonitorHeight(int monitor) {
    auto& platform = PlatformLayer::GetInstance();
    return platform.GetScreenHeight();
}

// GetMonitorPosition already defined above - removed duplicate

// Font functions
Font GetFontDefault() {
    return g_defaultFont;
}

Font LoadFont(const char* fileName) {
    Font font = {nullptr, 16};
    return font; // Stub - would load actual font
}

void UnloadFont(Font font) {
    // Stub - would unload font resources
}

// Timing functions
float GetFrameTime() {
    return 1.0f / 60.0f; // Default 60 FPS
}

double GetTime() {
    if (g_startTime == 0.0) {
        g_startTime = clock() / (double)CLOCKS_PER_SEC;
    }
    return (clock() / (double)CLOCKS_PER_SEC) - g_startTime;
}

// Random functions
int GetRandomValue(int min, int max) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    return min + (rand() % (max - min + 1));
}

// Audio functions (stubs)
void InitAudioDevice() {
    g_audioInitialized = true;
}

void CloseAudioDevice() {
    g_audioInitialized = false;
}

bool IsMusicStreamPlaying(Music music) {
    return false; // Stub
}

void PlayMusicStream(Music music) {
    // Stub
}

void StopMusicStream(Music music) {
    // Stub
}

void UpdateMusicStream(Music music) {
    // Stub
}

void SetMusicVolume(Music music, float volume) {
    // Stub
}

Music LoadMusicStream(const char* fileName) {
    Music music = {0};
    return music; // Stub
}

void UnloadMusicStream(Music music) {
    // Stub
}

Sound LoadSound(const char* fileName) {
    Sound sound = {0};
    return sound; // Stub
}

void UnloadSound(Sound sound) {
    // Stub
}

void PlaySound(Sound sound) {
    // Stub
}

void SetSoundVolume(Sound sound, float volume) {
    // Stub
}

// Window functions
bool IsWindowFullscreen() {
    return g_windowFullscreen;
}

void ToggleFullscreen() {
    g_windowFullscreen = !g_windowFullscreen;
}

void SetConfigFlags(unsigned int flags) {
    // Stub - would set window configuration flags
}

void SetExitKey(int key) {
    // Stub - would set the exit key
}

void SetWindowPosition(int x, int y) {
    // Stub - would set window position
}

void SetWindowSize(int width, int height) {
    // Stub - would resize window
}

// Text functions
int MeasureText(const char* text, int fontSize) {
    if (!text) return 0;
    return strlen(text) * (fontSize / 2); // Rough approximation
}

void TraceLog(int logLevel, const char* text, ...) {
    va_list args;
    va_start(args, text);
    vprintf(text, args);
    va_end(args);
    printf("\n");
}

// Texture functions
void SetTextureWrap(Texture2D texture, int wrap) {
    // Stub - would set texture wrapping mode
}

Image LoadImageFromTexture(Texture2D texture) {
    Image image = {0};
    return image; // Stub
}

// Drawing functions
void DrawRectangleRec(Rectangle rec, Color color) {
    auto& platform = PlatformLayer::GetInstance();
    // Convert Color to unsigned int (RGBA format)
    unsigned int colorInt = ((unsigned int)color.r << 24) | ((unsigned int)color.g << 16) | ((unsigned int)color.b << 8) | (unsigned int)color.a;
    platform.DrawRectangle((int)rec.x, (int)rec.y, (int)rec.width, (int)rec.height, colorInt);
}

void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
    // Fallback to regular rectangle for now
    DrawRectangleRec(rec, color);
}

void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
    auto& platform = PlatformLayer::GetInstance();
    // Cast font texture to void* or use nullptr if not available
    void* fontPtr = (font.texture.id != 0) ? (void*)&font.texture : nullptr;
    platform.DrawText(text, position.x, position.y, fontSize, tint, fontPtr);
}

// Collision functions
bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
    return (point.x >= rec.x) && (point.x < (rec.x + rec.width)) &&
           (point.y >= rec.y) && (point.y < (rec.y + rec.height));
}

bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) {
    float dx = center.x - Clamp(center.x, rec.x, rec.x + rec.width);
    float dy = center.y - Clamp(center.y, rec.y, rec.y + rec.height);
    return (dx * dx + dy * dy) <= (radius * radius);
}

bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
    return (rec1.x < (rec2.x + rec2.width) && (rec1.x + rec1.width) > rec2.x) &&
           (rec1.y < (rec2.y + rec2.height) && (rec1.y + rec1.height) > rec2.y);
}

// Additional drawing functions
void DrawCircleV(Vector2 center, float radius, Color color) {
    // Stub - would draw a circle
    auto& platform = PlatformLayer::GetInstance();
    // Draw as a rectangle for now (simplified)
    unsigned int colorInt = ((unsigned int)color.r << 24) | ((unsigned int)color.g << 16) | ((unsigned int)color.b << 8) | (unsigned int)color.a;
    platform.DrawRectangle((int)(center.x - radius), (int)(center.y - radius), (int)(radius * 2), (int)(radius * 2), colorInt);
}

void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) {
    // Stub - would draw a line
    // For now, draw a thin rectangle approximating the line
    auto& platform = PlatformLayer::GetInstance();
    unsigned int colorInt = ((unsigned int)color.r << 24) | ((unsigned int)color.g << 16) | ((unsigned int)color.b << 8) | (unsigned int)color.a;
    int width = abs(endPosX - startPosX);
    int height = abs(endPosY - startPosY);
    if (width == 0) width = 1;
    if (height == 0) height = 1;
    platform.DrawRectangle(startPosX, startPosY, width, height, colorInt);
}

void DrawRectangle(int posX, int posY, int width, int height, Color color) {
    auto& platform = PlatformLayer::GetInstance();
    unsigned int colorInt = ((unsigned int)color.r << 24) | ((unsigned int)color.g << 16) | ((unsigned int)color.b << 8) | (unsigned int)color.a;
    platform.DrawRectangle(posX, posY, width, height, colorInt);
}

void DrawText(const char* text, int posX, int posY, int fontSize, Color color) {
    auto& platform = PlatformLayer::GetInstance();
    void* fontPtr = nullptr; // Use default font
    platform.DrawText(text, (float)posX, (float)posY, (float)fontSize, color, fontPtr);
}

// Text measurement functions
Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
    if (!text) return {0.0f, 0.0f};
    float width = strlen(text) * (fontSize * 0.6f); // Rough approximation
    float height = fontSize;
    return {width, height};
}

// iOS lifecycle callbacks
void OnAppPause()
{
#ifdef PLATFORM_IOS
    Game* gameInstance = GetGameInstance();
    if (gameInstance && gameInstance->IsInitialized()) {
        gameInstance->OnPause();
    }
#endif
}

void OnAppResume()
{
#ifdef PLATFORM_IOS
    Game* gameInstance = GetGameInstance();
    if (gameInstance && gameInstance->IsInitialized()) {
        gameInstance->OnResume();
    }
#endif
}


