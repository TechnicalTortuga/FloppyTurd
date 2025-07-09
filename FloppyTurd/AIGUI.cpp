#include "AIGUI.h"
#include "TouchControls.h"
#include "ResourceManager.h"

#if defined(__APPLE__) && TARGET_OS_IOS
// Forward declarations to avoid including Objective-C headers in C++
class PlatformLayer;
class PlatformLayerDelegate;
class MetalTextRenderer;
class MetalRenderer;

// External C function to load system font (implemented in Objective-C++)
extern "C" Font LoadSystemFontForUI(const char* fontName, float fontSize);
#endif

AIGUI_Context g_AIGUI;
static class TouchControls* s_TouchControls = nullptr;

AIGUI_DEF void AIGUI_Init() {
    memset(&g_AIGUI, 0, sizeof(g_AIGUI));
    
    // Clear only font cache to force fresh font loading and avoid corrupted textures
    TraceLog(LOG_INFO, "AIGUI: Clearing font cache to force fresh font loading");
    ResourceManager::GetInstance().ClearFontCache();
    
#if defined(__APPLE__) && TARGET_OS_IOS
    // Remove system font loading, always use Whacky Joe
    // g_AIGUI.defaultFont = LoadSystemFontForUI("ChalkDuster", 32.0f);
    // TraceLog(LOG_INFO, "AIGUI: Using ChalkDuster system font for UI");
#endif
    
    // Try to load Whacky Joe font first, fall back to default if it fails
    TraceLog(LOG_INFO, "AIGUI: Attempting to load whacky_joe_font from ResourceManager");
    Font whackyJoeFont = ResourceManager::GetInstance().GetFont("whacky_joe_font");
    TraceLog(LOG_INFO, "AIGUI: Font loaded - baseSize=%d, glyphCount=%d, ctFont=%p", 
             whackyJoeFont.baseSize, whackyJoeFont.glyphCount, whackyJoeFont.ctFont);
    
    if (whackyJoeFont.baseSize > 0 && whackyJoeFont.glyphCount > 0 && 
#if defined(__APPLE__) && TARGET_OS_IPHONE
        whackyJoeFont.ctFont != nullptr
#else
        whackyJoeFont.texture.id != 0
#endif
    ) {
        g_AIGUI.defaultFont = whackyJoeFont;
        TraceLog(LOG_INFO, "AIGUI: Using Whacky Joe font for UI");
    } else {
        g_AIGUI.defaultFont = GetFontDefault();
        TraceLog(LOG_WARNING, "AIGUI: Whacky Joe font failed, using default font");
        if (g_AIGUI.defaultFont.glyphCount == 0) {
            // Fallback to default font if GetFontDefault fails
            TraceLog(LOG_ERROR, "AIGUI: GetFontDefault also failed, UI may not display text correctly");
        }
    }
    
    // Detect mobile platform and set appropriate scaling
#ifdef PLATFORM_MOBILE
    g_AIGUI.isMobile = true;
    
    // Get screen density from platform layer
    auto& platform = PlatformLayer::GetInstance();
    float density = platform.GetScreenDensity();
    
    // Set UI scale based on screen density with a cap to prevent excessive scaling
    g_AIGUI.uiScale = fminf(fmaxf(1.0f, density * 0.8f), 2.0f);  // Cap at 2.0
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

AIGUI_DEF void AIGUI_Shutdown() {
    // Cleanup if necessary
}

AIGUI_DEF void AIGUI_BeginFrame() {
    // Use PlatformLayer for unified input handling
    // Mouse position is now set by the Game class with proper letterboxing
    // No need to recalculate here - just get button state
    auto& platform = PlatformLayer::GetInstance();
    g_AIGUI.mousePos = _GetScaledInputPosition();
    g_AIGUI.mouseLeftDown = platform.IsPrimaryInputDown();
    
    TraceLog(LOG_INFO, "[AIGUI] BeginFrame: mousePos=(%.1f,%.1f), mouseLeftDown=%d", g_AIGUI.mousePos.x, g_AIGUI.mousePos.y, g_AIGUI.mouseLeftDown);
}

AIGUI_DEF void AIGUI_EndFrame() {
    // End of frame logic if needed
}

AIGUI_DEF void AIGUI_SetFont(Font font) {
    g_AIGUI.defaultFont = font;
}

AIGUI_DEF bool AIGUI_IsGestureDetected(int gestureType) {
    if (s_TouchControls && s_TouchControls->IsEnabled()) {
        return s_TouchControls->IsGestureDetected(gestureType);
    }
    return false;
}

AIGUI_DEF void AIGUI_SetTouchControls(class TouchControls* controls) {
    s_TouchControls = controls;
}

AIGUI_DEF bool AIGUI_ButtonRounded(const char* label, float x, float y, float width, float height, float radius, int fontSize, Color textColor) {
    TraceLog(LOG_INFO, "[AIGUI] ButtonRounded called with label: %s, rect=(%.1f,%.1f,%.1f,%.1f), fontSize=%d, textColor=(%d,%d,%d,%d)", label, x, y, width, height, fontSize, textColor.r, textColor.g, textColor.b, textColor.a);
    
    Rectangle rect = { x, y, width, height };
    bool mouseOutsideGameArea = (g_AIGUI.mousePos.x < 0 || g_AIGUI.mousePos.y < 0);
    bool hovered = !mouseOutsideGameArea && CheckCollisionPointRec(g_AIGUI.mousePos, rect);
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();
    bool gestureTriggered = false;
    
    if (g_AIGUI.isMobile) {
        if (AIGUI_IsGestureDetected(GESTURE_TAP) && CheckCollisionPointRec(_GetScaledInputPosition(), rect)) {
            gestureTriggered = true;
        }
    }
    
    // Use more visible button colors with better contrast
    Color bgColor;
    Color outlineColor = Color{255, 255, 255, 255}; // White outline for visibility
    
    if (clicked) {
        bgColor = Color{255, 100, 100, 255}; // Bright red when clicked
        outlineColor = Color{200, 50, 50, 255}; // Darker red outline
    } else if (hovered) {
        bgColor = Color{255, 150, 150, 255};  // Light red when hovered
        outlineColor = Color{200, 100, 100, 255}; // Medium red outline
    } else {
        bgColor = Color{200, 200, 200, 255};   // Light gray when normal
        outlineColor = Color{100, 100, 100, 255}; // Dark gray outline
    }
    
    TraceLog(LOG_INFO, "[AIGUI] Button state: hovered=%d, clicked=%d, gestureTriggered=%d, bgColor=(%d,%d,%d,%d), outlineColor=(%d,%d,%d,%d)", 
             hovered, clicked, gestureTriggered, bgColor.r, bgColor.g, bgColor.b, bgColor.a, 
             outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
    
    // Draw button background with visible color
    DrawRectangleRounded(rect, radius, 8, bgColor);
    
    // Draw button outline for better visibility - use thicker line for mobile
    float outlineThickness = g_AIGUI.isMobile ? 3.0f : 2.0f;
    DrawRectangleRoundedLinesEx(rect, radius, 8, outlineThickness, outlineColor);
    
    // Center text in button (vertical centering fix: use font metrics if available)
    Vector2 textSize = MeasureTextEx(g_AIGUI.defaultFont, label, fontSize, 1.0f);
    float textX = x + (width - textSize.x) / 2;
    float textY = y + (height - textSize.y) / 2;
    // Optionally tweak textY for better baseline centering if needed
    // textY += fontSize * 0.1f; // Uncomment and adjust if text still looks low
    TraceLog(LOG_INFO, "[AIGUI] DrawTextEx params: label=%s, x=%.1f, y=%.1f, fontSize=%d, color=(%d,%d,%d,%d)", label, textX, textY, fontSize, textColor.r, textColor.g, textColor.b, textColor.a);
    DrawTextEx(g_AIGUI.defaultFont, label, {textX, textY}, fontSize, 1.0f, textColor);

    bool result = (hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) || gestureTriggered;
    if (result) {
        TraceLog(LOG_INFO, "[AIGUI] Button '%s' was clicked/activated", label);
    }
    
    return result;
}

AIGUI_DEF bool AIGUI_ImageButton(Texture2D textureDefault, Texture2D textureHover, float x, float y, float width, float height, const char* text, int fontSize, Color textColor, Vector2* customMousePos) {
    Rectangle rect = {x, y, width, height};
    Vector2 mousePos = customMousePos ? *customMousePos : g_AIGUI.mousePos;
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    bool clicked = hovered && g_AIGUI.mouseLeftDown;
    Texture2D textureToDraw = (hovered || clicked) ? textureHover : textureDefault;

    // Check for gesture interaction on mobile
    bool gestureTriggered = false;
    if (g_AIGUI.isMobile) {
        if (AIGUI_IsGestureDetected(GESTURE_TAP) && CheckCollisionPointRec(_GetScaledInputPosition(), rect)) {
            gestureTriggered = true;
        }
    }

    DrawTexturePro(
        textureToDraw,
        {0, 0, (float)textureToDraw.width, (float)textureToDraw.height},
        rect,
        {0, 0},
        0.0f,
        WHITE
    );

    if (text && strlen(text) > 0) {
        Vector2 textSize = MeasureTextEx(g_AIGUI.defaultFont, text, fontSize, 1.0f);
        float textX = x + (width - textSize.x) / 2;
        float textY = y + (height - textSize.y) / 2;
        DrawTextEx(g_AIGUI.defaultFont, text, {textX, textY}, fontSize, 1.0f, textColor);
    }
    
    return clicked;
}

AIGUI_DEF bool AIGUI_TouchButton(const char* label, float x, float y, float width, float height, float minTouchSize) {
    // Ensure minimum touch size for mobile accessibility
    float touchWidth = fmaxf(width, minTouchSize * g_AIGUI.touchTargetScale);
    float touchHeight = fmaxf(height, minTouchSize * g_AIGUI.touchTargetScale);
    
    // Center the touch area if it's larger than visual area
    float touchX = x - (touchWidth - width) / 2.0f;
    float touchY = y - (touchHeight - height) / 2.0f;
    
    Rectangle touchRect = { touchX, touchY, touchWidth, touchHeight };
    Rectangle visualRect = { x, y, width, height };
    
    bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, touchRect);
    
    // Use platform-agnostic input
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();
    
    // Scale font size for mobile readability
    int scaledFontSize = AIGUI_GetScaledFontSize(20);
    
    // Simple visual states with better mobile contrast
    Color bgColor = hovered ? Color(255, 180, 0, 255) : Color(255, 140, 0, 255);
    DrawRectangleRec(visualRect, bgColor);
    
    Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, label, (float)scaledFontSize, 1.0f);
    float textX = x + (width - size.x) / 2.0f;
    float textY = y + (height - size.y) / 2.0f;
    DrawTextEx(g_AIGUI.defaultFont, label, { textX, textY }, (float)scaledFontSize, 1.0f, WHITE);
    
    return clicked;
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

// Additional missing function implementations
AIGUI_DEF void AIGUI_SetUIScale(float scale) {
    g_AIGUI.uiScale = scale;
}

AIGUI_DEF float AIGUI_GetUIScale() {
    return g_AIGUI.uiScale;
}

AIGUI_DEF Vector2 AIGUI_GetScaledSize(float width, float height) {
    return {width * g_AIGUI.uiScale, height * g_AIGUI.uiScale};
}

AIGUI_DEF Rectangle AIGUI_GetSafeAreaInsets() {
    return g_AIGUI.safeArea;
}

AIGUI_DEF bool AIGUI_IsPointInSafeArea(Vector2 point) {
    return CheckCollisionPointRec(point, g_AIGUI.safeArea);
}

AIGUI_DEF float AIGUI_GetMinTouchSize() {
#ifdef PLATFORM_MOBILE
    // Apple and Google guidelines recommend 44pt minimum touch target
    return 44.0f * g_AIGUI.touchTargetScale;
#else
    // Desktop can use smaller targets with precise mouse input
    return 32.0f;
#endif
}

AIGUI_DEF int AIGUI_GetScaledFontSize(int baseFontSize) {
    float scaledSize = (float)baseFontSize * g_AIGUI.uiScale;
    
#ifdef PLATFORM_MOBILE
    // Ensure minimum readable size on mobile
    if (scaledSize < 16.0f) scaledSize = 16.0f;
    
    // Cap maximum size to prevent overly large text
    if (scaledSize > 48.0f) scaledSize = 48.0f;
#endif
    
    return (int)scaledSize;
}

AIGUI_DEF void AIGUI_DrawResponsiveText(const char* text, Vector2 position, int baseFontSize, Color color) {
    int scaledFontSize = AIGUI_GetScaledFontSize(baseFontSize);
    DrawText(text, (int)position.x, (int)position.y, scaledFontSize, color);
}

// Stub implementations for missing functions
AIGUI_DEF void AIGUI_SliderFloat(const char* label, float x, float y, float width, float min, float max, float* value) {
    // TODO: Implement slider functionality
}

AIGUI_DEF bool AIGUI_Button(const char* label, float x, float y, float width, float height) {
    Rectangle rect = { x, y, width, height };
    
    bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, rect);
    
    // Use platform-agnostic input
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();
    
    // Simple visual states
    Color bgColor = hovered ? Color(255, 160, 0, 255) : Color(255, 128, 0, 255);
    DrawRectangleRec(rect, bgColor);
    
    Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, label, 20.0f, 1.0f);
    float textX = x + (width - size.x) / 2.0f;
    float textY = y + (height - size.y) / 2.0f;
    DrawTextEx(g_AIGUI.defaultFont, label, { textX, textY }, 20.0f, 1.0f, WHITE);
    
    return clicked;
}

