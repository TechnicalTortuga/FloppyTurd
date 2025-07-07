# Metal Renderer Issue Report and Action Plan

## Overview
The Metal-based rendering pipeline for the FloppyTurd 2D sprite game on iOS exhibits three main issues:
1. **Full-screen image renders as a single color** (likely due to texture coordinate stretching or projection matrix misalignment).
2. **Buttons not fully rendering** (due to degenerate geometry, depth sorting, and overlapping positions).
3. **Text glitching/flashing** (due to font loading failure and transient text texture lifecycle).

This updated report incorporates analysis of the runtime log (`floppyturd_20250707_151758_103.log`) and source files (`AIGUI.h`, `AIGUI.cpp`, `MainMenu.h`, `MainMenu.cpp`) to refine root causes and action items, focusing on font handling, button scaling, texture coordinates, and explicit 2D layering.

## Detailed Analysis

### 1. Single-Color Full-Screen Image
- **Symptoms**: The main menu background (`main_menu_bg_mobile`, 393x852) renders as a single color, despite successful loading.
- **Root Causes**:
  - **Texture Coordinate Stretching**: `MainMenu::DrawMobileUI` draws the background with a destination rectangle of `1179x2556` (full screen), stretching the `393x852` texture. Unnormalized texture coordinates in `MetalRenderer::AddTexturedRectangleVertices` may cause incorrect sampling:
    ```cpp
    DrawTexturePro(_MenuBackground, Rectangle{ 0, 0, 393, 852 }, Rectangle{ 0, 0, 1179, 2556 }, ...);
    ```
  - **Projection Matrix Misalignment**: The projection matrix in `MetalRenderer::BeginFrame` may not account for the safe area (`393x852`) or screen scale (`1179x2556`, 3x Retina).
  - **Test Rectangle Interference**: A test rectangle in `EndFrame` may overwrite the background:
    ```
    [2025-07-07 15:18:38.483] [TRACE] [METAL DEBUG] EndFrame called - drawing test rectangle
    ```

- **Evidence**:
  - Log confirms background texture validity:
    ```
    [2025-07-07 15:18:38.470] [TRACE] [MAINMENU] Background texture validity: id=3, w=393, h=852, mipmaps=10, format=7, metalPtr=0x104f0bdd0
    ```
  - Logo’s negative X-coordinate (`x=-121.6`) suggests potential clipping or sampling issues:
    ```
    [2025-07-07 15:18:38.470] [TRACE] [MAINMENU] Drawing logo at x=-121.6 y=50.0 w=636.2 h=454.4
    ```

### 2. Buttons Not Fully Rendering
- **Symptoms**: Buttons (`PLAY`, `OPTIONS`, `CREDITS`, `QUIT`) are partially or not rendered.
- **Root Causes**:
  - **Degenerate Triangles**: `AddTexturedRectangleVertices` repeats a vertex, creating a degenerate quad:
    ```cpp
    AddVertex(dest.x + dest.width, dest.y, u2, v1, tint); // Repeated
    ```
  - **Depth Testing**: The pipeline uses `MTLCompareFunctionLessEqual` with depth writes enabled, culling buttons with identical depth (`0.0f`).
  - **Button Overlap**: `MainMenu::DrawMobileUI` positions buttons with overlapping Y-coordinates (`OPTIONS` at `y=426.0, h=284.0`, `QUIT` at `y=548.0, h=284.0`):
    ```
    [2025-07-07 15:18:38.474] [TRACE] [MAINMENU] Drawing button: OPTIONS at x=39.3 y=426.0 w=314.4 h=284.0
    [2025-07-07 15:18:38.480] [TRACE] [MAINMENU] Drawing button: QUIT at x=39.3 y=548.0 w=314.4 h=284.0
    ```
  - **Excessive Scaling**: Button heights are scaled by `ui.GetScaleFactor()` (~2.4), inflating `buttonHeight` to `284.0`, causing overlap and potential clipping.

- **Evidence**:
  - Overlap confirmed (`426.0 + 284.0 = 710.0 > 548.0`).
  - `AIGUI_ButtonRounded` uses `DrawRectangleRounded` without textures, relying on solid colors, but degenerate geometry may affect rendering.

