#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS

// ========== METAL/IOS INCLUDES ==========
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreText/CoreText.h>

#include <cmath>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

// ========== TYPE DEFINITIONS ==========

typedef struct Vector2 {
    float x;
    float y;
} Vector2;

typedef struct Rectangle {
    float x;
    float y;
    float width;
    float height;
} Rectangle;

typedef struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

typedef struct Texture2D {
    id<MTLTexture> metalTexture;
    int width;
    int height;
} Texture2D;

typedef struct Image {
    CGImageRef cgImage;
    int width;
    int height;
} Image;

typedef struct Font {
    CTFontRef ctFont;
    int size;
    int glyphCount;
    int baseSize;
    Texture2D texture;
} Font;

typedef struct AudioStream {
    void* buffer;
    unsigned int sampleRate;
    unsigned int sampleSize;
    unsigned int channels;
} AudioStream;

typedef struct Music {
    AVAudioPlayer* player;
    int volume;
    bool looping;
} Music;

typedef struct Sound {
    AVAudioPlayer* player;
    int sampleCount;
    int stream; // Not used, for compatibility
} Sound;

// ========== COLOR CONSTANTS ==========
#define WHITE (Color){ 255, 255, 255, 255 }
#define BLACK (Color){ 0, 0, 0, 255 }
#define RED (Color){ 255, 0, 0, 255 }
#define GREEN (Color){ 0, 255, 0, 255 }
#define BLUE (Color){ 0, 0, 255, 255 }
#define YELLOW (Color){ 255, 255, 0, 255 }
#define GRAY (Color){ 128, 128, 128, 255 }
#define MAROON (Color){ 128, 0, 0, 255 }
#define GOLD (Color){ 255, 203, 0, 255 }
#define MAGENTA (Color){ 255, 0, 255, 255 }
#define BLANK (Color){ 0, 0, 0, 0 }

// ========== CONSTANTS ==========
// Texture constants
#define TEXTURE_WRAP_REPEAT 0
#define TEXTURE_WRAP_CLAMP 1
#define TEXTURE_WRAP_MIRROR_REPEAT 2
#define TEXTURE_WRAP_MIRROR_CLAMP 3

// Texture filter constants
#define TEXTURE_FILTER_POINT 0
#define TEXTURE_FILTER_BILINEAR 1
#define TEXTURE_FILTER_TRILINEAR 2
#define TEXTURE_FILTER_ANISOTROPIC_4X 3
#define TEXTURE_FILTER_ANISOTROPIC_8X 4
#define TEXTURE_FILTER_ANISOTROPIC_16X 5

// Mouse button constants (mapped to touch)
#define MOUSE_BUTTON_LEFT 0
#define MOUSE_BUTTON_RIGHT 1
#define MOUSE_BUTTON_MIDDLE 2
#define MOUSE_LEFT_BUTTON 0
#define MOUSE_RIGHT_BUTTON 1
#define MOUSE_MIDDLE_BUTTON 2

// Key constants (not used on iOS)
#define KEY_NULL 0
#define KEY_SPACE 32
#define KEY_ESCAPE 256
#define KEY_ENTER 257
#define KEY_TAB 258
#define KEY_BACKSPACE 259
#define KEY_A 65
#define KEY_B 66
#define KEY_C 67
#define KEY_D 68
#define KEY_E 69
#define KEY_F 70
#define KEY_G 71
#define KEY_H 72
#define KEY_I 73
#define KEY_J 74
#define KEY_K 75
#define KEY_L 76
#define KEY_M 77
#define KEY_N 78
#define KEY_O 79
#define KEY_P 80
#define KEY_Q 81
#define KEY_R 82
#define KEY_S 83
#define KEY_T 84
#define KEY_U 85
#define KEY_V 86
#define KEY_W 87
#define KEY_X 88
#define KEY_Y 89
#define KEY_Z 90
#define KEY_F1 290
#define KEY_F2 291
#define KEY_F3 292
#define KEY_F4 293
#define KEY_F5 294
#define KEY_F6 295
#define KEY_F7 296
#define KEY_F8 297
#define KEY_F9 298
#define KEY_F10 299
#define KEY_F11 300
#define KEY_F12 301

// Window flags
#define FLAG_WINDOW_RESIZABLE 0x00000004
#define FLAG_VSYNC_HINT 0x00000040
#define FLAG_WINDOW_MAXIMIZED 0x00000200