AIGUI_DEF void AIGUI_LabelRounded(const char* text, float x, float y, float width, float height, float radius, int fontSize, Color textColor, Color bgColor) {
    Rectangle rect = {x, y, width, height};
    if (bgColor.a > 0) {
        DrawRectangleRounded(rect, radius, 8, bgColor);
    }
    Vector2 textSize = MeasureTextEx(g_AIGUI.defaultFont, text, fontSize, 1.0f);
    float textX = x + (width - textSize.x) / 2;
    float textY = y + (height - textSize.y) / 2;
    DrawTextEx(g_AIGUI.defaultFont, text, {textX, textY}, fontSize, 1.0f, textColor);
}

AIGUI_DEF bool AIGUI_StateButton(Texture2D textureNormal, Texture2D textureHover, Texture2D textureClicked, float x, float y, float width, float height, const char* text, int fontSize, Color textColor, Vector2* customMousePos) {
    Rectangle rect = { x, y, width, height };
    Vector2 inputPos = customMousePos ? *customMousePos : g_AIGUI.mousePos;
    
    bool hovered = CheckCollisionPointRec(inputPos, rect);
    
    // Use platform-agnostic input
    auto& platform = PlatformLayer::GetInstance();
    bool pressed = hovered && platform.IsPrimaryInputDown();
    bool clicked = hovered && platform.IsPrimaryInputReleased();
    
    // Choose texture based on state
    Texture2D textureToUse = textureNormal;
    if (pressed) {
        textureToUse = textureClicked;
    } else if (hovered) {
        textureToUse = textureHover;
    }
    
    DrawTexturePro(textureToUse, { 0, 0, (float)textureToUse.width, (float)textureToUse.height }, rect, { 0, 0 }, 0.0f, WHITE);
    
    if (text) {
        Vector2 size = MeasureTextEx(g_AIGUI.defaultFont, text, fontSize, 1.0f);
        float textX = x + (width - size.x) / 2.0f;
        float textY = y + (height - size.y) / 2.0f;
        DrawTextEx(g_AIGUI.defaultFont, text, { textX, textY }, fontSize, 1.0f, textColor);
    }
    
    return clicked;
}