### 3. Text Glitching/Flashing
- **Symptoms**: Text labels (`PLAY`, `OPTIONS`, `CREDITS`, `QUIT`) flicker or flash.
- **Root Causes**:
  - **Font Loading Failure**: `AIGUI_Init` fails to load `whacky_joe_font`, falling back to `GetFontDefault()`, which has `glyphCount=0`:
    ```
    [2025-07-07 15:18:38.473] [TRACE] [RaylibCompat_iOS] DrawTextEx called: text='PLAY', ..., glyphCount=0, font.ctFont=0x10581d020
    ```
  - **Texture Lifecycle**: `MetalTextRenderer::RenderTextToTexture` creates transient textures, which may be released by ARC before rendering completes.
  - **Synchronization**: Text textures aren’t managed by `MetalFrameResources` triple buffering, causing GPU-CPU contention.

- **Evidence**:
  - `AIGUI_Init` logs indicate `whacky_joe_font` failure, using default font with `glyphCount=0`.
  - No text texture creation logs, suggesting silent failures in `MetalTextRenderer`.

### 4. Lack of Explicit 2D Layering
- **Symptoms**: Overlapping elements (background, logo, buttons, text) render unpredictably.
- **Root Causes**:
  - **No Layer Separation**: `MainMenu::DrawMobileUI` and `AIGUI_ButtonRounded` don’t specify depth, defaulting to `0.0f`.
  - **Depth Conflicts**: Identical depth values cause sorting issues, especially for overlapping buttons.
  - **Test Rectangle**: The test rectangle in `EndFrame` may interfere with other elements.

- **Evidence**:
  - Log shows no layer-specific depth values:
    ```
    [2025-07-07 15:18:38.470] [TRACE] [MAINMENU] Drawing logo at x=-121.6 y=50.0 w=636.2 h=454.4
    ```

### 5. Additional Observations
- **Asset Loading**: Audio failures (`OSStatus error 2003334207`) suggest asset bundling issues, but textures load correctly.
- **Scaling**: `g_AIGUI.uiScale` (~2.4) inflates button sizes, contributing to overlap.
- **Rendering Loop**: 2141 `EndFrame` calls indicate unnecessary redraws, potentially straining the renderer.

## Action Plan

Below is the updated action plan, incorporating code analysis. New actions (A16-A18) address font loading, button scaling, and coordinate alignment. Existing actions are refined for specificity.

### Action Items

| ID  | Issue | Action | Status | Priority | Owner | Notes |
|-----|-------|--------|--------|----------|-------|-------|
| A1  | Single-color image | Normalize texture coordinates in `AddTexturedRectangleVertices`. | Not Started | High | Developer | Divide `source` by texture dimensions. Log UVs for `main_menu_bg_mobile`. Test with safe area dimensions (`393x852`). |
| A2  | Single-color image | Verify asset paths in Xcode asset catalog. | Completed | High | Developer | Log confirms textures load correctly. Check font assets for `whacky_joe_font`. |
| A3  | Single-color image | Clear render texture in `LoadRenderTexture_iOS`. | Not Started | Medium | Developer | Use `MTLRenderCommandEncoder` to clear with black. Log clear operation. |
| A4  | Buttons not rendering | Fix vertex data in `AddTexturedRectangleVertices` for six unique vertices. | Not Started | High | Developer | Correct repeated vertex. Test with `PLAY` button. Log vertex coordinates. |
| A5  | Buttons not rendering | Disable depth testing for UI elements in `CreatePipelines`. | Not Started | High | Developer | Use `MTLCompareFunctionAlways`, `depthWriteEnabled=NO` for UI pipeline. Test with overlapping buttons. |
| A6  | Buttons not rendering | Verify button rendering in `AIGUI_ButtonRounded`. | Not Started | High | Developer | Log `DrawRectangleRounded` parameters and alpha channel. Ensure valid geometry. |
| A7  | Text glitching | Cache text textures in `MetalTextureCache`. | Not Started | High | Developer | Store textures with key `text + fontSize`. Log cache hits/misses. |
| A8  | Text glitching | Synchronize text textures in `MetalFrameResources`. | Not Started | Medium | Developer | Track text textures per frame, release after command buffer commit. Log lifecycle. |
| A9  | All issues | Implement explicit 2D layering in `MetalRenderer`. | Not Started | High | Developer | Assign depth: background=0.0, logo=0.1, UI=0.2, text=0.3. Sort by layer. Test with buttons. |
| A10 | All issues | Enable Metal API validation and frame capture. | Not Started | High | Developer | Enable in Xcode. Capture frames to inspect draw calls and texture bindings. |
| A11 | All issues | Add detailed logging in `ExecuteOptimizedDrawCommands`. | Not Started | Medium | Developer | Log texture pointers, vertex counts, and render states. |
| A12 | Single-color image | Align projection matrix in `BeginFrame` with safe area (`393x852`). | Not Started | High | Developer | Log `screenWidth`, `screenHeight`, `GetScreenDensity`. Test logo at `x=-121.6`. |
| A13 | Text glitching | Debug font glyph loading in `ResourceManager::GetFont`. | Not Started | High | Developer | Log font properties (`baseSize`, `glyphCount`, `ctFont`). Verify `whacky_joe_font` in Xcode. Test with `PLAY`. |
| A14 | Buttons not rendering | Adjust button positions in `MainMenu::DrawMobileUI` to avoid overlap. | Not Started | High | Developer | Reduce `buttonHeight` to `100 * scale`. Increase `buttonSpacing`. Log rectangles. Test rendering. |
| A15 | Single-color image | Remove test rectangle in `EndFrame`. | Not Started | High | Developer | Disable or assign depth=1.0. Log draw call details. Test background. |
| A16 | Text glitching | Fallback to system font if `GetFontDefault` fails. | Not Started | High | Developer | Use `CTFontCreateWithName` (e.g., Helvetica) in `AIGUI_Init`. Log font properties. |
| A17 | Buttons not rendering | Cap `uiScale` in `AIGUI_Init` to prevent excessive button sizes. | Not Started | Medium | Developer | Limit `uiScale` to 2.0. Log `uiScale` and button dimensions. |
| A18 | Single-color image | Use safe area dimensions for background destination rectangle. | Not Started | High | Developer | Set destination to `safeArea.width x safeArea.height`. Test rendering. |

