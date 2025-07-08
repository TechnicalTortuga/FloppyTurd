# Metal UI Best Practices and Refactor Plan

## Analysis: iOS Metal UI, Screen Size, and Safe Area

### Key Findings from Apple and Industry Best Practices

1. **Render at Native Pixel Size**
   - Always render Metal drawables at the *exact pixel size* of the display.
   - Use `UIScreen.mainScreen.nativeBounds` and `nativeScale` for true pixel dimensions.
   - MTKView handles this automatically; for custom Metal layers, set `contentScaleFactor` and `drawableSize` to match `nativeScale`.

2. **Use Safe Area Insets for UI Layout**
   - Use `UIView.safeAreaInsets` or `safeAreaLayoutGuide.layoutFrame` to avoid notches and home indicators.
   - Layout UI elements (logo, buttons) within the safe area, but render backgrounds to the *full screen* (native pixel size).

3. **Points vs. Pixels**
   - UIKit uses points; Metal uses pixels. Always convert points to pixels (`points * nativeScale`) before rendering.

4. **MTKView and Drawable Size**
   - MTKView sets `drawableSize` automatically. For custom layers:
     ```objc
     CGSize drawableSize = self.bounds.size;
     drawableSize.width *= screen.nativeScale;
     drawableSize.height *= screen.nativeScale;
     metalLayer.drawableSize = drawableSize;
     ```

5. **UI Scaling and Anchoring**
   - Use a consistent scale factor for UI, but cap it to avoid oversized elements.
   - Anchor UI elements using the safe area, always converting to pixel coordinates.

6. **Debug Overlays**
   - Only enable debug overlays when needed. Disable for production/user testing.

---

## Action Plan for FloppyTurd Refactor

### 1. **Screen and Safe Area Handling**
- Query `nativeBounds` and `nativeScale` for true pixel size.
- Set Metal layer's `drawableSize` to match native pixel size.
- Use `safeAreaInsets` for UI layout, but always convert to pixels.

### 2. **Background Rendering**
- Render backgrounds to the full screen (native pixel size), not just the safe area.

### 3. **UI Element Layout**
- Layout logo and buttons within the safe area, but ensure all coordinates/sizes are in pixels.
- Center the logo in the safe area and scale it (e.g., up to 2.4x) but never exceed safe area width.
- Scale buttons and text for readability, using pixel units.

### 4. **UIManager Refactor**
- Refactor UIManager to:
  - Store both point and pixel dimensions.
  - Provide helpers to convert between points and pixels.
  - Anchor and scale UI elements in pixel space.

### 5. **Debug Overlay**
- Add a toggle for debug overlays; ensure they are off by default.

### 6. **Testing and Validation**
- Test on multiple device sizes and orientations.
- Use Xcode's View Debugger and Metal Frame Debugger to verify correct layout and rendering.

---

## References
- [Apple Metal Best Practices Guide](https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/NativeScreenScale.html)
- [QA1909: Supporting native screen scale in your graphics application](https://developer.apple.com/library/archive/qa/qa1909/_index.html)
- [WWDC19: Delivering Optimized Metal Apps and Games](https://developer.apple.com/videos/play/wwdc2019/606)

---

## Next Steps
1. Refactor initialization and UI layout code to follow these best practices.
2. Update UIManager and all UI rendering code to use pixel coordinates.
3. Test and iterate for correctness and visual quality. 