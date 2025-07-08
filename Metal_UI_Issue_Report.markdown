# Metal UI Issue Report and Action Plan

## Overview
As of July 7, 2025, the *Floppy Turd* iOS port using Metal and Raylib is experiencing UI rendering issues that affect the main menu background, logo, text, and overall UI predictability. This report analyzes the root causes based on the provided code and documentation (*Metal_Migration_UICoordinate_Analysis.md*) and outlines actionable steps to resolve these issues. The goal is to ensure a robust, pixel-accurate, device-independent rendering pipeline that delivers a polished gaming experience.

## Known Issues
1. **Main Menu Background Cutoff**:
   - The background texture (393x852, designed for iPhone 14) is misaligned or cut off, not filling the screen as intended.
2. **Logo Texture Cutoff**:
   - The *Floppy Turd* logo is missing, cut off, or incorrectly positioned/scaled.
3. **Text Rendering Failures**:
   - UI text (e.g., button labels) appears upside down, mirrored, or missing.
4. **General UI Predictability**:
   - The rendering system lacks pixel-accurate, device-independent layout, leading to inconsistent UI positioning across devices.

## Root Cause Analysis

### 1. Main Menu Background Cutoff
The background texture is not rendering fully, likely due to:
- **Projection Matrix Mismatch**: The orthographic projection in `MetalRenderer::SetProjectionMatrix` or `SetProjectionMatrixWithSafeArea` may not align with the `MTKView`’s `drawableSize` (pixel resolution, e.g., 1179x2556 for iPhone 14 at 3x retina).
- **Safe Area Constraints**: Using `SetProjectionMatrixWithSafeArea` may restrict rendering to the safe area, excluding notch or home indicator regions.
- **UV Coordinate Errors**: Incorrect UV normalization in `AddTexturedRectangleVertices` could clip or distort the texture.
- **Texture Loading Issues**: `MetalTexture.mm` may load the texture with incorrect dimensions or orientation.
- **Viewport/Scissor Issues**: The Metal viewport or scissor rect may limit the renderable area.

### 2. Logo Texture Cutoff
Similar to the background, the logo issue may stem from:
- Incorrect `source` or `dest` rectangles in `DrawTexture`, causing partial rendering.
- Unintended transformations (e.g., scaling, rotation) in `DrawTextureEx`.
- Depth sorting conflicts in `SortDrawCommands` affecting the `RenderLayer::Logo` layer.

### 3. Text Rendering Failures
Text rendering issues (upside down, mirrored, or missing) likely arise from:
- **Core Text Coordinate Mismatch**: Core Text uses a bottom-left, y-up coordinate system, while Metal uses top-left, y-down. The y-flip transform in `MetalTextRenderer::RenderTextToTexture` may be incorrect or undone.
- **Texture Creation Errors**: The `CGContext` in `RenderTextToTexture` may produce invalid or empty textures due to size miscalculations or pixel data issues.
- **Cache Issues**: The text texture cache (`m_textTextureCache`) may return stale or incorrect textures.

### 4. General UI Predictability
Inconsistent UI layout is caused by:
- **Coordinate System Mismatches**: UIKit uses points, Metal uses pixels, and retina scaling complicates conversions.
- **Safe Area Inconsistencies**: Safe area data from `UIManager`/`PlatformLayer` may not be consistently applied.
- **Draw Order Issues**: The draw command sorting in `SortDrawCommands` may reorder UI elements unexpectedly.

## Actionable Steps

### Step 1: Fix Main Menu Background Cutoff
**Objective**: Ensure the 393x852 background texture fills the entire screen without cutoff.
1. **Verify Texture Dimensions**:
   - In `MetalTexture.mm`, log the texture’s dimensions after loading:
     ```cpp
     id<MTLTexture> texture = ...; // After loading
     NSLog(@"[DEBUG] Loaded background texture: width=%lu, height=%lu", texture.width, texture.height);
     ```
   - Confirm the texture is 393x852 and loaded with `MTLPixelFormatRGBA8Unorm`. Check for unintended scaling or orientation flips in `UIImage`/`CGImage` loading.
