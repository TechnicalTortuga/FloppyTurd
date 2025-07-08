# Floppy Turd Font Fix Report

## Issue Overview

Fonts in *Floppy Turd* are rendering as a black bar instead of individual characters. This issue likely stems from the font atlas generation or rendering pipeline in the Metal-based iOS implementation using Raylib. This report analyzes the current implementation, identifies potential causes, and provides actionable steps to resolve the issue and ensure crisp, turd-tastic text rendering.

## Current Implementation

### Font Atlas Generation (`MetalTextRenderer::GenerateFontAtlas`)
- **Purpose**: Creates a texture atlas containing glyphs for ASCII printable characters (32–126).
- **Process**:
  - Creates a 512x512 RGBA bitmap context using `CGBitmapContextCreate`.
  - Iterates over characters, drawing each glyph using Core Text (`CTLineDraw`).
  - Stores glyph metrics (value, offsetX, offsetY, advanceX) in `font.glyphs`.
  - Stores normalized UV coordinates in `font.recs` (x, y, width, height as fractions of atlas size).
  - Uploads pixel data to a Metal texture (`id<MTLTexture>`) using `[metalTexture replaceRegion:...]`.
  - Assigns the texture to `font.texture.texture`.
- **File**: `MetalTextRenderer.mm`

### Text Rendering (`DrawTextEx` in `RaylibCompat_iOS.mm`)
- **Purpose**: Renders text by drawing glyph quads from the atlas.
- **Process**:
  - Iterates over input string characters.
  - Looks up glyph index (ASCII value - 32).
  - Retrieves UV rectangle from `font.recs`.
  - Computes screen-space destination rectangle, scaled by `fontSize / font.baseSize`.
  - Calls `DrawTexturePro_iOS` to draw each glyph quad.
  - Falls back to `DrawText` if the font atlas is invalid.
- **File**: `RaylibCompat_iOS.mm`

### Texture Drawing (`DrawTexturePro_iOS`)
- **Purpose**: Draws a textured quad using Metal.
- **Process**:
  - Validates the texture and converts it to `id<MTLTexture>`.
  - Passes texture, source/destination rectangles, origin, rotation, and tint to `PlatformLayer::DrawTexture`.
- **File**: `RaylibCompat_iOS.mm`

## Potential Causes of the Black Bar
1. **Empty/Corrupt Atlas Texture**:
   - Glyphs not drawn correctly, leaving the atlas empty or black.
   - Core Text drawing failure (e.g., invalid font or context).
2. **Incorrect UV Coordinates**:
   - `font.recs` contains invalid or zeroed UVs, sampling a black atlas region.
3. **Texture Binding/Sampler Issue**:
   - Metal pipeline fails to bind the atlas texture or uses incorrect sampler settings.
4. **Shader Issue**:
   - Fragment shader incorrectly samples the texture or outputs black.
5. **Glyph Metrics Issue**:
   - Incorrect `offsetX`, `offsetY`, or `advanceX` in `font.glyphs` causes misaligned or overlapping glyphs.
6. **Scaling/Positioning Error**:
   - Destination rectangles in `DrawTextEx` are miscomputed, causing glyphs to overlap or draw off-screen.

## Actionable Steps for IDE Implementation

Below are the steps to implement the fixes and debug the font rendering issue in your IDE (e.g., Xcode). Each step includes code changes, file locations, and verification tasks to ensure the font atlas and rendering pipeline work correctly.

### Step 1: Update `GenerateFontAtlas` for Robustness
**Goal**: Enhance atlas generation to ensure glyphs are drawn and validate pixel data.
**File**: `MetalTextRenderer.mm`
**Changes**:
- Add pixel data validation to check for non-zero alpha pixels.
- Improve glyph metrics storage using `CTFontGetBoundingRectsForGlyphs`.
- Add debug logging for atlas creation and pixel data.
- Ensure proper resource cleanup.

