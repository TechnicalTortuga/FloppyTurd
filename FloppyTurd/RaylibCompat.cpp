#include "PlatformLayer.h"
#include <cstdarg>
#include <cstdlib>
#include "RaylibCompat.h"
#include <cstdint>
#include <ctime>
// Removed Metal include to keep this file pure C++
#include <cmath>
#ifdef PLATFORM_IOS
#include "RaylibCompat_iOS.h"
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

bool IsMouseButtonDown(int button) {
#ifdef PLATFORM_IOS
    // TODO: Hook into touch-to-mouse mapping if desired
    return false;
#else
    if (button >= 0 && button < 3) return g_mousePressed[button];
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
        PlatformLayer::GetInstance().Initialize();
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
        return false; // Implement actual logic if needed
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
        // Implement if needed for iOS
    #else
        // Non-iOS implementation will be handled separately
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

// Metal implementation functions

void* LoadTexture_iOS(const char* fileName, int* width, int* height) {
    // Implemented in RaylibCompat_iOS.mm
}

void UnloadTexture_iOS(void* texture) {
    // Implemented in RaylibCompat_iOS.mm
}

void* LoadRenderTexture_iOS(int width, int height) {
    // Implemented in RaylibCompat_iOS.mm
}

void UnloadRenderTexture_iOS(void* texture) {
    // Implemented in RaylibCompat_iOS.mm
}

void BeginDrawing_iOS(void* renderTexture) {
    // Implemented in RaylibCompat_iOS.mm
}

void EndDrawing_iOS(void* renderTexture) {
    // Implemented in RaylibCompat_iOS.mm
}

void DrawRectangle_iOS(int posX, int posY, int width, int height, unsigned int color) {
    // Implemented in RaylibCompat_iOS.mm
}

void DrawText_iOS(const char* text, int posX, int posY, int fontSize, unsigned int color) {
    // Implemented in RaylibCompat_iOS.mm
}

Image LoadImage_iOS(const char* fileName);
void UnloadImage_iOS(Image image);
Image GenImageColor_iOS(int width, int height, Color color);
void ImageDraw_iOS(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);
Texture2D LoadTextureFromImage_iOS(Image image);
void ImageResize_iOS(Image* image, int newWidth, int newHeight);

#ifdef PLATFORM_IOS
Image LoadImage_iOS(const char* fileName) {
    // Implemented in RaylibCompat_iOS.mm
}
void UnloadImage_iOS(Image image) {
    // Implemented in RaylibCompat_iOS.mm
}
Image GenImageColor_iOS(int width, int height, Color color) {
    // Implemented in RaylibCompat_iOS.mm
}
void ImageDraw_iOS(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint) {
    // Implemented in RaylibCompat_iOS.mm
}
Texture2D LoadTextureFromImage_iOS(Image image) {
    // Implemented in RaylibCompat_iOS.mm
}
void ImageResize_iOS(Image* image, int newWidth, int newHeight) {
    // Implemented in RaylibCompat_iOS.mm
}
#else
// Stubs for non-iOS platforms
Image LoadImage_iOS(const char* fileName) {
    Image image = { 0 };
    return image;
}
void UnloadImage_iOS(Image image) {
    // Stub
}
Image GenImageColor_iOS(int width, int height, Color color) {
    Image image = { 0 };
    return image;
}
void ImageDraw_iOS(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint) {
    // Stub
}
Texture2D LoadTextureFromImage_iOS(Image image) {
    Texture2D texture = { 0 };
    return texture;
}
void ImageResize_iOS(Image* image, int newWidth, int newHeight) {
    // Stub
}
#endif
