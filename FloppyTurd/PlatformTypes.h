#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

// Platform-specific type definitions
// This file provides unified type definitions across different platforms
// while maintaining compatibility with raylib and platform-specific implementations

#include <stdint.h>
#include <stdbool.h>

// Forward declaration for GlobalStateManager to avoid circular dependencies
#ifdef __cplusplus
class GlobalStateManager;
#endif

#ifdef PLATFORM_IOS
// iOS platform - Use Swift types for C++ interop compatibility
// Note: These C-style typedefs map to Swift structs defined in SwiftTypes.swift
// This provides a unified type system between C++ and Swift code.
// ============================================================================
// BASIC TYPES (C++ compatible versions of Swift types)
// ============================================================================

// Vector2 - matches SwiftTypes.swift Vector2
typedef struct Vector2 {
    float x;
    float y;
} Vector2;

// Rectangle - matches SwiftTypes.swift Rectangle  
typedef struct Rectangle {
    float x;
    float y;
    float width;
    float height;
} Rectangle;

// Color - matches SwiftTypes.swift RaylibColor
#ifndef COLOR_TYPE_DEFINED
#define COLOR_TYPE_DEFINED
typedef struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;
#endif

// ============================================================================
// CAMERA AND MATRIX TYPES
// ============================================================================

typedef struct Camera2D {
    Vector2 offset;
    Vector2 target;
    float rotation;
    float zoom;
} Camera2D;

// Matrix - matches SwiftTypes.swift Matrix layout
typedef struct Matrix {
    float m0, m4, m8, m12;
    float m1, m5, m9, m13;
    float m2, m6, m10, m14;
    float m3, m7, m11, m15;
} Matrix;

// ============================================================================
// TEXTURE AND RENDERING TYPES (iOS Metal-specific)
// ============================================================================

// Texture2D - matches SwiftTypes.swift Texture2D
typedef struct Texture2D {
    uint32_t id;            // Texture ID for Swift bridge compatibility
    int32_t width;          // Texture base width
    int32_t height;         // Texture base height
    int32_t mipmaps;        // Mipmap levels, 1 by default
    int32_t format;         // Data format (PixelFormat type)
} Texture2D;

// Image - matches SwiftTypes.swift Image (simplified for C++ interop)
typedef struct Image {
    void* data;             // Image data pointer (matches Swift UnsafeMutableRawPointer?)
    int32_t width;
    int32_t height;
    int32_t mipmaps;
    int32_t format;
} Image;

// RenderTexture2D - matches SwiftTypes.swift RenderTexture2D
typedef struct RenderTexture2D {
    uint32_t id;            // Render texture ID
    Texture2D texture;      // Color buffer attachment texture
    Texture2D depth;        // Depth buffer attachment texture
} RenderTexture2D;

// ============================================================================
// FONT AND AUDIO TYPES (iOS-specific implementations)
// ============================================================================

// Font - iOS-specific font structure for C++ interop
typedef struct Font {
    int32_t baseSize;       // Base size (default chars height)
    int32_t glyphCount;     // Number of glyph characters
    int32_t glyphPadding;   // Padding around the glyph characters
    Texture2D texture;      // Texture atlas containing the glyphs
    Rectangle* recs;        // Rectangles in texture for the glyphs
    void* glyphs;           // GlyphInfo* glyphs
} Font;

// Sound - iOS AVAudioPlayer wrapper
typedef struct Sound {
    int32_t id;             // Sound ID for bridge compatibility
    void* player;           // AVAudioPlayer* reference
    int length;             // Sound length in samples
} Sound;

// Music - iOS AVAudioPlayer wrapper for music
typedef struct Music {
    int32_t id;             // Music ID for bridge compatibility
    void* player;           // AVAudioPlayer* reference
    int length;             // Music length in samples
} Music;

// ============================================================================
// COLOR HELPER FUNCTION
// ============================================================================

#ifdef __cplusplus
// Color helper function with default argument (C++ only)
inline Color MakeColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return {r, g, b, a};
}
#else
// C version without default arguments
static inline Color MakeColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    Color color = {r, g, b, a};
    return color;
}
#endif