**Code**:
```cpp
bool MetalTextRenderer::GenerateFontAtlas(Font& font) {
    if (!font.ctFont || !m_device) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Invalid font or device");
        return false;
    }

    TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: Generating atlas for font with %d glyphs", (int)CTFontGetGlyphCount((CTFontRef)font.ctFont));

    const int startChar = 32;
    const int endChar = 126;
    const int charCount = endChar - startChar + 1;

    const int glyphSize = font.baseSize + 4;
    const int atlasSize = 512;
    const int glyphsPerRow = atlasSize / glyphSize;
    const int glyphsPerCol = atlasSize / glyphSize;

    if (charCount > glyphsPerRow * glyphsPerCol) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Too many characters (%d) for atlas size %dx%d", charCount, atlasSize, atlasSize);
        return false;
    }

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    uint8_t* pixelData = (uint8_t*)calloc(atlasSize * atlasSize * 4, 1);
    CGContextRef context = CGBitmapContextCreate(pixelData, atlasSize, atlasSize, 8, atlasSize * 4, colorSpace, kCGImageAlphaPremultipliedLast);

    if (!context) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Failed to create CGContext");
        free(pixelData);
        CGColorSpaceRelease(colorSpace);
        return false;
    }

    CGContextClearRect(context, CGRectMake(0, 0, atlasSize, atlasSize));
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    CGContextTranslateCTM(context, 0, atlasSize);
    CGContextScaleCTM(context, 1.0, -1.0);

    CGFloat components[] = {1.0f, 1.0f, 1.0f, 1.0f};
    CGColorRef textColor = CGColorCreate(colorSpace, components);

    font.glyphCount = charCount;
    font.glyphs = (void*)malloc(charCount * sizeof(int) * 4);
    font.recs = (Rectangle*)malloc(charCount * sizeof(Rectangle));

    if (!font.glyphs || !font.recs) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Failed to allocate glyph arrays");
        CGContextRelease(context);
        free(pixelData);
        CGColorSpaceRelease(colorSpace);
        CGColorRelease(textColor);
        return false;
    }

    int nonzeroPixelCount = 0;
    for (int i = 0; i < charCount; i++) {
        int charCode = startChar + i;
        int row = i / glyphsPerRow;
        int col = i % glyphsPerRow;
        int x = col * glyphSize + 2;
        int y = row * glyphSize + 2;

        char charStr[2] = {(char)charCode, 0};
        NSString* string = [NSString stringWithUTF8String:charStr];
        NSDictionary* attributes = @{
            (NSString*)kCTFontAttributeName: (__bridge id)font.ctFont,
            (NSString*)kCTForegroundColorAttributeName: (__bridge id)textColor
        };
        NSAttributedString* attributedString = [[NSAttributedString alloc] initWithString:string attributes:attributes];
        CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)attributedString);
        CGContextSetTextPosition(context, x, y + font.baseSize);
        CTLineDraw(line, context);

        CFArrayRef glyphRuns = CTLineGetGlyphRuns(line);
        if (CFArrayGetCount(glyphRuns) > 0) {
            CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(glyphRuns, 0);
            CFIndex glyphCount = CTRunGetGlyphCount(run);
            if (glyphCount > 0) {
                CGGlyph glyph;
                CTRunGetGlyphs(run, CFRangeMake(0, 1), &glyph);
                CGRect bounds;
                CTFontGetBoundingRectsForGlyphs((CTFontRef)font.ctFont, kCTFontHorizontalOrientation, &glyph, &bounds, 1);
                int* glyphData = (int*)font.glyphs;
                glyphData[i * 4 + 0] = charCode;
                glyphData[i * 4 + 1] = (int)bounds.origin.x;
                glyphData[i * 4 + 2] = (int)bounds.origin.y;
                glyphData[i * 4 + 3] = (int)CTFontGetAdvancesForGlyphs((CTFontRef)font.ctFont, kCTFontHorizontalOrientation, &glyph, nullptr, 1);
                font.recs[i] = {
                    (float)x / atlasSize,
                    (float)y / atlasSize,
                    (float)glyphSize / atlasSize,
                    (float)glyphSize / atlasSize
                };
                for (int gy = y; gy < y + glyphSize; gy++) {
                    for (int gx = x; gx < x + glyphSize; gx++) {
                        int index = (gy * atlasSize + gx) * 4 + 3;
                        if (pixelData[index] > 0) {
                            nonzeroPixelCount++;
                            break;
                        }
                    }
                }
            }
        }
        CFRelease(line);
    }

    TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: Nonzero alpha pixels: %d", nonzeroPixelCount);
    if (nonzeroPixelCount == 0) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Atlas appears empty");
    } else {
        TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: First 8 bytes: %02x %02x %02x %02x %02x %02x %02x %02x",
                 pixelData[0], pixelData[1], pixelData[2], pixelData[3],
                 pixelData[4], pixelData[5], pixelData[6], pixelData[7]);
    }

    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                           width:atlasSize
                                                                                          height:atlasSize
                                                                                       mipmapped:NO];
    id<MTLTexture> metalTexture = [m_device newTextureWithDescriptor:textureDesc];
    if (!metalTexture) {
        TraceLog(LOG_ERROR, "[METAL ERROR] GenerateFontAtlas: Failed to create Metal texture");
        CGContextRelease(context);
        free(pixelData);
        CGColorSpaceRelease(colorSpace);
        CGColorRelease(textColor);
        return false;
    }

    [metalTexture replaceRegion:MTLRegionMake2D(0, 0, atlasSize, atlasSize)
                   mipmapLevel:0
                     withBytes:pixelData
                   bytesPerRow:atlasSize * 4];

    font.texture.id = (unsigned int)(uintptr_t)metalTexture;
    font.texture.width = atlasSize;
    font.texture.height = atlasSize;
    font.texture.mipmaps = 1;
    font.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    font.texture.texture = (__bridge void*)metalTexture;

    TraceLog(LOG_INFO, "[METAL DEBUG] GenerateFontAtlas: Generated atlas %dx%d with %d glyphs, texture=%p",
             atlasSize, atlasSize, charCount, metalTexture);

    CGContextRelease(context);
    free(pixelData);
    CGColorSpaceRelease(colorSpace);
    CGColorRelease(textColor);
    return true;
}
```