### Code Modifications

Below are updated and new code changes for key actions.

#### A1: Normalize Texture Coordinates
Update `MetalRenderer::AddTexturedRectangleVertices`:
```cpp
void MetalRenderer::AddTexturedRectangleVertices(Rectangle dest, Rectangle source, Color tint) {
    float texWidth = texture ? texture.width : 1.0f;
    float texHeight = texture ? texture.height : 1.0f;
    float u1 = source.x / texWidth;
    float v1 = source.y / texHeight;
    float u2 = (source.x + source.width) / texWidth;
    float v2 = (source.y + source.height) / texHeight;
    NSLog(@"[DEBUG] UVs: u1=%.2f, v1=%.2f, u2=%.2f, v2=%.2f, tex=%dx%d", u1, v1, u2, v2, (int)texWidth, (int)texHeight);
    AddVertex(dest.x, dest.y + dest.height, u1, v2, tint);
    AddVertex(dest.x, dest.y, u1, v1, tint);
    AddVertex(dest.x + dest.width, dest.y + dest.height, u2, v2, tint);
    AddVertex(dest.x, dest.y, u1, v1, tint);
    AddVertex(dest.x + dest.width, dest.y + dest.height, u2, v2, tint);
    AddVertex(dest.x + dest.width, dest.y, u2, v1, tint);
}
```

#### A4: Fix Vertex Data
Correct vertex data:
```cpp
void MetalRenderer::AddTexturedRectangleVertices(Rectangle dest, Rectangle source, Color tint) {
    float texWidth = texture ? texture.width : 1.0f;
    float texHeight = texture ? texture.height : 1.0f;
    float u1 = source.x / texWidth;
    float v1 = source.y / texHeight;
    float u2 = (source.x + source.width) / texWidth;
    float v2 = (source.y + source.height) / texHeight;
    NSLog(@"[DEBUG] Vertices: x1=%.1f, y1=%.1f, x2=%.1f, y2=%.1f", dest.x, dest.y, dest.x + dest.width, dest.y + dest.height);
    AddVertex(dest.x, dest.y + dest.height, u1, v2, tint); // Bottom-left
    AddVertex(dest.x, dest.y, u1, v1, tint);               // Top-left
    AddVertex(dest.x + dest.width, dest.y + dest.height, u2, v2, tint); // Bottom-right
    AddVertex(dest.x, dest.y, u1, v1, tint);               // Top-left
    AddVertex(dest.x + dest.width, dest.y + dest.height, u2, v2, tint); // Bottom-right
    AddVertex(dest.x + dest.width, dest.y, u2, v1, tint);  // Top-right
}
```

