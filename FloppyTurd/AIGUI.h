#ifndef AIGUI_H
#define AIGUI_H

#include "RaylibCompat.h"
#include "PlatformLayer.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS
// For iOS builds, we never include or call raylib directly
// We'll use the platform compatibility layer instead
#else
#include "raylib.h"
#endif

#ifdef AIGUI_STATIC
#define AIGUI_DEF static
#elif defined(AIGUI_INLINE)
#define AIGUI_DEF inline
#else
#define AIGUI_DEF extern
#endif

// Standard font sizes for consistency
const int AIGUI_FONT_SIZE_SMALL = 10;
const int AIGUI_FONT_SIZE_MEDIUM = 14;
const int AIGUI_FONT_SIZE_LARGE = 18;

// Function declarations
AIGUI_DEF void AIGUI_Init();
AIGUI_DEF void AIGUI_Shutdown();
AIGUI_DEF void AIGUI_BeginFrame();
AIGUI_DEF void AIGUI_EndFrame();
AIGUI_DEF void AIGUI_SetFont(Font font);
AIGUI_DEF void AIGUI_SliderFloat(const char* label, float x, float y, float width, float min, float max, float* value);
AIGUI_DEF bool AIGUI_Button(const char* label, float x, float y, float width, float height);
AIGUI_DEF bool AIGUI_ButtonRounded(const char* label, float x, float y, float width, float height, float radius = 0.2f, int fontSize = 18, Color textColor = WHITE);
AIGUI_DEF void AIGUI_LabelRounded(const char* text, float x, float y, float width, float height, float radius = 0.2f, int fontSize = 18, Color textColor = WHITE, Color bgColor = { 0, 0, 0, 0 });
AIGUI_DEF bool AIGUI_ImageButton(Texture2D textureDefault, Texture2D textureHover, float x, float y, float width, float height, const char* text, int fontSize, Color textColor, Vector2* customMousePos);
AIGUI_DEF bool AIGUI_StateButton(Texture2D textureNormal, Texture2D textureHover, Texture2D textureClicked, float x, float y, float width, float height, const char* text = nullptr, int fontSize = 20, Color textColor = WHITE, Vector2* customMousePos = nullptr);
AIGUI_DEF Vector2 _GetScaledInputPosition();
AIGUI_DEF Vector2 _GetScaledMousePosition();
AIGUI_DEF bool AIGUI_TouchButton(const char* label, float x, float y, float width, float height, float minTouchSize = 44.0f);
AIGUI_DEF bool AIGUI_TouchImageButton(Texture2D textureDefault, Texture2D textureHover, float x, float y, float width, float height, float minTouchSize = 44.0f);
AIGUI_DEF void AIGUI_SetUIScale(float scale);
AIGUI_DEF float AIGUI_GetUIScale();
AIGUI_DEF Vector2 AIGUI_GetScaledSize(float width, float height);
AIGUI_DEF Rectangle AIGUI_GetSafeAreaInsets();
AIGUI_DEF bool AIGUI_IsPointInSafeArea(Vector2 point);
AIGUI_DEF float AIGUI_GetMinTouchSize();
AIGUI_DEF int AIGUI_GetScaledFontSize(int baseFontSize);
AIGUI_DEF void AIGUI_DrawResponsiveText(const char* text, Vector2 position, int baseFontSize, Color color);
// Gesture support for UI interactions
AIGUI_DEF bool AIGUI_IsGestureDetected(int gestureType);
AIGUI_DEF void AIGUI_SetTouchControls(class TouchControls* controls);

struct AIGUI_Context {
    Vector2 mousePos;
    bool mouseLeftDown;
    Font defaultFont; 
    float uiScale;
    Rectangle safeArea;
    bool isMobile;
    float touchTargetScale;
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

#endif // AIGUI_H