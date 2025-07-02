#ifndef AIGUI_H
#define AIGUI_H

#include "raylib.h" 
#include "PlatformLayer.h"  // Add platform layer for input abstraction

#ifdef AIGUI_STATIC
#define AIGUI_DEF static
#elif defined(AIGUI_INLINE)
#define AIGUI_DEF inline
#else
#define AIGUI_DEF extern
#endif

// Standard font sizes for consistency - Adjusted for better readability
const int AIGUI_FONT_SIZE_SMALL = 10;  // Increased from 8
const int AIGUI_FONT_SIZE_MEDIUM = 14; // Increased from 12
const int AIGUI_FONT_SIZE_LARGE = 18;  // Increased from 16

AIGUI_DEF void AIGUI_Init();
AIGUI_DEF void AIGUI_Shutdown();
AIGUI_DEF void AIGUI_BeginFrame();
AIGUI_DEF void AIGUI_EndFrame();
AIGUI_DEF void AIGUI_SetFont(Font font);
AIGUI_DEF void AIGUI_SliderFloat(const char* label, float x, float y, float width, float min, float max, float* value);
AIGUI_DEF bool AIGUI_Button(const char* label, float x, float y, float width, float height);
AIGUI_DEF bool AIGUI_ButtonRounded(const char* label, float x, float y, float width, float height, float radius = 0.2f, int fontSize = 18, Color textColor = WHITE);
AIGUI_DEF void AIGUI_LabelRounded(const char* text, float x, float y, float width, float height, float radius = 0.2f, int fontSize = 18, Color textColor = WHITE, Color bgColor = { 0, 0, 0, 0 }); // Added bgColor with default transparent
AIGUI_DEF bool AIGUI_ImageButton(
    Texture2D textureDefault,
    Texture2D textureHover,
    float x, float y,
    float width, float height,
    const char* text = nullptr,
    int fontSize = 20,
    Color textColor = WHITE,
    Vector2* customMousePos = nullptr
);
AIGUI_DEF bool AIGUI_StateButton(
    Texture2D textureNormal,
    Texture2D textureHover,
    Texture2D textureClicked,
    float x, float y,
    float width, float height,
    const char* text = nullptr,
    int fontSize = 20,
    Color textColor = WHITE,
    Vector2* customMousePos = nullptr
);
AIGUI_DEF Vector2 _GetScaledInputPosition();
AIGUI_DEF Vector2 _GetScaledMousePosition();

struct AIGUI_Context {
    Vector2 mousePos;
    bool mouseLeftDown;
    Font defaultFont = GetFontDefault();
};
extern AIGUI_Context g_AIGUI;

template <typename RenderFunc>
void AIGUI_Container(Rectangle container, Vector2* scrollOffset, RenderFunc childRender) {
    BeginScissorMode(container.x, container.y, container.width, container.height);
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 delta = GetMouseDelta();
        scrollOffset->x -= delta.x;
        scrollOffset->y -= delta.y;
    }
    childRender(*scrollOffset);
    EndScissorMode();
}

#ifdef AIGUI_IMPLEMENTATION

#include "raylib.h"
#include "raymath.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// Define the global AIGUI context
AIGUI_Context g_AIGUI;

AIGUI_DEF void AIGUI_Init() {
    memset(&g_AIGUI, 0, sizeof(g_AIGUI));
    g_AIGUI.defaultFont = GetFontDefault();
}

AIGUI_DEF void AIGUI_SetFont(Font font) {
    g_AIGUI.defaultFont = font;
}

AIGUI_DEF void AIGUI_Shutdown() {}

AIGUI_DEF void AIGUI_BeginFrame() {
    // Use PlatformLayer for unified input handling
    auto& platform = PlatformLayer::GetInstance();
    g_AIGUI.mousePos = _GetScaledInputPosition();
    g_AIGUI.mouseLeftDown = platform.IsPrimaryInputDown();
}

AIGUI_DEF void AIGUI_SliderFloat(const char* label, float x, float y, float width, float min, float max, float* value) {
    auto& platform = PlatformLayer::GetInstance();
    // Draw label above the slider
    DrawTextEx(g_AIGUI.defaultFont, label, Vector2{x, y - 14}, 14, 1.0f, WHITE);
    // Optionally, draw the value to the right
    char valStr[32];
    snprintf(valStr, sizeof(valStr), "%.2f", *value);
    DrawTextEx(g_AIGUI.defaultFont, valStr, Vector2{x + width - 40, y - 14}, 14, 1.0f, YELLOW);
    // Draw the slider bar
    Rectangle slider = { x, y, width, 18 };
    DrawRectangleRec(slider, GRAY);
    float norm = (*value - min) / (max - min);
    float handleX = x + norm * width;
    Rectangle handle = { handleX - 8, y, 16, 18 };
    DrawRectangleRec(handle, WHITE);
    if (CheckCollisionPointRec(g_AIGUI.mousePos, slider) && platform.IsPrimaryInputDown()) {
        float inputX = g_AIGUI.mousePos.x;
        float newNorm = (inputX - x) / width;
        if (newNorm < 0) newNorm = 0;
        if (newNorm > 1) newNorm = 1;
        *value = min + newNorm * (max - min);
    }
}

