// RaylibCompat.h
#ifndef RAYLIB_COMPAT_H
#define RAYLIB_COMPAT_H

#include <TargetConditionals.h>

// Define necessary types and structures compatible with raylib but without including raylib headers directly if possible
// This header should be safe for inclusion in both C++ and Objective-C++ files

// Forward declarations or type definitions for raylib-like structures
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

#ifndef COLOR_TYPE_DEFINED
#define COLOR_TYPE_DEFINED
typedef struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;
#endif

typedef struct Texture2D {
    unsigned int id;        // OpenGL texture id
    int width;              // Texture base width
    int height;             // Texture base height
    int mipmaps;            // Mipmap levels, 1 by default
    int format;             // Data format (PixelFormat type)
    void* texture;          // Platform-specific texture reference (e.g., id<MTLTexture> for Metal)
} Texture2D;

typedef struct Image {
    void* data;
    int width;
    int height;
    int mipmaps;
    int format;
} Image;

#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
struct Sound {
    void* player; // Actually an AVAudioPlayer*
    int length;
};

struct Music {
    void* player; // Actually an AVAudioPlayer*
    int length;
};

// Audio functions are now declared in the general section below
#else
typedef struct Sound {
    unsigned int id;        // Sound id
    unsigned int frameCount;// Total number of frames (considering channels)
    void* chunk;            // Platform-specific sound data
} Sound;

typedef struct Music {
    unsigned int id;        // Music id
    unsigned int frameCount;// Total number of frames (considering channels)
    void* music;            // Platform-specific music data
    bool looping;           // Music looping enable
    int ctxType;            // Type of music context (audio codec)
    void *ctxData;          // Audio context data, depends on type
} Music;
#endif

typedef struct Font {
    void* font;
    int baseSize;
    int glyphCount;
    int glyphPadding;
    Texture2D texture;
    Rectangle* recs;
    void* glyphs;
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    void* fontData;
    const void* ctFont; // CTFontRef for iOS (const qualified)
    int size;           // Font size for iOS
#endif
} Font;

// Render texture type
#if !defined(RENDER_TEXTURE_TYPE)
#define RENDER_TEXTURE_TYPE
typedef struct RenderTexture2D {
    unsigned int id;        // OpenGL framebuffer object id
    Texture2D texture;      // Color buffer attachment texture
    Texture2D depth;        // Depth buffer attachment texture
} RenderTexture2D, RenderTexture;
#endif

// Define pixel format constants if needed
#define PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 7

// Texture wrap mode constants
#define TEXTURE_WRAP_REPEAT        0
#define TEXTURE_WRAP_CLAMP         1
#define TEXTURE_WRAP_MIRROR_REPEAT 2
#define TEXTURE_WRAP_MIRROR_CLAMP  3

// Color constants
#define WHITE               Color{ 255, 255, 255, 255 }
#define BLACK               Color{ 0, 0, 0, 255 }
#define BLANK               Color{ 0, 0, 0, 0 }
#define MAGENTA             Color{ 255, 0, 255, 255 }
#define RED                 Color{ 255, 0, 0, 255 }
#define GREEN               Color{ 0, 255, 0, 255 }
#define BLUE                Color{ 0, 0, 255, 255 }
#define YELLOW              Color{ 255, 255, 0, 255 }
#define PURPLE              Color{ 128, 0, 128, 255 }
#define ORANGE              Color{ 255, 165, 0, 255 }
#define BROWN               Color{ 165, 42, 42, 255 }
#define GRAY                Color{ 128, 128, 128, 255 }
#define PINK                Color{ 255, 192, 203, 255 }
#define MAROON              Color{ 128, 0, 0, 255 }
#define GOLD                Color{ 255, 203, 0, 255 }

// Math constants
const float PI = 3.14159265358979323846f;
const float DEG2RAD = PI / 180.0f;
const float RAD2DEG = 180.0f / PI;

// Window flags
#define FLAG_WINDOW_RESIZABLE   0x00000004
#define FLAG_VSYNC_HINT         0x00000040
#define FLAG_WINDOW_MAXIMIZED   0x00000200

// Logging levels
#define LOG_ALL     0
#define LOG_TRACE   1
#define LOG_DEBUG   2
#define LOG_INFO    3
#define LOG_WARNING 4
#define LOG_ERROR   5
#define LOG_FATAL   6
#define LOG_NONE    7

// Mouse buttons
#define MOUSE_LEFT_BUTTON 0
#define MOUSE_RIGHT_BUTTON 1
#define MOUSE_MIDDLE_BUTTON 2
#define MOUSE_BUTTON_LEFT 0
#define MOUSE_BUTTON_RIGHT 1
#define MOUSE_BUTTON_MIDDLE 2