AIGUI_DEF bool AIGUI_TouchImageButton(Texture2D textureDefault, Texture2D textureHover, float x, float y, float width, float height, float minTouchSize) {
    // Ensure minimum touch size for mobile accessibility
    float touchWidth = fmaxf(width, minTouchSize * g_AIGUI.touchTargetScale);
    float touchHeight = fmaxf(height, minTouchSize * g_AIGUI.touchTargetScale);
    
    // Center the touch area if it's larger than visual area
    float touchX = x - (touchWidth - width) / 2.0f;
    float touchY = y - (touchHeight - height) / 2.0f;
    
    Rectangle touchRect = { touchX, touchY, touchWidth, touchHeight };
    Rectangle visualRect = { x, y, width, height };
    
    bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, touchRect);
    
    // Use platform-agnostic input
    auto& platform = PlatformLayer::GetInstance();
    bool clicked = hovered && platform.IsPrimaryInputReleased();
    
    DrawTexturePro(hovered ? textureHover : textureDefault, 
                   { 0, 0, (float)textureDefault.width, (float)textureDefault.height }, 
                   visualRect, { 0, 0 }, 0.0f, WHITE);
    
    return clicked;
}

AIGUI_DEF void AIGUI_DrawText(const char* text, float x, float y, float fontSize, Color color) {
    TraceLog(LOG_INFO, "[AIGUI] DrawText called with text: %s, font.baseSize: %d, font.glyphCount: %d", text, g_AIGUI.defaultFont.baseSize, g_AIGUI.defaultFont.glyphCount);
    DrawText(text, (int)x, (int)y, (int)fontSize, color);
}