// Logging levels
#define LOG_ALL 0
#define LOG_TRACE 1
#define LOG_DEBUG 2
#define LOG_INFO 3
#define LOG_WARNING 4
#define LOG_ERROR 5
#define LOG_FATAL 6
#define LOG_NONE 7

// Math constants
#define PI 3.14159265358979323846f
#define RAD2DEG (180.0f/PI)
#define DEG2RAD (PI/180.0f)

// Gesture constants
#define GESTURE_TAP 1
#define GESTURE_DOUBLETAP 2
#define GESTURE_HOLD 4
#define GESTURE_DRAG 8
#define GESTURE_SWIPE_RIGHT 16
#define GESTURE_SWIPE_LEFT 32
#define GESTURE_SWIPE_UP 64
#define GESTURE_SWIPE_DOWN 128
#define GESTURE_PINCH_IN 256
#define GESTURE_PINCH_OUT 512

// ========== FUNCTION DECLARATIONS ==========

// Window functions
void InitWindow(int width, int height, const char* title);
void CloseWindow();
bool WindowShouldClose();
void SetTargetFPS(int fps);
void SetWindowSize(int width, int height);
bool IsWindowFullscreen();
int GetScreenWidth();
int GetScreenHeight();
void BeginDrawing();
void EndDrawing();
void ClearBackground(Color color);
void UpdateSafeAreaInsets(float top, float right, float bottom, float left);
void OnAppPause();
void OnAppResume();

// Texture functions
Texture2D LoadTexture(const char* fileName);
void UnloadTexture(Texture2D texture);
Texture2D LoadTextureFromImage(Image image);
void SetTextureWrap(Texture2D texture, int wrap);
void SetTextureFilter(Texture2D texture, int filter);
void DrawTexture(Texture2D texture, int posX, int posY, Color tint);
void DrawTextureV(Texture2D texture, Vector2 position, Color tint);
void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);

// Image functions
Image LoadImage(const char* fileName);
void UnloadImage(Image image);
Image GenImageColor(int width, int height, Color color);
void ImageResize(Image* image, int newWidth, int newHeight);
void ImageDraw(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);
Image LoadImageFromTexture(Texture2D texture);

// Audio functions
void InitAudioDevice();
void CloseAudioDevice();
Sound LoadSound(const char* fileName);
void UnloadSound(Sound sound);
void PlaySound(Sound sound);
void SetSoundVolume(Sound sound, float volume);

Music LoadMusicStream(const char* fileName);
void UnloadMusicStream(Music music);
void PlayMusicStream(Music music);
void StopMusicStream(Music music);
void PauseMusicStream(Music music);
void ResumeMusicStream(Music music);
void SetMusicVolume(Music music, float volume);
bool IsMusicStreamPlaying(Music music);
void UpdateMusicStream(Music music);

// Input functions
void UpdateTouchState(int touchId, float x, float y, bool is_down);
void ClearAllTouchStates();
Vector2 GetMousePosition();
Vector2 GetMouseDelta();
bool IsMouseButtonPressed(int button);
bool IsMouseButtonReleased(int button);
bool IsMouseButtonDown(int button);

// Multi-touch functions
int GetTouchCount();
bool IsTouchDown(int index);
bool IsTouchPressed(int index);
bool IsTouchReleased(int index);

// Font functions
Font LoadFont(const char* fileName);
Font LoadFontEx(const char* fileName, int fontSize, int* codepoints, int codepointCount);
void UnloadFont(Font font);
Font GetFontDefault();
void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint);
void DrawText(const char* text, int posX, int posY, int fontSize, Color color);

// Drawing functions
void BeginScissorMode(int x, int y, int width, int height);
void EndScissorMode();
void DrawRectangle(int posX, int posY, int width, int height, Color color);
void DrawRectangleRec(Rectangle rec, Color color);
void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);
void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color);
void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
void DrawCircleV(Vector2 center, float radius, Color color);

// Text functions
int MeasureText(const char* text, int fontSize);
Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing);

// Utility functions
Color Fade(Color color, float alpha);
void TraceLog(int logLevel, const char* text, ...);
void SetConfigFlags(unsigned int flags);
void SetExitKey(int key);
void ToggleFullscreen();
void SetWindowPosition(int x, int y);
int GetCurrentMonitor();
int GetMonitorWidth(int monitor);
int GetMonitorHeight(int monitor);

// Math functions
float Lerp(float start, float end, float amount);
int GetRandomValue(int min, int max);