// Keyboard keys
#define KEY_NULL            0
#define KEY_SPACE           32
#define KEY_ENTER           257
#define KEY_ESCAPE          256
// Alphabetic keys
#define KEY_A               65
#define KEY_B               66
#define KEY_C               67
#define KEY_D               68
#define KEY_E               69
#define KEY_F               70
#define KEY_G               71
#define KEY_H               72
#define KEY_I               73
#define KEY_J               74
#define KEY_K               75
#define KEY_L               76
#define KEY_M               77
#define KEY_N               78
#define KEY_O               79
#define KEY_P               80
#define KEY_Q               81
#define KEY_R               82
#define KEY_S               83
#define KEY_T               84
#define KEY_U               85
#define KEY_V               86
#define KEY_W               87
#define KEY_X               88
#define KEY_Y               89
#define KEY_Z               90

// API export macro
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    #define RLAPI                            // iOS doesn't use raylib, so no export needed
#elif defined(_WIN32) && defined(BUILD_LIBTYPE_SHARED)
    #define RLAPI __declspec(dllexport)      // We are building the library as a Win32 shared library (.dll)
#elif defined(_WIN32) && defined(USE_LIBTYPE_SHARED)
    #define RLAPI __declspec(dllimport)      // We are using the library as a Win32 shared library (.dll)
#else
    #define RLAPI                            // We are using or building the library as a static library (or a shared library on other platforms than Windows)
#endif

// Music functions
float GetMusicTimeLength(Music music);
float GetMusicDuration(Music music);

#define KEY_F1              290
#define KEY_F2              291
#define KEY_F3              292
#define KEY_F4              293
#define KEY_F5              294
#define KEY_F6              295
#define KEY_F7              296
#define KEY_F8              297
#define KEY_F9              298
#define KEY_F10             299
#define KEY_F11             300
#define KEY_F12             301

// Gesture definitions
#define GESTURE_NONE        0
#define GESTURE_TAP         1
#define GESTURE_DOUBLETAP   2
#define GESTURE_HOLD        4
#define GESTURE_DRAG        8
#define GESTURE_SWIPE_RIGHT 16
#define GESTURE_SWIPE_LEFT  32
#define GESTURE_SWIPE_UP    64
#define GESTURE_SWIPE_DOWN  128
#define GESTURE_PINCH_IN    256
#define GESTURE_PINCH_OUT   512

// Log level constants (removed - already defined above)

// Function declarations - these should match raylib API signatures but be implemented in platform-specific files
#ifdef __cplusplus
extern "C" {
#endif

void InitWindow(int width, int height, const char* title);
void CloseWindow(void);
bool WindowShouldClose(void);
void SetTargetFPS(int fps);
int GetScreenWidth(void);
int GetScreenHeight(void);
void BeginDrawing(void);
void EndDrawing(void);
void ClearBackground(Color color);
Texture2D LoadTexture(const char* fileName);
Texture2D LoadTextureFromImage(Image image);
void UnloadTexture(Texture2D texture);
void DrawTexture(Texture2D texture, int posX, int posY, Color tint);
void DrawTextureV(Texture2D texture, Vector2 position, Color tint);
void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
void DrawCircle(int centerX, int centerY, float radius, Color color);
void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color);
Color ColorAlpha(Color color, float alpha);
Image LoadImage(const char *fileName);
void UnloadImage(Image image);
Image GenImageColor(int width, int height, Color color);
// Audio functions
void InitAudioDevice();
void CloseAudioDevice();
Sound LoadSound(const char* fileName);
void UnloadSound(Sound sound);
void PlaySound(Sound sound);
void SetSoundVolume(Sound sound, float volume);
Music LoadMusic(const char* fileName);
void UnloadMusic(Music music);
void PlayMusic(Music music);
void PlayMusicLoop(Music music);
void PauseMusic(Music music);
void ResumeMusic(Music music);
void SetMusicVolume(Music music, float volume);
void StopMusic(Music music);
bool IsMusicPlaying(Music music);
void SetLooping(Music music, bool looping);

