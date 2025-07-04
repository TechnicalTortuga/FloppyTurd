#define AIGUI_IMPLEMENTATION
#include "AIGUI.h"

#if defined(__APPLE__) && defined(TARGET_OS_IOS) && defined(USE_METAL_RENDERER)
#include "MetalRaylibCompat.h"
#endif

// The rest of the implementation from AIGUI.h will go here
#include "RaylibCompat.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// Define the global AIGUI context
AIGUI_Context g_AIGUI;

AIGUI_DEF void AIGUI_Init() {
    memset(&g_AIGUI, 0, sizeof(g_AIGUI));
    g_AIGUI.defaultFont = GetFontDefault();
    
    // Detect mobile platform and set appropriate scaling
#ifdef PLATFORM_MOBILE
    g_AIGUI.isMobile = true;
    
    // Get screen density from platform layer
    auto& platform = PlatformLayer::GetInstance();
    float density = platform.GetScreenScale();
    
    // Set UI scale based on screen density
    g_AIGUI.uiScale = fmaxf(1.0f, density * 0.8f);  // Scale but not too aggressively
    g_AIGUI.touchTargetScale = 1.2f;  // Slightly larger touch targets on mobile
    
    // Initialize safe area
    g_AIGUI.safeArea = platform.GetSafeArea();
    
    TraceLog(LOG_INFO, "AIGUI: Mobile platform detected, UI scale: %.2f, density: %.2f", 
             g_AIGUI.uiScale, density);
#else
    g_AIGUI.isMobile = false;
    g_AIGUI.uiScale = 1.0f;
    g_AIGUI.touchTargetScale = 1.0f;
    
    // On desktop, safe area is the full screen
    g_AIGUI.safeArea = Rectangle{ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
    
    TraceLog(LOG_INFO, "AIGUI: Desktop platform detected");
#endif
}

AIGUI_DEF void AIGUI_SetFont(Font font) {
    g_AIGUI.defaultFont = font;
}

AIGUI_DEF void AIGUI_Shutdown() {}

AIGUI_DEF void AIGUI_BeginFrame() {
    // Mouse position is now set by the Game class with proper letterboxing
    // No need to recalculate here - just get button state
    auto& platform = PlatformLayer::GetInstance();
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
    // This function is now obsolete since Game.cpp handles coordinate mapping
    // Return the already-scaled position from the global context
    return g_AIGUI.mousePos;
}

// Keep the old function name for compatibility but redirect to new implementation
AIGUI_DEF Vector2 _GetScaledMousePosition() {
    return g_AIGUI.mousePos;
}

AIGUI_DEF bool AIGUI_Button(const char* label, float x, float y, float width, float height) {
    Rectangle rect = { x, y, width, height };
    
    bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, rect);
    
    // Use platform-agnostic input
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();
    
    // Simple visual states
    Color bgColor = hovered ? Color{255, 160, 0, 255} : Color{255, 128, 0, 255};
    DrawRectangleRec(rect, bgColor);
    
    Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, label, 20.0f, 1.0f);
    float textX = x + (width - size.x) / 2.0f;
    float textY = y + (height - size.y) / 2.0f;
    DrawTextEx(g_AIGUI.defaultFont, label, { textX, textY }, 20.0f, 1.0f, BLACK);
    
    return clicked;
}