AIGUI_DEF Vector2 _GetScaledInputPosition() {
    // Get input position from platform layer instead of direct mouse
    auto& platform = PlatformLayer::GetInstance();
    Vector2 input = platform.GetPrimaryInputPosition();
    
    float scaleX = (float)GetScreenWidth() / 320.0f;
    float scaleY = (float)GetScreenHeight() / 180.0f;
    // Invert scaling: map screen coordinates to 320x180 space
    return { input.x / scaleX, input.y / scaleY };
}

// Keep the old function name for compatibility but redirect to new implementation
AIGUI_DEF Vector2 _GetScaledMousePosition() {
    return _GetScaledInputPosition();
}

AIGUI_DEF bool AIGUI_Button(const char* label, float x, float y, float width, float height) {
    Rectangle rect = { x, y, width, height };
    bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, rect);
    
    // Use platform-agnostic input
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();
    
    Color hoverColor = Color(200, 200, 200, 255);
    Color normalColor = Color(255, 128, 0, 255);
    DrawRectangleRec(rect, hovered ? hoverColor : normalColor);
    Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, label, 20.0f, 1.0f);
    float textX = x + (width - size.x) / 2.0f;
    float textY = y + (height - size.y) / 2.0f;
    DrawTextEx(g_AIGUI.defaultFont, label, { textX, textY }, 20.0f, 1.0f, BLACK);
    return clicked;
}

AIGUI_DEF bool AIGUI_ButtonRounded(const char* label, float x, float y, float width, float height, float radius, int fontSize, Color textColor) {
    Rectangle rect = { x, y, width, height };
    bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, rect);
    
    // Use platform-agnostic input
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();

    Color topColor = hovered ? Color{ 255, 200, 70, 255 } : Color{ 255, 180, 50, 255 };
    Color bottomColor = hovered ? Color{ 230, 120, 40, 255 } : Color{ 210, 110, 30, 255 };

    rect.x = (float)(int)rect.x;
    rect.y = (float)(int)rect.y;
    rect.width = (float)(int)rect.width;
    rect.height = (float)(int)rect.height;

    // Scale effect on hover
    float hoverScale = hovered ? 1.05f : 1.0f;
    float scaledW = rect.width * hoverScale;
    float scaledH = rect.height * hoverScale;
    float scaledX = rect.x - (scaledW - rect.width) / 2.0f;
    float scaledY = rect.y - (scaledH - rect.height) / 2.0f;
    Rectangle scaledRect = { scaledX, scaledY, scaledW, scaledH };

    // Step 1: Draw the base rounded rectangle with the top color
    DrawRectangleRounded(scaledRect, radius, 12, topColor);

    // Step 2: Draw the gradient inside the scaled rectangle
    float gradientRadius = radius;
    for (int i = 0; i < (int)height; ++i) {
        float t = (float)i / (height - 1);
        if (height <= 1) t = 0.0f;
        Color c = {
            (unsigned char)Lerp((float)topColor.r, (float)bottomColor.r, t),
            (unsigned char)Lerp((float)topColor.g, (float)bottomColor.g, t),
            (unsigned char)Lerp((float)topColor.b, (float)bottomColor.b, t),
            255
        };
        Rectangle fullGradientRect = { scaledX, scaledY - 1, scaledW + 2, scaledH + 1 };
        BeginScissorMode((int)scaledX, (int)(scaledY + i), (int)scaledW + 1, 1);
        DrawRectangleRounded(fullGradientRect, gradientRadius, 12, c);
        EndScissorMode();
    }

    // Step 3: Draw the outline slightly offset
    Rectangle outlineRect = scaledRect;
    outlineRect.x += 1;
    outlineRect.width -= 1;
    outlineRect.height -= 1;
    DrawRectangleRoundedLinesEx(outlineRect, radius, 12, 1, BLACK);

    // Step 4: Draw the text
    if (label && label[0] != '\0') {
        if (g_AIGUI.defaultFont.baseSize > 0 && g_AIGUI.defaultFont.glyphCount > 0 && g_AIGUI.defaultFont.texture.id != 0) {
            Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, label, (float)fontSize, 1.0f);
            float textX = scaledX + (scaledW - size.x) / 2.0f;
            float textY = scaledY + (scaledH - size.y) / 2.0f;
            textX = (float)(int)textX;
            textY = (float)(int)textY;
            DrawTextEx(g_AIGUI.defaultFont, label, { textX, textY }, (float)fontSize, 1.0f, textColor);
        }
        else {
            DrawRectangle((int)scaledX, (int)scaledY, (int)scaledW, (int)scaledH, RED);
            printf("Error: Invalid font in AIGUI_ButtonRounded\n");
        }
    }

    return clicked;
}

