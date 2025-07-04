 AIGUI_Context g_AIGUI;
static class TouchControls* s_TouchControls = nullptr;

AIGUI_DEF void AIGUI_Init() {
    g_AIGUI.mousePos = {0, 0};
    g_AIGUI.mouseLeftDown = false;
    g_AIGUI.uiScale = 1.0f;
    g_AIGUI.safeArea = {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
    g_AIGUI.isMobile = PlatformLayer::GetInstance().IsTouchSupported();
    g_AIGUI.touchTargetScale = g_AIGUI.isMobile ? 1.5f : 1.0f;
}

AIGUI_DEF void AIGUI_Shutdown() {
    // Cleanup if necessary
}

AIGUI_DEF void AIGUI_BeginFrame() {
    g_AIGUI.mousePos = _GetScaledMousePosition();
    g_AIGUI.mouseLeftDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    if (s_TouchControls) {
        s_TouchControls->Update();
    }
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
    Rectangle rect = {x, y, width, height};
    bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, rect);
    bool clicked = hovered && g_AIGUI.mouseLeftDown;
    Color bgColor = clicked ? Fade(BLUE, 0.8f) : hovered ? Fade(BLUE, 0.6f) : Fade(BLUE, 0.4f);

    // Check for gesture interaction on mobile
    bool gestureTriggered = false;
    if (g_AIGUI.isMobile) {
        if (AIGUI_IsGestureDetected(GESTURE_TAP) && CheckCollisionPointRec(_GetScaledInputPosition(), rect)) {
            gestureTriggered = true;
        }
    }

    DrawRectangleRounded(rect, radius, 8, bgColor);
    Vector2 textSize = MeasureTextEx(g_AIGUI.defaultFont, label, fontSize, 1.0f);
    float textX = x + (width - textSize.x) / 2;
    float textY = y + (height - textSize.y) / 2;
    DrawTextEx(g_AIGUI.defaultFont, label, {textX, textY}, fontSize, 1.0f, textColor);

    return (hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) || gestureTriggered;
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

    return (hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) || gestureTriggered;
}

AIGUI_DEF bool AIGUI_TouchButton(const char* label, float x, float y, float width, float height, float minTouchSize) {
    float touchWidth = std::max(width, minTouchSize * g_AIGUI.touchTargetScale);
    float touchHeight = std::max(height, minTouchSize * g_AIGUI.touchTargetScale);
    float touchX = x - (touchWidth - width) / 2.0f;
    float touchY = y - (touchHeight - height) / 2.0f;
    Rectangle touchRect = {touchX, touchY, touchWidth, touchHeight};
    Rectangle displayRect = {x, y, width, height};

    bool hovered = CheckCollisionPointRec(g_AIGUI.mousePos, touchRect);
    bool clicked = hovered && g_AIGUI.mouseLeftDown;
    Color bgColor = clicked ? Fade(BLUE, 0.8f) : hovered ? Fade(BLUE, 0.6f) : Fade(BLUE, 0.4f);

    // Check for gesture interaction on mobile
    bool gestureTriggered = false;
    if (g_AIGUI.isMobile) {
        if (AIGUI_IsGestureDetected(GESTURE_TAP) && CheckCollisionPointRec(_GetScaledInputPosition(), touchRect)) {
            gestureTriggered = true;
        }
    }

    DrawRectangleRounded(displayRect, 0.2f, 8, bgColor);
    Vector2 textSize = MeasureTextEx(g_AIGUI.defaultFont, label, AIGUI_FONT_SIZE_MEDIUM, 1.0f);
    float textX = x + (width - textSize.x) / 2;
    float textY = y + (height - textSize.y) / 2;
    DrawTextEx(g_AIGUI.defaultFont, label, {textX, textY}, AIGUI_FONT_SIZE_MEDIUM, 1.0f, WHITE);

    return (hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) || gestureTriggered;
}