AIGUI_DEF bool AIGUI_ButtonRounded(const char* label, float x, float y, float width, float height, float radius, int fontSize, Color textColor) {
    Rectangle rect = { x, y, width, height };
    
    // Check if mouse is outside game area
    bool mouseOutsideGameArea = (g_AIGUI.mousePos.x < 0 || g_AIGUI.mousePos.y < 0);
    bool hovered = !mouseOutsideGameArea && CheckCollisionPointRec(g_AIGUI.mousePos, rect);
    
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
        if (g_AIGUI.defaultFont.baseSize > 0 && g_AIGUI.defaultFont.glyphCount > 0 && 
#if defined(__APPLE__) && TARGET_OS_IPHONE
            g_AIGUI.defaultFont.texture.texture != nullptr
#else
            g_AIGUI.defaultFont.texture.id != 0
#endif
        ) {
            Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, label, (float)fontSize, 1.0f);
            float textX = scaledX + (scaledW - size.x) / 2.0f;
            float textY = scaledY + (scaledH - size.y) / 2.0f;
            DrawTextEx(g_AIGUI.defaultFont, label, { textX, textY }, (float)fontSize, 1.0f, textColor);
        }
    }

    return clicked;
}
AIGUI_DEF void AIGUI_LabelRounded(const char* text, float x, float y, float width, float height, float radius, int fontSize, Color textColor, Color bgColor) {
    Rectangle rect = { x, y, width, height };

    // Draw background
    if (bgColor.a > 0) {
        DrawRectangleRounded(rect, radius, 12, bgColor);
    }

    // Draw text
    if (text && text[0] != '\0') {
        if (g_AIGUI.defaultFont.baseSize > 0 && g_AIGUI.defaultFont.glyphCount > 0 &&
#if defined(__APPLE__) && TARGET_OS_IPHONE
            g_AIGUI.defaultFont.texture.texture != nullptr
#else
            g_AIGUI.defaultFont.texture.id != 0
#endif
        ) {
            Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, text, (float)fontSize, 1.0f);
            float textX = x + (width - size.x) / 2.0f;
            float textY = y + (height - size.y) / 2.0f;
            DrawTextEx(g_AIGUI.defaultFont, text, { textX, textY }, (float)fontSize, 1.0f, textColor);
        }
    }
}
AIGUI_DEF bool AIGUI_ImageButton(Texture2D textureDefault, Texture2D textureHover, float x, float y, float width, float height, const char* text, int fontSize, Color textColor, Vector2* customMousePos) {
    Rectangle rect = { x, y, width, height };
    Vector2 mousePos = customMousePos ? *customMousePos : g_AIGUI.mousePos;

    bool hovered = CheckCollisionPointRec(mousePos, rect);
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();

    Texture2D currentTexture = hovered ? textureHover : textureDefault;
    DrawTexturePro(currentTexture, { 0, 0, (float)currentTexture.width, (float)currentTexture.height }, rect, { 0, 0 }, 0.0f, WHITE);

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
    Vector2 mousePos = customMousePos ? *customMousePos : g_AIGUI.mousePos;

    bool hovered = CheckCollisionPointRec(mousePos, rect);
    auto& platform = PlatformLayer::GetInstance();
    bool down = hovered && platform.IsPrimaryInputDown();
    bool clicked = hovered && platform.IsPrimaryInputReleased();

    Texture2D currentTexture = textureNormal;
    if (down) {
        currentTexture = textureClicked;
    } else if (hovered) {
        currentTexture = textureHover;
    }
    
    DrawTexturePro(currentTexture, { 0, 0, (float)currentTexture.width, (float)currentTexture.height }, rect, { 0, 0 }, 0.0f, WHITE);
    
    if (text && strlen(text) > 0) {
        Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, text, (float)fontSize, 1.0f);
        float textX = x + (width - size.x) / 2.0f;
        float textY = y + (height - size.y) / 2.0f;
        DrawTextEx(g_AIGUI.defaultFont, text, { textX, textY }, (float)fontSize, 1.0f, textColor);
    }
    
    return clicked;
}
AIGUI_DEF bool AIGUI_TouchButton(const char* label, float x, float y, float width, float height, float minTouchSize) {
    float scaledWidth = width * g_AIGUI.touchTargetScale;
    float scaledHeight = height * g_AIGUI.touchTargetScale;
    
    // Ensure the touch area is at least the minimum size
    if (scaledWidth < minTouchSize) scaledWidth = minTouchSize;
    if (scaledHeight < minTouchSize) scaledHeight = minTouchSize;
    
    Rectangle touchRect = {
        x + (width - scaledWidth) / 2.0f,
        y + (height - scaledHeight) / 2.0f,
        scaledWidth,
        scaledHeight
    };

    Rectangle visualRect = { x, y, width, height };
    
    bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, touchRect);
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();
    
    Color bgColor = hovered ? Color{ 255, 160, 0, 255 } : Color{ 255, 128, 0, 255 };
    DrawRectangleRec(visualRect, bgColor);
    
    Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, label, 20.0f, 1.0f);
    float textX = x + (width - size.x) / 2.0f;
    float textY = y + (height - size.y) / 2.0f;
    DrawTextEx(g_AIGUI.defaultFont, label, { textX, textY }, 20.0f, 1.0f, BLACK);
    
    return clicked;
}
AIGUI_DEF bool AIGUI_TouchImageButton(Texture2D textureDefault, Texture2D textureHover, float x, float y, float width, float height, float minTouchSize) {
    float scaledWidth = width * g_AIGUI.touchTargetScale;
    float scaledHeight = height * g_AIGUI.touchTargetScale;
    
    if (scaledWidth < minTouchSize) scaledWidth = minTouchSize;
    if (scaledHeight < minTouchSize) scaledHeight = minTouchSize;
    
    Rectangle touchRect = {
        x + (width - scaledWidth) / 2.0f,
        y + (height - scaledHeight) / 2.0f,
        scaledWidth,
        scaledHeight
    };
    
    Rectangle visualRect = { x, y, width, height };
    
    bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, touchRect);
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();

    Texture2D currentTexture = hovered ? textureHover : textureDefault;
    DrawTexturePro(currentTexture, { 0, 0, (float)currentTexture.width, (float)currentTexture.height }, visualRect, { 0, 0 }, 0.0f, WHITE);

    return clicked;
}
AIGUI_DEF void AIGUI_SetUIScale(float scale) {
    g_AIGUI.uiScale = scale;
}
AIGUI_DEF float AIGUI_GetUIScale() {
    return g_AIGUI.uiScale;
}
AIGUI_DEF Vector2 AIGUI_GetScaledSize(float width, float height) {
    return { width * g_AIGUI.uiScale, height * g_AIGUI.uiScale };
}
AIGUI_DEF Rectangle AIGUI_GetSafeAreaInsets() {
    return g_AIGUI.safeArea;
}
AIGUI_DEF bool AIGUI_IsPointInSafeArea(Vector2 point) {
    return CheckCollisionPointRec(point, g_AIGUI.safeArea);
}
AIGUI_DEF float AIGUI_GetMinTouchSize() {
#ifdef PLATFORM_IOS
    return 44.0f; // Apple Human Interface Guideline
#elif defined(PLATFORM_ANDROID)
    return 48.0f; // Android Material Design Guideline
#else
    return 0.0f; // Not applicable on desktop
#endif
}
AIGUI_DEF int AIGUI_GetScaledFontSize(int baseFontSize) {
    return (int)(baseFontSize * g_AIGUI.uiScale);
}
AIGUI_DEF void AIGUI_DrawResponsiveText(const char* text, Vector2 position, int baseFontSize, Color color) {
    int scaledFontSize = AIGUI_GetScaledFontSize(baseFontSize);
    DrawTextEx(g_AIGUI.defaultFont, text, position, (float)scaledFontSize, 1.0f, color);
}
AIGUI_DEF void AIGUI_EndFrame() {}