**IDE Tasks**:
1. Open `MetalTextRenderer.mm` in Xcode.
2. Replace the existing `GenerateFontAtlas` method with the code above.
3. Ensure `<CoreText/CoreText.h>` and `<CoreGraphics/CoreGraphics.h>` are included in the file.
4. Verify that `TraceLog` is defined (part of Raylib’s logging system).

**Verification**:
- Build and run the project.
- Check Xcode’s console for logs:
  - Look for `[METAL DEBUG] GenerateFontAtlas: Nonzero alpha pixels: N` (N should be > 0).
  - Confirm `[METAL DEBUG] GenerateFontAtlas: Generated atlas 512x512 with 95 glyphs, texture=<non-null-pointer>`.

### Step 2: Enhance `DrawTextEx` for Debugging
**Goal**: Add validation and logging to ensure correct glyph rendering.
**File**: `RaylibCompat_iOS.mm`
**Changes**:
- Validate UV coordinates and glyph indices.
- Apply glyph offsets for precise positioning.
- Log glyph details for debugging.

**Code**:
```cpp
void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
    TraceLog(LOG_INFO, "[RaylibCompat_iOS] DrawTextEx: text='%s', pos=(%.1f, %.1f), fontSize=%.1f, spacing=%.1f, color=(%d,%d,%d,%d), font.ctFont=%p, texture.id=%u",
             text, position.x, position.y, fontSize, spacing, tint.r, tint.g, tint.b, tint.a, font.ctFont, font.texture.id);

    if (font.texture.id != 0 && font.recs && font.glyphs && font.texture.texture) {
        float scale = fontSize / (float)font.baseSize;
        float x = position.x;
        float y = position.y;
        const int startChar = 32;
        const int endChar = 126;
        int* glyphData = (int*)font.glyphs;

        for (const char* p = text; *p; p++) {
            unsigned char c = (unsigned char)*p;
            if (c < startChar || c > endChar) {
                TraceLog(LOG_WARNING, "[RaylibCompat_iOS] DrawTextEx: Skipping unsupported char '%c' (code=%d)", c, c);
                x += fontSize * 0.5f;
                continue;
            }
            int index = c - startChar;
            if (index < 0 || index >= font.glyphCount) {
                TraceLog(LOG_WARNING, "[RaylibCompat_iOS] DrawTextEx: Invalid glyph index %d for char '%c'", index, c);
                x += fontSize * 0.5f;
                continue;
            }

            Rectangle src = {
                font.recs[index].x * font.texture.width,
                font.recs[index].y * font.texture.height,
                font.recs[index].width * font.texture.width,
                font.recs[index].height * font.texture.height
            };
            Rectangle dst = {
                x + (glyphData[index * 4 + 1] * scale),
                y + (glyphData[index * 4 + 2] * scale),
                src.width * scale,
                src.height * scale
            };

            if (src.width <= 0 || src.height <= 0 || src.x < 0 || src.y < 0 ||
                src.x + src.width > font.texture.width || src.y + src.height > font.texture.height) {
                TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTextEx: Invalid UVs for char '%c': src=(%.3f,%.3f,%.3f,%.3f), texture=(%d,%d)",
                         c, src.x, src.y, src.width, src.height, font.texture.width, font.texture.height);
                x += fontSize * 0.5f;
                continue;
            }

            TraceLog(LOG_INFO, "[RaylibCompat_iOS] Drawing char '%c' (index=%d): src=(%.3f,%.3f,%.3f,%.3f), dst=(%.1f,%.1f,%.1f,%.1f)",
                     c, index, src.x, src.y, src.width, src.height, dst.x, dst.y, dst.width, dst.height);

            Vector2 origin = {0, 0};
            DrawTexturePro_iOS(font.texture, src, dst, origin, 0.0f, tint);

            int advance = glyphData[index * 4 + 3];
            x += (advance > 0 ? advance : src.width) * scale + spacing;
        }
    } else {
        TraceLog(LOG_WARNING, "[RaylibCompat_iOS] DrawTextEx: Invalid font data, falling back to DrawText");
        DrawText(text, (int)position.x, (int)position.y, (int)fontSize, tint);
    }
}
```