// Vector2 functions
Vector2 Vector2Zero();
Vector2 Vector2One();
Vector2 Vector2Add(Vector2 v1, Vector2 v2);
Vector2 Vector2AddValue(Vector2 v, float add);
Vector2 Vector2Subtract(Vector2 v1, Vector2 v2);
Vector2 Vector2SubtractValue(Vector2 v, float sub);
float Vector2Length(Vector2 v);
float Vector2LengthSqr(Vector2 v);
float Vector2DotProduct(Vector2 v1, Vector2 v2);
float Vector2Distance(Vector2 v1, Vector2 v2);
float Vector2DistanceSqr(Vector2 v1, Vector2 v2);
float Vector2Angle(Vector2 v1, Vector2 v2);
Vector2 Vector2Scale(Vector2 v, float scale);
Vector2 Vector2Multiply(Vector2 v1, Vector2 v2);
Vector2 Vector2Negate(Vector2 v);
Vector2 Vector2Divide(Vector2 v1, Vector2 v2);
Vector2 Vector2Normalize(Vector2 v);
Vector2 Vector2Lerp(Vector2 v1, Vector2 v2, float amount);
Vector2 Vector2Rotate(Vector2 v, float angle);

// Collision functions
bool CheckCollisionPointRec(Vector2 point, Rectangle rec);
bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2);
bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec);

// Time functions
float GetFrameTime();
double GetTime();

// ========== INLINE IMPLEMENTATIONS ==========

// Utility functions
inline float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline Color ColorLerp(Color a, Color b, float t) {
    Color result;
    result.r = (unsigned char)(a.r + (b.r - a.r) * t);
    result.g = (unsigned char)(a.g + (b.g - a.g) * t);
    result.b = (unsigned char)(a.b + (b.b - a.b) * t);
    result.a = (unsigned char)(a.a + (b.a - a.a) * t);
    return result;
}

inline Color ColorAlpha(Color color, float alpha) {
    Color result = color;
    result.a = (unsigned char)(color.a * alpha);
    return result;
}

// Stub functions for iOS
inline float GetMusicTimeLength(Music music) {
    if (music.player) {
        return music.player.duration;
    }
    return 0.0f;
}

inline bool IsKeyPressed(int key) {
    // On iOS, physical keys are not used; always return false
    return false;
}

inline bool IsKeyDown(int key) {
    // On iOS, physical keys are not used; always return false
    return false;
}

// RenderTexture2D for compatibility
typedef struct RenderTexture2D {
    unsigned int id; // Not used on iOS
    Texture2D texture; // Not used on iOS
    int width;
    int height;
} RenderTexture2D;

// Stub functions for render textures
inline void SetWindowIcon(Image image) {
    // Not supported on iOS; app icon is set via Xcode asset catalog
}

inline RenderTexture2D LoadRenderTexture(int width, int height) {
    RenderTexture2D rt = {0};
    rt.width = width;
    rt.height = height;
    return rt;
}

inline void EndTextureMode(void) {
    // Not implemented for iOS/Metal
}

inline Vector2 GetMonitorPosition(int monitor) {
    return (Vector2){0, 0};
}

inline void BeginTextureMode(RenderTexture2D target) {
    // Not implemented for iOS/Metal
}

inline void UnloadRenderTexture(RenderTexture2D target) {
    // Not implemented for iOS/Metal
}

// Drawing stubs
inline void DrawCircle(int centerX, int centerY, float radius, Color color) {
    DrawCircleV((Vector2){(float)centerX, (float)centerY}, radius, color);
}

inline void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) {
    DrawRectangleRoundedLinesEx(rec, 0.0f, 1, lineThick, color);
}

// Platform functions
inline std::string GetApplicationDirectory() {
    // On iOS, resources are in the app bundle
    return "./";
}

inline int GetTouchPointCount() {
    // Will be implemented in .mm file
    return 0;
}

inline Vector2 GetTouchPosition(int index) {
    // Will be implemented in .mm file
    return {0, 0};
}

inline bool IsGestureDetected(int gesture) {
    // Will be implemented in .mm file
    return false;
}

inline Vector2 GetWindowScaleDPI() {
    // Will be implemented in .mm file
    return {1.0f, 1.0f};
}

// TextFormat implementation
inline const char* TextFormat(const char* format, ...) {
    static char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return buffer;
}

#endif // defined(__APPLE__) && TARGET_OS_IOS