// ============================================================================
// BASIC COLORS
// ============================================================================

    #define WHITE       MakeColor(255, 255, 255, 255)
    #define BLACK       MakeColor(0, 0, 0, 255)
    #define RED         MakeColor(255, 0, 0, 255)
    #define GREEN       MakeColor(0, 255, 0, 255)
    #define BLUE        MakeColor(0, 0, 255, 255)
    #define YELLOW      MakeColor(255, 255, 0, 255)
    #define ORANGE      MakeColor(255, 165, 0, 255)
    #define PURPLE      MakeColor(128, 0, 128, 255)
    #define GRAY        MakeColor(128, 128, 128, 255)
    #define LIGHTGRAY   MakeColor(200, 200, 200, 255)
    #define DARKGRAY    MakeColor(80, 80, 80, 255)
    #define BROWN       MakeColor(139, 69, 19, 255)
    #define PINK        MakeColor(255, 192, 203, 255)
    #define GOLD        MakeColor(255, 215, 0, 255)
    #define LIME        MakeColor(0, 255, 0, 255)
    #define MAROON      MakeColor(128, 0, 0, 255)
    #define NAVY        MakeColor(0, 0, 128, 255)
    #define DARKGREEN   MakeColor(0, 128, 0, 255)
    #define DARKBLUE    MakeColor(0, 0, 139, 255)
    #define DARKPURPLE  MakeColor(72, 61, 139, 255)
    #define DARKBROWN   MakeColor(101, 67, 33, 255)
    #define BEIGE       MakeColor(245, 245, 220, 255)
    #define MAGENTA     MakeColor(255, 0, 255, 255)
    #define RAYWHITE    MakeColor(245, 245, 245, 255)

    // ============================================================================
    // MATH CONSTANTS (iOS only - for C++ interop)
    // ============================================================================
    
    #define PI          3.14159265358979323846
    #define DEG2RAD     (PI/180.0f)
    #define RAD2DEG     (180.0f/PI)

    // ============================================================================
    // LOGGING CONSTANTS (iOS only - for C++ interop)
    // ============================================================================
    
    #define LOG_ALL     0
    #define LOG_TRACE   1
    #define LOG_DEBUG   2
    #define LOG_INFO    3
    #define LOG_WARNING 4
    #define LOG_ERROR   5
    #define LOG_FATAL   6
    #define LOG_NONE    7

    // ============================================================================
    // CONSTANTS - iOS ONLY (raylib equivalents for C++ interop)
    // ============================================================================
    
    // Mouse buttons
    #define MOUSE_BUTTON_LEFT        0
    #define MOUSE_BUTTON_RIGHT       1
    #define MOUSE_BUTTON_MIDDLE      2
    
    // Window flags
    #define FLAG_VSYNC_HINT          0x00000040
    #define FLAG_FULLSCREEN_MODE     0x00000002
    #define FLAG_WINDOW_RESIZABLE    0x00000004
    #define FLAG_WINDOW_UNDECORATED  0x00000008
    #define FLAG_WINDOW_HIDDEN       0x00000080
    #define FLAG_WINDOW_MINIMIZED    0x00000200
    #define FLAG_WINDOW_MAXIMIZED    0x00000400
    #define FLAG_WINDOW_UNFOCUSED    0x00000800
    #define FLAG_WINDOW_TOPMOST      0x00001000
    #define FLAG_WINDOW_ALWAYS_RUN   0x00000100
    #define FLAG_WINDOW_TRANSPARENT  0x00000010
    #define FLAG_WINDOW_HIGHDPI      0x00002000
    #define FLAG_WINDOW_MOUSE_PASSTHROUGH 0x00004000
    #define FLAG_BORDERLESS_WINDOWED_MODE 0x00008000
    #define FLAG_MSAA_4X_HINT        0x00000020
    #define FLAG_INTERLACED_HINT     0x00010000
    
    // Texture filter constants
    #define TEXTURE_FILTER_POINT     0
    #define TEXTURE_FILTER_BILINEAR  1
    #define TEXTURE_FILTER_TRILINEAR 2
    #define TEXTURE_FILTER_ANISOTROPIC_4X 3
    #define TEXTURE_FILTER_ANISOTROPIC_8X 4
    #define TEXTURE_FILTER_ANISOTROPIC_16X 5
    
    // Texture wrap constants
    #define TEXTURE_WRAP_REPEAT      0
    #define TEXTURE_WRAP_CLAMP       1
    #define TEXTURE_WRAP_MIRROR_REPEAT 2
    #define TEXTURE_WRAP_MIRROR_CLAMP 3
    
    // Pixel format constants
    #define PIXELFORMAT_UNCOMPRESSED_GRAYSCALE    1
    #define PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA   2
    #define PIXELFORMAT_UNCOMPRESSED_R5G6B5       3
    #define PIXELFORMAT_UNCOMPRESSED_R8G8B8       4
    #define PIXELFORMAT_UNCOMPRESSED_R5G5B5A1     5
    #define PIXELFORMAT_UNCOMPRESSED_R4G4B4A4     6
    #define PIXELFORMAT_UNCOMPRESSED_R8G8B8A8     7
    #define PIXELFORMAT_UNCOMPRESSED_R32          8
    #define PIXELFORMAT_UNCOMPRESSED_R32G32B32    9
    #define PIXELFORMAT_UNCOMPRESSED_R32G32B32A32 10
    #define PIXELFORMAT_COMPRESSED_DXT1_RGB       11
    #define PIXELFORMAT_COMPRESSED_DXT1_RGBA      12
    #define PIXELFORMAT_COMPRESSED_DXT3_RGBA      13
    #define PIXELFORMAT_COMPRESSED_DXT5_RGBA      14
    #define PIXELFORMAT_COMPRESSED_ETC1_RGB       15
    #define PIXELFORMAT_COMPRESSED_ETC2_RGB       16
    #define PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA  17
    #define PIXELFORMAT_COMPRESSED_PVRT_RGB       18
    #define PIXELFORMAT_COMPRESSED_PVRT_RGBA      19
    #define PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA  20
    #define PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA  21
    
    // Key codes (iOS only)
    #define KEY_NULL             0
    #define KEY_APOSTROPHE       39
    #define KEY_COMMA            44
    #define KEY_MINUS            45
    #define KEY_PERIOD           46
    #define KEY_SLASH            47
    #define KEY_ZERO             48
    #define KEY_ONE              49
    #define KEY_TWO              50
    #define KEY_THREE            51
    #define KEY_FOUR             52
    #define KEY_FIVE             53
    #define KEY_SIX              54
    #define KEY_SEVEN            55
    #define KEY_EIGHT            56
    #define KEY_NINE             57
    #define KEY_SEMICOLON        59
    #define KEY_EQUAL            61
    #define KEY_A                65
    #define KEY_B                66
    #define KEY_C                67
    #define KEY_D                68
    #define KEY_E                69
    #define KEY_F                70
    #define KEY_G                71
    #define KEY_H                72
    #define KEY_I                73
    #define KEY_J                74
    #define KEY_K                75
    #define KEY_L                76
    #define KEY_M                77
    #define KEY_N                78
    #define KEY_O                79
    #define KEY_P                80
    #define KEY_Q                81
    #define KEY_R                82
    #define KEY_S                83
    #define KEY_T                84
    #define KEY_U                85
    #define KEY_V                86
    #define KEY_W                87
    #define KEY_X                88
    #define KEY_Y                89
    #define KEY_Z                90
    #define KEY_LEFT_BRACKET     91
    #define KEY_BACKSLASH        92
    #define KEY_RIGHT_BRACKET    93
    #define KEY_GRAVE            96
    #define KEY_SPACE            32
    #define KEY_ESCAPE           256
    #define KEY_ENTER            257
    #define KEY_TAB              258
    #define KEY_BACKSPACE        259
    #define KEY_INSERT           260
    #define KEY_DELETE           261
    #define KEY_RIGHT            262
    #define KEY_LEFT             263
    #define KEY_DOWN             264
    #define KEY_UP               265
    #define KEY_PAGE_UP          266
    #define KEY_PAGE_DOWN        267
    #define KEY_HOME             268
    #define KEY_END              269
    #define KEY_CAPS_LOCK        280
    #define KEY_SCROLL_LOCK      281
    #define KEY_NUM_LOCK         282
    #define KEY_PRINT_SCREEN     283
    #define KEY_PAUSE            284
    #define KEY_F1               290
    #define KEY_F2               291
    #define KEY_F3               292
    #define KEY_F4               293
    #define KEY_F5               294
    #define KEY_F6               295
    #define KEY_F7               296
    #define KEY_F8               297
    #define KEY_F9               298
    #define KEY_F10              299
    #define KEY_F11              300
    #define KEY_F12              301
    #define KEY_LEFT_SHIFT       340
    #define KEY_LEFT_CONTROL     341
    #define KEY_LEFT_ALT         342
    #define KEY_LEFT_SUPER       343
    #define KEY_RIGHT_SHIFT      344
    #define KEY_RIGHT_CONTROL    345
    #define KEY_RIGHT_ALT        346
    #define KEY_RIGHT_SUPER      347
    #define KEY_KB_MENU          348
    
    // Gesture constants (iOS only)
    #define GESTURE_NONE       0
    #define GESTURE_TAP        1
    #define GESTURE_DOUBLETAP  2
    #define GESTURE_HOLD       4
    #define GESTURE_DRAG       8
    #define GESTURE_SWIPE_RIGHT 16
    #define GESTURE_SWIPE_LEFT  32
    #define GESTURE_SWIPE_UP    64
    #define GESTURE_SWIPE_DOWN  128
    #define GESTURE_PINCH_IN    256
    #define GESTURE_PINCH_OUT   512

#else
// Desktop platforms - use raylib types directly
// No custom type definitions needed as raylib provides all required types
#include "raylib.h"
#endif

#endif // PLATFORM_TYPES_H