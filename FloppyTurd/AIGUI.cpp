#include "AIGUI.h"
#include "TouchControls.h"
#include "ResourceManager.h"
#include "UICoordinateSystem.h"
#include "PlatformLayer.h"

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
    
    // Initialize touch state tracking
    g_AIGUI.touchPressedOverButton = false;
    g_AIGUI.touchStartPos = {0, 0};
    
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
    static int frameCount = 0;
    frameCount++;
    
    TraceLog(LOG_INFO, "[AIGUI] BeginFrame ENTRY - Frame %d, touchPosition=(%.1f,%.1f), touchDown=%d, touchPressed=%d, touchReleased=%d, isMobile=%d",
             frameCount, g_AIGUI.touchPosition.x, g_AIGUI.touchPosition.y,
             g_AIGUI.touchDown, g_AIGUI.touchPressed, g_AIGUI.touchReleased, g_AIGUI.isMobile);
    
    // Reset transient states at the start of each frame
    g_AIGUI.touchPressed = false;
    g_AIGUI.touchReleased = false;
    
    TraceLog(LOG_INFO, "[AIGUI] BeginFrame EXIT - Frame %d", frameCount);
}

AIGUI_DEF void AIGUI_EndFrame() {
    TraceLog(LOG_INFO, "[AIGUI] EndFrame ENTRY");
    
    // End of frame logic if needed
    
    TraceLog(LOG_INFO, "[AIGUI] EndFrame EXIT");
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

AIGUI_DEF void AIGUI_UpdateInput() {
    TraceLog(LOG_INFO, "[AIGUI] UpdateInput ENTRY - isMobile=%d, s_TouchControls=%p", g_AIGUI.isMobile, s_TouchControls);
    
    // Platform-specific input handling
    if (g_AIGUI.isMobile) {
        TraceLog(LOG_INFO, "[AIGUI] UpdateInput: Mobile platform detected");
        
        // On mobile, get input directly from TouchControls static API
        bool touchActive = TouchControls::IsPrimaryInputDown();
        Vector2 touchPos = TouchControls::GetPrimaryInputPosition();
        bool touchPressed = TouchControls::IsPrimaryInputPressed();
        bool touchReleased = TouchControls::IsPrimaryInputReleased();
        
        TraceLog(LOG_INFO, "[AIGUI] UpdateInput: Raw TouchControls data - active=%d, pos=(%.1f,%.1f), pressed=%d, released=%d", 
                 touchActive, touchPos.x, touchPos.y, touchPressed, touchReleased);
        
        // Convert touch coordinates to UI coordinates
        Vector2 uiPos = UICoordinateSystem::PointsToPixels(touchPos);
        
        TraceLog(LOG_INFO, "[AIGUI] UpdateInput: Coordinate conversion - points=(%.1f,%.1f) -> pixels=(%.1f,%.1f)", 
                 touchPos.x, touchPos.y, uiPos.x, uiPos.y);
        
        // Update AIGUI context
        g_AIGUI.touchPosition = uiPos;
        g_AIGUI.touchDown = touchActive;
        g_AIGUI.touchPressed = touchPressed;
        g_AIGUI.touchReleased = touchReleased;
        g_AIGUI.mousePos = uiPos;  // AIGUI still uses mousePos for compatibility
        g_AIGUI.mouseLeftDown = touchActive;
        
        TraceLog(LOG_INFO, "[AIGUI] UpdateInput: AIGUI context updated - touchPosition=(%.1f,%.1f), touchDown=%d, touchPressed=%d, touchReleased=%d, mousePos=(%.1f,%.1f), mouseLeftDown=%d", 
                 g_AIGUI.touchPosition.x, g_AIGUI.touchPosition.y,
                 g_AIGUI.touchDown, g_AIGUI.touchPressed, g_AIGUI.touchReleased,
                 g_AIGUI.mousePos.x, g_AIGUI.mousePos.y, g_AIGUI.mouseLeftDown);
    } else {
        TraceLog(LOG_INFO, "[AIGUI] UpdateInput: Desktop platform detected");
        
        // On desktop, use mouse input
        Vector2 mousePos = GetMousePosition();
        bool mouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        bool mouseReleased = IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
        
        // For desktop, we need to track the previous state to detect pressed
        static bool previousMouseDown = false;
        bool mousePressed = mouseDown && !previousMouseDown;
        previousMouseDown = mouseDown;
        
        TraceLog(LOG_INFO, "[AIGUI] UpdateInput: Desktop mouse data - pos=(%.1f,%.1f), down=%d, pressed=%d, released=%d", 
                 mousePos.x, mousePos.y, mouseDown, mousePressed, mouseReleased);
        
        g_AIGUI.mousePos = mousePos;
        g_AIGUI.mouseLeftDown = mouseDown;
        g_AIGUI.touchPosition = mousePos;
        g_AIGUI.touchDown = mouseDown;
        g_AIGUI.touchPressed = mousePressed;
        g_AIGUI.touchReleased = mouseReleased;
        
        TraceLog(LOG_INFO, "[AIGUI] UpdateInput: Desktop AIGUI context updated");
    }
    
    TraceLog(LOG_INFO, "[AIGUI] UpdateInput EXIT");
}

AIGUI_DEF bool AIGUI_ButtonRounded(const char* label, float x, float y, float width, float height, float radius, int fontSize, Color textColor) {
    TraceLog(LOG_INFO, "[AIGUI] ButtonRounded ENTRY - '%s' at (%.1f,%.1f) size(%.1f,%.1f)", label, x, y, width, height);
    
    if (fontSize > 36) fontSize = 36;
    Rectangle rect = { x, y, width, height };
    bool clicked = false;
    bool gestureTriggered = false;
    Vector2 inputPos = g_AIGUI.touchPosition; // Use dedicated touch position
    
    TraceLog(LOG_INFO, "[AIGUI] ButtonRounded: Current input state - inputPos=(%.1f,%.1f), touchDown=%d, touchPressed=%d, touchReleased=%d, isMobile=%d", 
             inputPos.x, inputPos.y, g_AIGUI.touchDown, g_AIGUI.touchPressed, g_AIGUI.touchReleased, g_AIGUI.isMobile);
    
    // Use full screen dimensions instead of hardcoded 320x180 or safe area
    Rectangle pixelScreenRect = UICoordinateSystem::GetPixelScreenRect();
    bool inputInBounds = (inputPos.x >= 0 && inputPos.y >= 0 && inputPos.x <= pixelScreenRect.width && inputPos.y <= pixelScreenRect.height);
    
    TraceLog(LOG_INFO, "[AIGUI] ButtonRounded: Screen bounds check - inputInBounds=%d, screenSize=(%.1f,%.1f)", 
             inputInBounds, pixelScreenRect.width, pixelScreenRect.height);
    
    if (inputInBounds) {
        bool touchInButton = CheckCollisionPointRec(inputPos, rect);
        
        TraceLog(LOG_INFO, "[AIGUI] ButtonRounded: Touch collision check - touchInButton=%d, buttonRect=(%.1f,%.1f,%.1f,%.1f)", 
                 touchInButton, rect.x, rect.y, rect.width, rect.height);
        
        if (g_AIGUI.isMobile) {
            TraceLog(LOG_INFO, "[AIGUI] ButtonRounded: Mobile touch logic");
            
            // Touch-based logic: detect clicks when touch begins over button
            if (touchInButton && g_AIGUI.touchPressed) {
                clicked = true;
                TraceLog(LOG_INFO, "[AIGUI] Button '%s' clicked at (%.1f,%.1f)", label, inputPos.x, inputPos.y);
            }
            
            TraceLog(LOG_INFO, "[AIGUI] ButtonRounded: Mobile state - touchPressedOverButton=%d, clicked=%d", 
                     g_AIGUI.touchPressedOverButton, clicked);
        } else {
            TraceLog(LOG_INFO, "[AIGUI] ButtonRounded: Desktop mouse logic");
            
            // Desktop mouse logic: keep hover and click detection
            bool hovered = CheckCollisionPointRec(inputPos, rect);
            auto& platform = PlatformLayer::GetInstance();
            clicked = hovered && platform.IsPrimaryInputReleased();
            if (clicked) {
                TraceLog(LOG_INFO, "[AIGUI] Button '%s' clicked at (%.1f,%.1f)", label, inputPos.x, inputPos.y);
            }
        }
        
        if (g_AIGUI.isMobile) {
            if (AIGUI_IsGestureDetected(GESTURE_TAP) && CheckCollisionPointRec(inputPos, rect)) {
                gestureTriggered = true;
                TraceLog(LOG_INFO, "[AIGUI] Button '%s' gesture tap at (%.1f,%.1f)", label, inputPos.x, inputPos.y);
            }
        }
    } else {
        TraceLog(LOG_INFO, "[AIGUI] ButtonRounded: Input out of bounds, skipping collision check");
    }
    
    // Visual state determination
    Color bgColor;
    Color outlineColor = Color{255, 255, 255, 255};
    
    if (g_AIGUI.isMobile) {
        TraceLog(LOG_INFO, "[AIGUI] ButtonRounded: Mobile visual state determination");
        
        // Touch-based visual states
        if (clicked) {
            bgColor = Color{255, 100, 100, 255};
            outlineColor = Color{200, 50, 50, 255};
            TraceLog(LOG_INFO, "[AIGUI] Button '%s' visual state: CLICKED", label);
        } else if (g_AIGUI.touchPressedOverButton && g_AIGUI.touchDown) {
            // Button is being pressed
            bgColor = Color{255, 150, 150, 255};
            outlineColor = Color{200, 100, 100, 255};
            TraceLog(LOG_INFO, "[AIGUI] Button '%s' visual state: PRESSED", label);
        } else {
            // Default state
            bgColor = Color{200, 200, 200, 255};
            outlineColor = Color{100, 100, 100, 255};
            TraceLog(LOG_INFO, "[AIGUI] Button '%s' visual state: DEFAULT", label);
        }
    } else {
        TraceLog(LOG_INFO, "[AIGUI] ButtonRounded: Desktop visual state determination");
        
        // Desktop mouse-based visual states (keep hover logic)
        bool hovered = CheckCollisionPointRec(inputPos, rect);
        if (clicked) {
            bgColor = Color{255, 100, 100, 255};
            outlineColor = Color{200, 50, 50, 255};
        } else if (hovered) {
            bgColor = Color{255, 150, 150, 255};
            outlineColor = Color{200, 100, 100, 255};
        } else {
            bgColor = Color{200, 200, 200, 255};
            outlineColor = Color{100, 100, 100, 255};
        }
    }
    
    DrawRectangleRounded(rect, radius, 8, bgColor);
    float outlineThickness = g_AIGUI.isMobile ? 3.0f : 2.0f;
    DrawRectangleRoundedLinesEx(rect, radius, 8, outlineThickness, outlineColor);
    Vector2 textSize = MeasureTextEx(g_AIGUI.defaultFont, label, fontSize, 1.0f);
    float textX = x + (width - textSize.x) / 2;
    float textY = y + (height - textSize.y) / 2;
    if (g_AIGUI.defaultFont.baseSize > 0 && g_AIGUI.defaultFont.glyphCount > 0) {
        float ascent = g_AIGUI.defaultFont.baseSize * 0.8f;
        float descent = g_AIGUI.defaultFont.baseSize * 0.2f;
        textY = y + (height + ascent - descent - textSize.y) / 2.0f;
    }
    DrawTextEx(g_AIGUI.defaultFont, label, {textX, textY}, fontSize, 1.0f, BLACK);
    bool result = clicked || gestureTriggered;
    
    TraceLog(LOG_INFO, "[AIGUI] ButtonRounded EXIT - '%s' result=%d (clicked=%d, gesture=%d)", label, result, clicked, gestureTriggered);
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
        if (g_AIGUI.defaultFont.baseSize > 0 && g_AIGUI.defaultFont.glyphCount > 0) {
            float ascent = g_AIGUI.defaultFont.baseSize * 0.8f;
            float descent = g_AIGUI.defaultFont.baseSize * 0.2f;
            textY = y + (height + ascent - descent - textSize.y) / 2.0f;
        }
        DrawTextEx(g_AIGUI.defaultFont, text, {textX, textY}, fontSize, 1.0f, BLACK);
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
    
    bool clicked = false;
    
    if (g_AIGUI.isMobile) {
        // Touch-based logic: no hovering, just press-and-release detection
        bool touchInButton = CheckCollisionPointRec(g_AIGUI.touchPosition, rect);
        
        if (touchInButton && g_AIGUI.touchPressed) {
            g_AIGUI.touchPressedOverButton = true;
            g_AIGUI.touchStartPos = g_AIGUI.touchPosition;
        } else if (g_AIGUI.touchPressedOverButton && g_AIGUI.touchReleased) {
            // Check if we're still in the button when released
            if (CheckCollisionPointRec(g_AIGUI.touchPosition, rect)) {
                clicked = true;
            }
            g_AIGUI.touchPressedOverButton = false;
        } else if (!g_AIGUI.touchDown) {
            g_AIGUI.touchPressedOverButton = false;
        }
    } else {
        // Desktop mouse logic: keep hover and click detection
        bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, rect);
        auto& platform = PlatformLayer::GetInstance();
        clicked = hovered && platform.IsPrimaryInputReleased();
    }
    
    // Visual states
    Color bgColor;
    if (g_AIGUI.isMobile) {
        // Touch-based visual states
        if (clicked) {
            bgColor = Color(255, 160, 0, 255);
        } else if (g_AIGUI.touchPressedOverButton && g_AIGUI.touchDown) {
            // Button is being pressed
            bgColor = Color(255, 180, 0, 255);
        } else {
            // Default state
            bgColor = Color(255, 128, 0, 255);
        }
    } else {
        // Desktop mouse-based visual states
        bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, rect);
        bgColor = hovered ? Color(255, 160, 0, 255) : Color(255, 128, 0, 255);
    }
    
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
    if (g_AIGUI.defaultFont.baseSize > 0 && g_AIGUI.defaultFont.glyphCount > 0) {
        float ascent = g_AIGUI.defaultFont.baseSize * 0.8f;
        float descent = g_AIGUI.defaultFont.baseSize * 0.2f;
        textY = y + (height + ascent - descent - textSize.y) / 2.0f;
    }
    DrawTextEx(g_AIGUI.defaultFont, text, {textX, textY}, fontSize, 1.0f, BLACK);
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

    DrawText(text, (int)x, (int)y, (int)fontSize, color);
}