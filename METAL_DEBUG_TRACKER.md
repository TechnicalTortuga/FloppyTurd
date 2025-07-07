# FloppyTurd iOS Metal Main Menu Debug Tracker

## Analysis & Findings

### 1. Background Not Showing (Just Brown)
- **Expected:** main_menu_bg_mobile image should stretch to fill the screen.
- **Observed:** Only a solid brown color is shown.
- **Possible Causes:**
  - Metal texture for background is invalid or not loaded.
  - Draw call is skipped or not reaching Metal.
  - Render pipeline may be clearing after draw, or draw order is wrong.

### 2. Buttons Are Rectangles, No Text
- **Expected:** Buttons have text labels and proper colors.
- **Observed:** Buttons are just rectangles, no text.
- **Possible Causes:**
  - Font not loaded (glyph count 0), so text rendering is skipped.
  - Metal text renderer is not producing textures for text.
  - DrawTextEx is a no-op or stub.

### 3. Text Rendering Pipeline
- **AIGUI_ButtonRounded** calls DrawTextEx for label.
- **DrawTextEx** (RaylibCompat_iOS) routes to PlatformLayer/MetalRenderer.
- **MetalRenderer** uses MetalTextRenderer to create a texture from text and draws it.
- **Font fallback**: Whacky Joe fails, Helvetica should be used, but glyph count is 0.

---

## Metal Rendering Pipeline: Boilerplate Reference

### **A. Texture Rendering**
1. **Load Texture:**
   - Load image data (PNG, etc.) into memory.
   - Create a Metal texture (`id<MTLTexture>`) from the image data.
2. **Begin Frame:**
   - Begin a render pass on the current drawable.
   - Set up the pipeline state for textured rendering.
3. **Draw Call:**
   - Set the texture as a fragment resource.
   - Issue a draw call (e.g., two triangles for a quad) with the correct vertex and texture coordinates.
4. **End Frame:**
   - End the render pass and present the drawable.

### **B. Text Rendering**
1. **Font Loading:**
   - Load a font (TTF, system, etc.) and create a CoreText/UIFont/CTFont object.
2. **Text to Texture:**
   - Render the text string to a bitmap (using CoreText/CGContext).
   - Create a Metal texture from the bitmap.
3. **Draw Text:**
   - In the main render pass, draw the text texture as a quad at the desired position.
   - Use the same pipeline as for other textures, but with the text texture.

### **C. Common Pitfalls**
- Texture pointer is null or not a valid Metal texture.
- Render pass is cleared after drawing, erasing previous content.
- Draw calls are outside the render pass or not committed.
- Font is not loaded, so text rendering fails or produces an empty texture.
- Textures are not sized/scaled correctly to match the screen.

---

## Action Steps (for next chat)

### A. Logging/Debugging
- [x] Log background texture validity (pointer, width, height, Metal pointer) before drawing.
- [x] Log all DrawTexture and DrawText calls in MetalRenderer.
- [x] Log all DrawTextEx calls in RaylibCompat_iOS.
- [x] Log AIGUI_ButtonRounded text rendering parameters.
- [x] Log MetalTextRenderer::RenderTextToTexture font fallback and Metal texture creation.

### B. Next Steps
- [ ] Run the app and review logs for:
  - Is the background texture valid and a real Metal texture?
  - Are DrawTexture and DrawText being called with valid parameters?
  - Is the fallback font loaded and has glyphs?
  - Is RenderTextToTexture creating a Metal texture for text?
  - Are there any Metal errors or warnings?
- [ ] If background texture is invalid, trace back to ResourceManager and asset loading.
- [ ] If text rendering fails, check font loading and fallback logic in MetalTextRenderer.
- [ ] If all draw calls are correct but nothing appears, check Metal render pass setup and clear order.

---

**Continue with log analysis and targeted fixes based on these debug outputs.** 