Font LoadFont(const char* fileName);
Font LoadFontEx(const char* fileName, int fontSize, int* codepoints, int codepointCount);
Font GetFontDefault(void);
void UnloadFont(Font font);
Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing);
void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint);
void DrawText(const char *text, int posX, int posY, int fontSize, Color color);
int MeasureText(const char *text, int fontSize);
const char* TextFormat(const char* text, ...);
void DrawRectangle(int posX, int posY, int width, int height, Color color);
void DrawRectangleRec(Rectangle rec, Color color);
void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);
void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color);
void SetExitKey(int key);
void DrawCircleV(Vector2 center, float radius, Color color);
void ImageResize(Image* image, int newWidth, int newHeight);
int GetRandomValue(int min, int max);
void SetWindowSize(int width, int height);
void EndScissorMode(void);
void SetConfigFlags(unsigned int flags);
// SetMusicVolume is declared in the iOS-specific section above
void SetTextureWrap(Texture2D texture, int wrapMode);
int GetMonitorWidth(int monitor);
Music LoadMusicStream(const char* fileName);
void PlayMusicStream(Music music);
void StopMusicStream(Music music);
void UpdateMusicStream(Music music);
bool IsWindowFullscreen(void);
bool IsMusicStreamPlaying(Music music);
Image LoadImageFromTexture(Texture2D texture);
bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec);
Color Fade(Color color, float alpha);
void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
void TraceLog(int logLevel, const char* text, ...);
void ImageDraw(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);
void BeginScissorMode(int x, int y, int width, int height);
int GetMonitorHeight(int monitor);
int GetCurrentMonitor(void);
void UnloadMusicStream(Music music);
void ToggleFullscreen(void);
void SetTextureFilter(Texture2D texture, int filter);
void SetWindowPosition(int x, int y);
double GetTime(void);
RLAPI void SetWindowIcon(Image image);
RLAPI RenderTexture2D LoadRenderTexture(int width, int height);
RLAPI void UnloadRenderTexture(RenderTexture2D target);
RLAPI void BeginTextureMode(RenderTexture2D target);
RLAPI void EndTextureMode(void);
RLAPI Vector2 GetMonitorPosition(int monitor);
RLAPI Vector2 GetMousePosition(void);

// iOS-specific input handling functions
RLAPI void UpdateSafeAreaInsets(float top, float right, float bottom, float left);
RLAPI void UpdateTouchState(int touchId, float x, float y, bool pressed);
RLAPI void ClearAllTouchStates(void);

// iOS lifecycle callbacks
RLAPI void OnAppPause(void);
RLAPI void OnAppResume(void);

// iOS game initialization
RLAPI int game_main(int argc, char *argv[]);

// === Utility constants ===
#ifndef PI
#define PI 3.14159265358979323846f
#endif
#ifndef DEG2RAD
#define DEG2RAD (PI/180.0f)
#endif

// Texture filter constants (match raylib)
#ifndef TEXTURE_FILTER_POINT
#define TEXTURE_FILTER_POINT 0
#endif
#ifndef TEXTURE_FILTER_BILINEAR
#define TEXTURE_FILTER_BILINEAR 1
#endif
#ifndef TEXTURE_FILTER_TRILINEAR
#define TEXTURE_FILTER_TRILINEAR 2
#endif
#ifndef TEXTURE_FILTER_ANISOTROPIC_4X
#define TEXTURE_FILTER_ANISOTROPIC_4X 3
#endif
#ifndef TEXTURE_FILTER_ANISOTROPIC_8X
#define TEXTURE_FILTER_ANISOTROPIC_8X 4
#endif
#ifndef TEXTURE_FILTER_ANISOTROPIC_16X
#define TEXTURE_FILTER_ANISOTROPIC_16X 5
#endif

// Utility math helpers
float Clamp(float value, float min, float max);
Color ColorLerp(Color a, Color b, float t);

// Mouse input functions
bool IsMouseButtonDown(int button);
bool IsMouseButtonReleased(int button);
bool IsKeyPressed(int key);
bool IsKeyDown(int key);
Vector2 GetMouseDelta(void);
float GetFrameTime(void);
bool CheckCollisionPointRec(Vector2 point, Rectangle rec);
Vector2 Vector2Add(Vector2 v1, Vector2 v2);
Vector2 Vector2Subtract(Vector2 v1, Vector2 v2);
Vector2 Vector2Scale(Vector2 v, float scale);
float Vector2Distance(Vector2 v1, Vector2 v2);
float Vector2Length(Vector2 v);
Vector2 Vector2Normalize(Vector2 v);
Texture2D GetTextureRegion(Texture2D texture, Rectangle region);
bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2);
void ImageToPOT(Image *image, Color fill);
void ImageFormat(Image *image, int newFormat);
bool ExportImage(Image image, const char *fileName);

// Agnostic FPS/frame time bridge
int GetCurrentFPS();
float GetCurrentFrameTime();

#ifdef __cplusplus
}

// C++ overloads (not exposed to C linkage)
void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);

// C++ Game instance access (for iOS)
class Game;
Game* GetGameInstance();
void SetGameInstance(Game* instance);

#endif

#endif // RAYLIB_COMPAT_H