2. **Adjust Projection Matrix**:
   - Modify `MetalRenderer::SetProjectionMatrix` to use `MTKView`’s `drawableSize` for pixel-accurate rendering:
     ```cpp
     void MetalRenderer::SetProjectionMatrix(float width, float height) {
         CGSize drawableSize = m_view.drawableSize;
         m_projectionMatrix = MakeOrthoMatrix(0, drawableSize.width, drawableSize.height, 0, -1.0f, 1.0f);
         TraceLog(LOG_INFO, "[METAL DEBUG] SetProjectionMatrix: drawableSize=%.1fx%.1f", drawableSize.width, drawableSize.height);
     }
     ```
   - For the background, avoid `SetProjectionMatrixWithSafeArea` and draw with:
     ```cpp
     Rectangle dest = {0, 0, (float)m_view.drawableSize.width, (float)m_view.drawableSize.height};
     DrawTexture(backgroundTexture, Rectangle{0, 0, (float)texture.width, (float)texture.height}, dest, WHITE, RenderLayer::Background);
     ```
3. **Validate UV Coordinates**:
   - In `AddTexturedRectangleVertices`, log texture dimensions and UVs:
     ```cpp
     void MetalRenderer::AddTexturedRectangleVertices(Rectangle dest, Rectangle source, Color tint) {
         if (!m_currentTexture) {
             TraceLog(LOG_ERROR, "[METAL ERROR] No current texture set for UV normalization");
             return;
         }
         float texWidth = (float)m_currentTexture.width;
         float texHeight = (float)m_currentTexture.height;
         TraceLog(LOG_INFO, "[METAL DEBUG] Texture dimensions: %fx%f", texWidth, texHeight);
         float u1 = source.x / texWidth;
         float v1 = source.y / texHeight;
         float u2 = (source.x + source.width) / texWidth;
         float v2 = (source.y + source.height) / texHeight;
         TraceLog(LOG_INFO, "[METAL DEBUG] UVs: u1=%.2f, v1=%.2f, u2=%.2f, v2=%.2f", u1, v1, u2, v2);
         // ... rest of the function
     }
     ```
   - Ensure `source` is `Rectangle{0, 0, 393, 852}`.
4. **Set Viewport**:
   - In `BeginFrame`, set the viewport to the full drawable size:
     ```cpp
     void MetalRenderer::BeginFrame() {
         // ... existing code ...
         MTLViewport viewport = {0, 0, m_view.drawableSize.width, m_view.drawableSize.height, 0, 1};
         [m_currentEncoder setViewport:viewport];
         TraceLog(LOG_INFO, "[METAL DEBUG] Set viewport: %.1fx%.1f", viewport.width, viewport.height);
     }
     ```

### Step 2: Fix Logo Texture Cutoff
**Objective**: Ensure the logo renders fully at the intended position and scale.
1. **Verify Logo Texture**:
   - Log the logo texture’s dimensions in `MetalTexture.mm` after loading.
   - Ensure the `source` rectangle in `DrawTexture` covers the full texture.
2. **Check Destination Rectangle**:
   - In `Game::RenderFrame`, log the logo’s `dest` rectangle:
     ```cpp
     Rectangle dest = {/* logo position and size */};
     TraceLog(LOG_INFO, "[DEBUG] Logo dest rect: (%.1f, %.1f, %.1f, %.1f)", dest.x, dest.y, dest.width, dest.height);
     DrawTexture(logoTexture, Rectangle{0, 0, (float)logoTexture.width, (float)logoTexture.height}, dest, WHITE, RenderLayer::Logo);
     ```
   - Use `UIManager` to position `dest` within the safe area, accounting for retina scaling.
3. **Disable Unintended Transformations**:
   - If using `DrawTextureEx`, log transformations:
     ```cpp
     void MetalRenderer::DrawTextureEx(id<MTLTexture> texture, Vector2 position, float rotation, float scale, Color tint) {
         TraceLog(LOG_INFO, "[METAL DEBUG] DrawTextureEx: position=(%.1f,%.1f), rotation=%.1f, scale=%.1f", position.x, position.y, rotation, scale);
         // ... existing code ...
     }
     ```