#### A5: Disable Depth Testing for UI
In `MetalRenderer::CreatePipelines`:
```cpp
MTLDepthStencilDescriptor* uiDepthDesc = [[MTLDepthStencilDescriptor alloc] init];
uiDepthDesc.depthCompareFunction = MTLCompareFunctionAlways;
uiDepthDesc.depthWriteEnabled = NO;
m_uiDepthStencilState = [m_device newDepthStencilStateWithDescriptor:uiDepthDesc];
```
In `ExecuteOptimizedDrawCommands`:
```cpp
if (cmd.debugName == "Texture" || cmd.debugName == "Rectangle" || cmd.debugName == "Text") {
    [m_currentEncoder setDepthStencilState:m_uiDepthStencilState];
    NSLog(@"[DEBUG] Using UI depth stencil state for %s", cmd.debugName.c_str());
} else {
    [m_currentEncoder setDepthStencilState:m_depthStencilState];
}
```

#### A7: Cache Text Textures
In `MetalTextRenderer::RenderTextToTexture`:
```cpp
id<MTLTexture> MetalTextRenderer::RenderTextToTexture(const char* text, Font font, float fontSize, Color color) {
    NSLog(@"[DEBUG] Rendering text: %s, fontSize=%.1f, glyphCount=%d, ctFont=%p", text, fontSize, font.glyphCount, font.ctFont);
    if (font.glyphCount == 0) {
        NSLog(@"[ERROR] No glyphs loaded for font: baseSize=%d, ctFont=%p", font.baseSize, font.ctFont);
    }
    id<MTLTexture> texture = CreateTextureFromCGImage(CGBitmapContextCreateImage(context));
    if (texture) {
        std::string cacheKey = std::string(text) + "_" + std::to_string(fontSize);
        MetalTextureCache::GetInstance().CacheTexture(cacheKey, texture);
        NSLog(@"[DEBUG] Cached text texture: %s, ptr=%p", cacheKey.c_str(), (__bridge void*)texture);
    } else {
        NSLog(@"[ERROR] Failed to create text texture for: %s", text);
    }
    return texture;
}
```

#### A9: Implement 2D Layering
In `MetalRenderer`:
```cpp
enum class RenderLayer {
    Background = 0,
    Logo = 1,
    UI = 2,
    Text = 3
};
void MetalRenderer::DrawTexture(id<MTLTexture> texture, Rectangle source, Rectangle dest, Color tint, RenderLayer layer) {
    DrawCommand cmd;
    cmd.depth = static_cast<float>(layer) * 0.1f;
    cmd.debugName = (layer == RenderLayer::Background) ? "Background" : (layer == RenderLayer::Logo) ? "Logo" : (layer == RenderLayer::UI) ? "UI" : "Text";
    NSLog(@"[DEBUG] DrawTexture: layer=%d, depth=%.1f, dest=(%.1f,%.1f,%.1f,%.1f)", (int)layer, cmd.depth, dest.x, dest.y, dest.width, dest.height);
}
void MetalRenderer::SortDrawCommands() {
    std::sort(m_drawCommands.begin(), m_drawCommands.end(), [](const DrawCommand& a, const DrawCommand& b) {
        float layerA = floor(a.depth / 0.1f);
        float layerB = floor(b.depth / 0.1f);
        if (layerA != layerB) return layerA < layerB;
        return a.sortKey < b.sortKey;
    });
}
```

#### A13: Debug Font Glyph Loading
In `ResourceManager::GetFont` (assumed implementation):
```cpp
Font ResourceManager::GetFont(const std::string& name) {
    Font font = LoadFont(name.c_str());
    NSLog(@"[DEBUG] GetFont: %s, baseSize=%d, glyphCount=%d, ctFont=%p", name.c_str(), font.baseSize, font.glyphCount, font.ctFont);
    if (font.glyphCount == 0) {
        NSLog(@"[ERROR] Failed to load glyphs for font: %s", name.c_str());
    }
    return font;
}
```