AIGUI_DEF void AIGUI_LabelRounded(const char* text, float x, float y, float width, float height, float radius, int fontSize, Color textColor, Color bgColor) {
    Rectangle rect = { x, y, width, height };

    // Use provided bgColor if not transparent, otherwise use the default gradient
    if (bgColor.a > 0) {
        DrawRectangleRounded(rect, radius, 12, bgColor);
    }
    else {
        Color topColor = Color{ 120, 135, 155, 255 };   // light steel-blue gray
        Color bottomColor = Color{ 75, 85, 105, 255 };  // darker muted base

        rect.x = (float)(int)rect.x;
        rect.y = (float)(int)rect.y;
        rect.width = (float)(int)rect.width;
        rect.height = (float)(int)rect.height;

        DrawRectangleRounded(rect, radius, 12, topColor);

        for (int i = 0; i < (int)height; ++i) {
            float t = (float)i / (height - 1);
            Color c = {
                (unsigned char)Lerp((float)topColor.r, (float)bottomColor.r, t),
                (unsigned char)Lerp((float)topColor.g, (float)bottomColor.g, t),
                (unsigned char)Lerp((float)topColor.b, (float)bottomColor.b, t),
                255
            };
            BeginScissorMode((int)x, (int)(y + i), (int)width + 1, 1);
            DrawRectangleRounded(rect, radius, 12, c);
            EndScissorMode();
        }
    }

    Rectangle outlineRect = rect;
    outlineRect.x += 1;
    outlineRect.width -= 1;
    outlineRect.height -= 1;
    DrawRectangleRoundedLinesEx(outlineRect, radius, 12, 1, BLACK);

    if (text && text[0] != '\0') {
        Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, text, (float)fontSize, 1.0f);
        float textX = x + (width - size.x) / 2.0f;
        float textY = y + (height - size.y) / 2.0f;
        DrawTextEx(g_AIGUI.defaultFont, text, { textX, textY }, (float)fontSize, 1.0f, textColor);
    }
}

AIGUI_DEF bool AIGUI_ImageButton(Texture2D textureDefault, Texture2D textureHover, float x, float y, float width, float height, const char* text, int fontSize, Color textColor, Vector2* customMousePos) {
    Rectangle rect = { x, y, width, height };
    Vector2 inputPos = customMousePos ? *customMousePos : g_AIGUI.mousePos;
    bool hovered = CheckCollisionPointRec(inputPos, rect);
    
    // Use platform-agnostic input
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();
    
    DrawTexturePro(hovered ? textureHover : textureDefault, { 0, 0, (float)textureDefault.width, (float)textureDefault.height }, { x, y, width, height }, { 0, 0 }, 0, WHITE);
    if (text) {
        Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, text, (float)fontSize, 1.0f);
        float textX = x + (width - size.x) / 2.0f;
        float textY = y + (height - size.y) / 2.0f;
        DrawTextEx(g_AIGUI.defaultFont, text, { textX, textY }, (float)fontSize, 1.0f, textColor);
    }
    return clicked;
}

AIGUI_DEF bool AIGUI_StateButton(Texture2D textureNormal, Texture2D textureHover, Texture2D textureClicked, float x, float y, float width, float height, const char* text, int fontSize, Color textColor, Vector2* customMousePos) {
    Rectangle rect = { x, y, width, height };
    Vector2 inputPos = (customMousePos) ? *customMousePos : g_AIGUI.mousePos;
    bool hovered = CheckCollisionPointRec(inputPos, rect);
    
    // Use platform-agnostic input
    auto& platform = PlatformLayer::GetInstance();
    bool pressed = (hovered && platform.IsPrimaryInputDown());
    bool released = (hovered && platform.IsPrimaryInputReleased());
    
    Texture2D tex = pressed ? textureClicked : hovered ? textureHover : textureNormal;
    DrawTexturePro(tex, { 0, 0, (float)tex.width, (float)tex.height }, rect, { 0, 0 }, 0.0f, WHITE);
    if (text) {
        Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, text, (float)fontSize, 1.0f);
        float textX = x + (width - size.x) / 2.0f;
        float textY = y + (height - size.y) / 2.0f;
        DrawTextEx(g_AIGUI.defaultFont, text, { textX, textY }, (float)fontSize, 1.0f, textColor);
    }
    return released;
}

AIGUI_DEF void AIGUI_EndFrame() {}

#endif // AIGUI_IMPLEMENTATION

#endif // AIGUI_H