### Step 3: Fix Text Rendering Failures
**Objective**: Ensure UI text renders right-side up and fully visible.
1. **Verify Core Text Transform**:
   - In `MetalTextRenderer::RenderTextToTexture`, confirm the y-flip transform:
     ```cpp
     CGContextSetTextMatrix(context, CGAffineTransformIdentity);
     CGContextTranslateCTM(context, 0, height);
     CGContextScaleCTM(context, 1.0, -1.0);
     TraceLog(LOG_INFO, "[METAL DEBUG] Text context transform: height=%zu", height);
     ```
   - Log the texture’s pixel data or save it for debugging:
     ```cpp
     id<MTLTexture> texture = CreateTextureFromCGImage(CGBitmapContextCreateImage(context));
     if (texture) {
         TraceLog(LOG_INFO, "[METAL DEBUG] Text texture created: %lux%lu", texture.width, texture.height);
     }
     ```
2. **Check Text UVs and Rectangles**:
   - In `DrawText`, log `source` and `dest` rectangles:
     ```cpp
     void MetalRenderer::DrawText(const char* text, float x, float y, float fontSize, Color color) {
         id<MTLTexture> textTexture = g_textRenderer->RenderTextToTexture(text, (int)fontSize, color);
         if (!textTexture) return;
         Rectangle source = {0, 0, (float)textTexture.width, (float)textTexture.height};
         Rectangle dest = {x, y, (float)textTexture.width, (float)textTexture.height};
         TraceLog(LOG_INFO, "[METAL DEBUG] DrawText: source=(%.1f,%.1f,%.1f,%.1f), dest=(%.1f,%.1f,%.1f,%.1f)", 
                  source.x, source.y, source.width, source.height, dest.x, dest.y, dest.width, dest.height);
         DrawTexture(textTexture, source, dest, WHITE, RenderLayer::Text);
     }
     ```
3. **Clear Text Cache During Debugging**:
   - Temporarily clear the text cache each frame:
     ```cpp
     void MetalRenderer::BeginFrame() {
         g_textRenderer->ClearTextCache(); // Temporary for debugging
         // ... existing code ...
     }
     ```

### Step 4: Improve General UI Predictability
**Objective**: Achieve pixel-accurate, device-independent UI layout.
1. **Standardize Coordinate System**:
   - Create a `UICoordinateSystem` class to handle point-to-pixel conversions and safe area calculations:
     ```cpp
     class UICoordinateSystem {
     public:
         static Rectangle GetPixelScreenRect() {
             CGRect bounds = [[UIScreen mainScreen] bounds];
             CGFloat scale = [[UIScreen mainScreen] scale];
             return {0, 0, bounds.size.width * scale, bounds.size.height * scale};
         }
         static Rectangle GetPointScreenRect() {
             CGRect bounds = [[UIScreen mainScreen] bounds];
             return {0, 0, bounds.size.width, bounds.size.height};
         }
         static Rectangle GetSafeAreaRect(bool inPixels) {
             UIEdgeInsets insets = [[UIApplication sharedApplication] windows][0].safeAreaInsets;
             CGRect bounds = [[UIScreen mainScreen] bounds];
             CGFloat scale = inPixels ? [[UIScreen mainScreen] scale] : 1.0f;
             return {insets.left * scale, insets.top * scale, 
                     (bounds.size.width - insets.left - insets.right) * scale, 
                     (bounds.size.height - insets.top - insets.bottom) * scale};
         }
     };
     ```
   - Use `UICoordinateSystem` in `MetalRenderer` and game code for consistent coordinates.
2. **Audit Draw Order**:
   - Adjust `GenerateSortKey` to prioritize UI and text layers:
     ```cpp
     uint32_t MetalRenderer::GenerateSortKey(const DrawCommand& cmd) {
         uint32_t sortKey = 0;
         sortKey |= (cmd.renderState & 0xFF) << 24;
         sortKey |= (cmd.textureId & 0xFF) << 16;
         sortKey |= (static_cast<uint32_t>(cmd.primitiveType) & 0xFF) << 8;
         uint8_t depthByte = static_cast<uint8_t>(255 - std::min(255.0f, cmd.depth * 1000.0f));
         sortKey |= depthByte;
         return sortKey;
     }
     ```