**IDE Tasks**:
1. Open `RaylibCompat_iOS.mm` in Xcode.
2. Replace the existing `DrawTextEx` function with the code above.
3. Ensure `raylib.h` is included for `Font`, `Vector2`, `Rectangle`, and `Color` types.

**Verification**:
- Build and run.
- Check console for `[RaylibCompat_iOS] Drawing char` logs, ensuring valid `src` and `dst` rectangles (non-zero, within texture bounds).
- Look for warnings about invalid glyphs or UVs.

### Step 3: Improve `DrawTexturePro_iOS`
**Goal**: Validate texture and rectangles during rendering.
**File**: `RaylibCompat_iOS.mm`
**Changes**:
- Add checks for texture validity and rectangle dimensions.
- Log detailed texture and rectangle info.

**Code**:
```cpp
void DrawTexturePro_iOS(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
    if (texture.texture == NULL) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: SKIP invalid texture");
        return;
    }

    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.texture;
    if (!metalTexture) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: Invalid texture pointer (NULL)");
        return;
    }

    if (metalTexture.width == 0 || metalTexture.height == 0) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: Texture has invalid dimensions (w=%lu, h=%lu)",
                 (unsigned long)metalTexture.width, (unsigned long)metalTexture.height);
        return;
    }

    if (source.width <= 0 || source.height <= 0 || dest.width <= 0 || dest.height <= 0) {
        TraceLog(LOG_ERROR, "[RaylibCompat_iOS] DrawTexturePro_iOS: Invalid rectangles: src=(%.3f,%.3f,%.3f,%.3f), dest=(%.1f,%.1f,%.1f,%.1f)",
                 source.x, source.y, source.width, source.height, dest.x, dest.y, dest.width, dest.height);
        return;
    }

    TraceLog(LOG_INFO, "[RaylibCompat_iOS] DrawTexturePro_iOS: Drawing texture=%p (w=%lu, h=%lu), src=(%.3f,%.3f,%.3f,%.3f), dest=(%.1f,%.1f,%.1f,%.1f), origin=(%.1f,%.1f), rotation=%.1f, tint=(%d,%d,%d,%d)",
             texture.texture, (unsigned long)metalTexture.width, (unsigned long)metalTexture.height,
             source.x, source.y, source.width, source.height,
             dest.x, dest.y, dest.width, dest.height,
             origin.x, origin.y, rotation, tint.r, tint.g, tint.b, tint.a);

    PlatformLayer::GetInstance().DrawTexture((__bridge void*)metalTexture, dest.x, dest.y, dest.width, dest.height, tint);
}
```