#### A14: Adjust Button Positions
In `MainMenu::DrawMobileUI`:
```cpp
void MainMenu::DrawMobileUI() {
    TraceLog(LOG_INFO, "[MAINMENU] --- Begin DrawMobileUI Frame ---");
    UIManager& ui = UIManager::GetInstance();
    float screenWidth = GetScreenWidth();
    float screenHeight = GetScreenHeight();
    Rectangle safeArea = ui.GetSafeArea();
    TraceLog(LOG_INFO, "[MAINMENU] Screen: %.1fx%.1f, Safe Area: x=%.1f y=%.1f w=%.1f h=%.1f", screenWidth, screenHeight, safeArea.x, safeArea.y, safeArea.width, safeArea.height);
    DrawTexturePro(_MenuBackground,
        Rectangle{ 0, 0, (float)_MenuBackground.width, (float)_MenuBackground.height },
        Rectangle{ safeArea.x, safeArea.y, safeArea.width, safeArea.height },
        Vector2{ 0,0 }, 0.0f, WHITE, RenderLayer::Background);
    
    Vector2 logoPos = ui.GetPosition(UIAnchor::TOP_CENTER, {0, 50});
    float logoScale = ui.GetScaleFactor() * 1.2f;
    float logoWidth = _FloppyLogo.width * logoScale;
    float logoHeight = _FloppyLogo.height * logoScale;
    TraceLog(LOG_INFO, "[MAINMENU] Drawing logo at x=%.1f y=%.1f w=%.1f h=%.1f", logoPos.x - logoWidth / 2, logoPos.y, logoWidth, logoHeight);
    DrawTexturePro(_FloppyLogo,
        Rectangle{ 0, 0, (float)_FloppyLogo.width, (float)_FloppyLogo.height },
        Rectangle{ logoPos.x - logoWidth / 2, logoPos.y, logoWidth, logoHeight },
        Vector2{ 0,0 }, 0.0f, WHITE, RenderLayer::Logo);
    
    float buttonWidth = safeArea.width * 0.8f;
    float minButtonHeight = 44.0f * ui.GetScaleFactor();
    float buttonHeight = fmaxf(100 * ui.GetScaleFactor(), minButtonHeight); // Reduced from 60
    float buttonSpacing = 40 * ui.GetScaleFactor(); // Increased spacing
    float centerX = safeArea.x + safeArea.width / 2.0f;
    float centerY = safeArea.y + safeArea.height / 2.0f;
    
    float playY = centerY - 1.5f * buttonHeight - buttonSpacing;
    float optionsY = centerY - 0.5f * buttonHeight;
    float creditsY = centerY + 0.5f * buttonHeight + buttonSpacing;
    float quitY = centerY + 1.5f * buttonHeight + 2 * buttonSpacing;
    
    TraceLog(LOG_INFO, "[MAINMENU] Drawing button: PLAY at x=%.1f y=%.1f w=%.1f h=%.1f", centerX - buttonWidth / 2, playY, buttonWidth, buttonHeight);
    if (AIGUI_ButtonRounded("PLAY", centerX - buttonWidth / 2, playY, buttonWidth, buttonHeight, 0.1f, 24, WHITE)) {
        currentMenu = LEVEL_SELECT;
    }
    TraceLog(LOG_INFO, "[MAINMENU] Drawing button: OPTIONS at x=%.1f y=%.1f w=%.1f h=%.1f", centerX - buttonWidth / 2, optionsY, buttonWidth, buttonHeight);
    if (AIGUI_ButtonRounded("OPTIONS", centerX - buttonWidth / 2, optionsY, buttonWidth, buttonHeight, 0.1f, 24, WHITE)) {
        currentMenu = OPTIONS_MENU;
    }
    TraceLog(LOG_INFO, "[MAINMENU] Drawing button: CREDITS at x=%.1f y=%.1f w=%.1f h=%.1f", centerX - buttonWidth / 2, creditsY, buttonWidth, buttonHeight);
    if (AIGUI_ButtonRounded("CREDITS", centerX - buttonWidth / 2, creditsY, buttonWidth, buttonHeight, 0.1f, 24, WHITE)) {
        AudioManager::GetInstance().StopMusic();
        game->SetGameState(CREDITS);
    }
    TraceLog(LOG_INFO, "[MAINMENU] Drawing button: QUIT at x=%.1f y=%.1f w=%.1f h=%.1f", centerX - buttonWidth / 2, quitY, buttonWidth, buttonHeight);
    if (AIGUI_ButtonRounded("QUIT", centerX - buttonWidth / 2, quitY, buttonWidth, buttonHeight, 0.1f, 24, WHITE)) {
        game->SetGameState(SHUTDOWN);
    }
    TraceLog(LOG_INFO, "[MAINMENU] --- End DrawMobileUI Frame ---");
}
```