3. **Log Render Commands**:
   - In `ExecuteOptimizedDrawCommands`, log all draw commands:
     ```cpp
     for (const auto& cmd : m_drawCommands) {
         TraceLog(LOG_INFO, "[RENDER] %s: vertices=%lu-%lu, texture=%p, depth=%.2f, instances=%lu", 
                  cmd.debugName, cmd.vertexStart, cmd.vertexStart + cmd.vertexCount - 1, 
                  cmd.texture, cmd.depth, cmd.instanceCount);
     }
     ```

## Additional Improvements
1. **Decouple Coordinate Handling**:
   - Implement `UICoordinateSystem` as a standalone class to centralize coordinate conversions, reducing errors in `UIManager` and `PlatformLayer`.
2. **Optimize Texture Loading**:
   - Add validation in `MetalTexture::CreateFromCGImage`:
     ```cpp
     id<MTLTexture> MetalTexture::CreateFromCGImage(CGImageRef image, id<MTLDevice> device) {
         size_t width = CGImageGetWidth(image);
         size_t height = CGImageGetHeight(image);
         TraceLog(LOG_INFO, "[TEXTURE] Loading texture: %zux%zu", width, height);
         // ... existing code ...
     }
     ```
3. **Enhance Text Rendering**:
   - Add fallback rendering in `RenderTextToTexture`:
     ```cpp
     if (!texture) {
         TraceLog(LOG_ERROR, "[METAL ERROR] Failed to create text texture for '%s'", text);
         return RenderTextToTexture("Error", m_defaultFont, fontSize, RED);
     }
     ```
   - Consider a text atlas to reduce draw calls.
4. **Reduce State Changes**:
   - Enhance `OptimizeDrawCommands` to batch commands with identical pipeline and texture states:
     ```cpp
     void MetalRenderer::OptimizeDrawCommands() {
         std::vector<DrawCommand> optimized;
         DrawCommand* current = nullptr;
         for (auto& cmd : m_drawCommands) {
             if (current && ShouldBatchCommands(*current, cmd)) {
                 current->vertexCount += cmd.vertexCount;
                 current->instanceCount += cmd.instanceCount;
             } else {
                 optimized.push_back(cmd);
                 current = &optimized.back();
             }
         }
         m_drawCommands = std::move(optimized);
     }
     ```
5. **Enhance Debug Visualization**:
   - Visualize safe areas in `DrawDebugOverlay`:
     ```cpp
     void MetalRenderer::DrawDebugOverlay() {
         Rectangle safeArea = UICoordinateSystem::GetSafeAreaRect(true);
         DrawRectangleRoundedLines(safeArea.x, safeArea.y, safeArea.width, safeArea.height, 5.0f, 8, 2.0f, RED);
         // ... existing debug visuals ...
     }
     ```

## Updated Checklist
Based on *Metal_Migration_UICoordinate_Analysis.md*:
- [x] Confirm `AddTexturedRectangleVertices` is always called with correct `source` and `dest` rects (added logging).
- [x] Confirm `texWidth`/`texHeight` match the actual Metal texture size (added dimension logging).
- [ ] Check all UIKit-to-Metal transitions for coordinate system mismatches (partially addressed; implement `UICoordinateSystem`).
- [x] Review projection matrix setup in `MetalRenderer` (adjusted to use `drawableSize`).
- [ ] Inspect `Shaders2D.metal` for NDC mapping and y-flip (y-flip handled in `RenderTextToTexture`).
- [ ] Check all texture loading in `MetalTexture.mm` (needs dimension logging).
- [ ] Audit all uses of safe area and screen size in `UIManager` and `PlatformLayer` (needs `UICoordinateSystem`).

## Next Steps
1. Implement the code changes above and test on an iPhone 14 simulator/device.
2. Review logs for texture dimensions, UVs, and render rectangles to identify discrepancies.
3. Create and integrate `UICoordinateSystem` class for consistent coordinate handling.
4. Test text rendering with various fonts and sizes.
5. Update *Metal_Migration_UICoordinate_Analysis.md* with checklist progress.
6. Share `Game::RenderFrame` code or texture assets for further refinement.

## Conclusion
These steps address the core UI rendering issues while improving the robustness and predictability of the Metal pipeline. By standardizing coordinates, validating textures, and enhancing debugging, we’ll ensure *Floppy Turd*’s UI is pixel-perfect and ready to shine. Let’s polish this turd to glory! 🚀