**IDE Tasks**:
1. Replace the `DrawTexturePro_iOS` function in `RaylibCompat_iOS.mm`.
2. Ensure `PlatformLayer` and its `DrawTexture` method are accessible.

**Verification**:
- Check console for `[RaylibCompat_iOS] DrawTexturePro_iOS` logs, confirming valid texture pointers and rectangle dimensions.

### Step 4: (Optional) Add Atlas PNG Export
**Goal**: Save the font atlas as a PNG for visual inspection.
**File**: `MetalTextRenderer.mm`
**Changes**:
- Add a `SaveAtlasToPNG` method to dump the atlas texture.
- Call it after `GenerateFontAtlas` for debugging.

**Code**:
```cpp
void MetalTextRenderer::SaveAtlasToPNG(const Font& font, const char* fileName) {
    if (!font.texture.texture) {
        TraceLog(LOG_ERROR, "[METAL ERROR] SaveAtlasToPNG: No texture to save");
        return;
    }

    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)font.texture.texture;
    int width = font.texture.width;
    int height = font.texture.height;

    uint8_t* pixelData = (uint8_t*)malloc(width * height * 4);
    [metalTexture getBytes:pixelData bytesPerRow:width * 4 fromRegion:MTLRegionMake2D(0, 0, width, height) mipmapLevel:0];

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(pixelData, width, height, 8, width * 4, colorSpace, kCGImageAlphaPremultipliedLast);
    CGImageRef cgImage = CGBitmapContextCreateImage(context);

    NSString* path = [NSString stringWithUTF8String:fileName];
    NSURL* url = [NSURL fileURLWithPath:path];
    CGImageDestinationRef destination = CGImageDestinationCreateWithURL((__bridge CFURLRef)url, kUTTypePNG, 1, nullptr);
    if (destination) {
        CGImageDestinationAddImage(destination, cgImage, nullptr);
        CGImageDestinationFinalize(destination);
        CFRelease(destination);
        TraceLog(LOG_INFO, "[METAL DEBUG] SaveAtlasToPNG: Saved atlas to %s", fileName);
    } else {
        TraceLog(LOG_ERROR, "[METAL ERROR] SaveAtlasToPNG: Failed to create image destination for %s", fileName);
    }

    CGImageRelease(cgImage);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    free(pixelData);
}
```

**IDE Tasks**:
1. Add the `SaveAtlasToPNG` method to `MetalTextRenderer.mm`.
2. Add `#include <MobileCoreServices/MobileCoreServices.h>` for `kUTTypePNG`.
3. In `Game::RenderFrame` or after `LoadFont`, call:
   ```cpp
   g_textRenderer->SaveAtlasToPNG(font, "/tmp/font_atlas.png");
   ```