#### A15: Remove Test Rectangle
In `MetalRenderer::EndFrame`:
```cpp
void MetalRenderer::EndFrame() {
    [m_currentEncoder endEncoding];
    m_currentEncoder = nullptr;
    NSLog(@"[DEBUG] EndFrame: Command buffer committed, no test rectangle");
}
```

#### A16: Fallback to System Font
In `AIGUI_Init`:
```cpp
AIGUI_DEF void AIGUI_Init() {
    Font whackyJoeFont = ResourceManager::GetInstance().GetFont("whacky_joe_font");
    if (whackyJoeFont.baseSize > 0 && whackyJoeFont.glyphCount > 0 && whackyJoeFont.texture.texture != nullptr) {
        g_AIGUI.defaultFont = whackyJoeFont;
        TraceLog(LOG_INFO, "AIGUI: Using Whacky Joe font for UI");
    } else {
        g_AIGUI.defaultFont = GetFontDefault();
        TraceLog(LOG_WARNING, "AIGUI: Whacky Joe font failed, trying system font");
        if (g_AIGUI.defaultFont.glyphCount == 0) {
            CFStringRef fontName = CFSTR("Helvetica");
            CTFontRef ctFont = CTFontCreateWithName(fontName, 16.0, nullptr);
            g_AIGUI.defaultFont.ctFont = ctFont;
            g_AIGUI.defaultFont.baseSize = 16;
            g_AIGUI.defaultFont.glyphCount = 128; // Approximate for ASCII
            TraceLog(LOG_INFO, "AIGUI: Using system font Helvetica, glyphCount=%d", g_AIGUI.defaultFont.glyphCount);
        }
    }
    // ... rest of initialization ...
}
```

#### A17: Cap `uiScale`
In `AIGUI_Init`:
```cpp
AIGUI_DEF void AIGUI_Init() {
    // ... existing code ...
#ifdef PLATFORM_MOBILE
    float density = platform.GetScreenDensity();
    g_AIGUI.uiScale = fminf(fmaxf(1.0f, density * 0.8f), 2.0f); // Cap at 2.0
    g_AIGUI.touchTargetScale = 1.2f;
    TraceLog(LOG_INFO, "AIGUI: Mobile platform detected, UI scale: %.2f, density: %.2f", g_AIGUI.uiScale, density);
    // ... rest of code ...
#endif
}
```

#### A18: Use Safe Area for Background
Already included in A14’s `DrawMobileUI` update:
```cpp
DrawTexturePro(_MenuBackground,
    Rectangle{ 0, 0, (float)_MenuBackground.width, (float)_MenuBackground.height },
    Rectangle{ safeArea.x, safeArea.y, safeArea.width, safeArea.height },
    Vector2{ 0,0 }, 0.0f, WHITE, RenderLayer::Background);
```

### Testing Steps
1. **A1, A12, A15, A18**: Test background rendering with safe area dimensions and normalized UVs. Disable test rectangle. Log UVs and matrix parameters. Verify `main_menu_bg_mobile` displays correctly.
2. **A4-A6, A14, A17**: Test `PLAY` button with fixed vertices, reduced `buttonHeight` (100 * scale), and capped `uiScale`. Log vertex coordinates and button rectangles. Ensure no overlap.
3. **A7, A8, A13, A16**: Test text rendering for `PLAY` with system font fallback. Log `glyphCount` and texture cache hits. Verify no flashing.
4. **A9**: Test layering with background, logo, UI, and text. Log depth values and render order.
5. **A10-A11**: Use Metal frame capture to inspect draw calls and texture bindings. Log render states.
6. **A2, A13**: Verify `whacky_joe_font` in Xcode asset catalog. Log font loading details.

### Next Steps
- Implement high-priority actions (A1, A4, A5, A6, A7, A9, A10, A12, A13, A14, A15, A16, A18).
- Test incrementally, capturing logs and Metal frame captures.
- Debug `ResourceManager::GetFont` to identify why `whacky_joe_font` fails.
- Verify asset bundling for fonts and textures in Xcode.
- Share updated logs or frame capture results if issues persist.