4. Retrieve the PNG from the iOS device/simulator (e.g., via Xcode’s Devices and Simulators window).

**Verification**:
- Check `/tmp/font_atlas.png` for a 512x512 image with white glyphs on a transparent background.
- If empty or black, investigate `GenerateFontAtlas` for Core Text issues.

### Step 5: Verify Metal Shader
**Goal**: Ensure the fragment shader samples the atlas correctly.
**File**: Metal shader file (e.g., `shaders.metal`)
**Changes**:
- Verify or add a fragment shader for texture sampling.

**Code** (Add to `shaders.metal`):
```metal
fragment float4 fragmentShader(VertexOut input [[stage_in]],
                              texture2d<float> texture [[texture(0)]],
                              sampler textureSampler [[sampler(0)]],
                              constant float4 &tint [[buffer(0)]]) {
    float4 color = texture.sample(textureSampler, input.texCoord);
    return color * tint;
}
```

**IDE Tasks**:
1. Open your Metal shader file in Xcode.
2. Ensure the fragment shader matches the above or correctly samples the texture.
3. In `MetalRenderer::Initialize`, verify the sampler state:
   ```objc
   MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
   samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
   samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
   id<MTLSamplerState> samplerState = [_device newSamplerStateWithDescriptor:samplerDesc];
   ```
4. Check for shader compilation errors in `MetalRenderer` logs.

**Verification**:
- Confirm no shader compilation errors in the console.
- Ensure the texture is bound to slot 0 in `MetalRenderer::DrawTexture`.

### Step 6: Test and Debug
**Goal**: Validate the fixes and inspect output.
**IDE Tasks**:
1. Build and run *Floppy Turd* on an iOS device or simulator.
2. Add a test draw call in `Game::RenderFrame`:
   ```cpp
   Font font = GetFontDefault();
   DrawTextEx(font, "Hello, Floppy Turd!", {100, 100}, 32, 2, WHITE);
   ```
3. Open Xcode’s console and filter for `[METAL DEBUG]` and `[RaylibCompat_iOS]`.
4. Check logs for:
   - Non-zero pixel count in `GenerateFontAtlas`.
   - Valid UVs and rectangles in `DrawTextEx` and `DrawTexturePro_iOS`.
   - No errors for invalid textures or rectangles.
5. If using `SaveAtlasToPNG`, retrieve and inspect the PNG file.

**Verification**:
- Text should render as individual characters, not a black bar.
- If the issue persists:
  - **Empty Atlas**: Check `nonzeroPixelCount` and PNG output. Try a different font (e.g., "Arial") or increase `font.baseSize`.
  - **Invalid UVs**: Verify `font.recs` calculations in `GenerateFontAtlas`.
  - **Shader Issue**: Debug texture binding in `MetalRenderer::DrawTexture`.
  - **Positioning Issue**: Check `offsetX`, `offsetY`, and `advanceX` in `font.glyphs`.

## Next Steps for *Floppy Turd*
1. **Optimize Atlas Size**:
   - Dynamically calculate atlas size based on `charCount` and `glyphSize` if needed.
2. **Font Caching**:
   - Cache fonts in `MetalTextRenderer` to avoid regenerating atlases.
3. **Polish UI**:
   - Integrate text into score displays, menus, etc., with animations (e.g., wobbling text) to match the game’s quirky vibe.
4. **Custom Font**:
   - Add a custom TTF font via the asset catalog (e.g., `LoadFont("asset://floppy_turd.ttf")`) for a unique look.

## Debugging Tips
- Use Xcode’s Metal Frame Capture to inspect texture contents and shader inputs.
- If logs indicate an empty atlas, verify `font.ctFont` and Core Text setup.
- Share console logs or PNG output with your team (or Grok!) for further analysis.

Let’s make those fonts pop and keep *Floppy Turd* soaring to polished glory!