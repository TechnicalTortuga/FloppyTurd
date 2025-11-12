# Rounded Rectangle Rendering TODO

## Status
**NOT YET IMPLEMENTED** - The `cornerRadius` field has been added to `UIShape` component, but the actual rendering is not yet supported.

## What's Been Done
1. ✅ Added `cornerRadius` field to `UIShape` struct in `GameComponents.h`
2. ✅ Initialized `cornerRadius` in all `UIShape` constructors
3. ✅ Set `cornerRadius = 20.0f` for shooting zone in TutorialState

## What Still Needs to Be Done

### 1. Add Command Type
**File**: `/Users/aimac/Development/FloppyTurd/src/Engine/Platform/PlatformDelegates.h`

Add to `CommandType` enum (around line 136):
```cpp
CMD_DRAW_ROUNDED_RECTANGLE = 70,
```

### 2. Add RenderCommandData Support
**File**: `/Users/aimac/Development/FloppyTurd/src/Engine/Platform/PlatformDelegates.h`

Add to `RenderCommandData` struct (around line 234):
```cpp
float cornerRadius = 0.0f;  // For rounded rectangles
```

### 3. Add Renderer Delegate Function Pointer
**File**: `/Users/aimac/Development/FloppyTurd/src/Engine/Platform/PlatformDelegates.h`

Add to `RendererDelegate` struct (around line 547):
```cpp
void (*drawRoundedRectangle)(float x, float y, float width, float height, float cornerRadius, float r, float g, float b, float a);
```

And initialize in constructor (around line 575):
```cpp
drawRoundedRectangle(nullptr),
```

### 4. Implement Metal Renderer Support
**File**: `/Users/aimac/Development/FloppyTurd/iOS/MetalRenderer.swift`

Add new rendering function:
```swift
func drawRoundedRectangle(x: Float, y: Float, width: Float, height: Float, cornerRadius: Float, r: Float, g: Float, b: Float, a: Float) {
    // Use Metal to draw a rounded rectangle
    // Options:
    // 1. Use UIBezierPath and convert to vertices
    // 2. Use a rounded rectangle shader
    // 3. Draw 9-patch style with corner arcs
}
```

### 5. Add Threading Proxy Support
**File**: `/Users/aimac/Development/FloppyTurd/src/Engine/Threading/ThreadingProxy.cpp`

Add command handling:
```cpp
case CommandType::CMD_DRAW_ROUNDED_RECTANGLE:
    if (delegates.renderer.drawRoundedRectangle) {
        delegates.renderer.drawRoundedRectangle(
            cmd.data.x, cmd.data.y, cmd.data.width, cmd.data.height,
            cmd.data.cornerRadius,
            cmd.data.r, cmd.data.g, cmd.data.b, cmd.data.a
        );
    }
    break;
```

### 6. Update RenderSystem
**File**: `/Users/aimac/Development/FloppyTurd/src/Engine/Systems/RenderSystem.cpp`

Update the UIShape rendering code to check for `cornerRadius` and use the rounded rectangle command:
```cpp
if (shape->cornerRadius > 0.0f) {
    // Use CMD_DRAW_ROUNDED_RECTANGLE
    RenderCommand cmd(CommandType::CMD_DRAW_ROUNDED_RECTANGLE);
    cmd.data.x = transform->position.x;
    cmd.data.y = transform->position.y;
    cmd.data.width = shape->width;
    cmd.data.height = shape->height;
    cmd.data.cornerRadius = shape->cornerRadius;
    cmd.data.r = shape->color.r / 255.0f;
    cmd.data.g = shape->color.g / 255.0f;
    cmd.data.b = shape->color.b / 255.0f;
    cmd.data.a = shape->color.a / 255.0f;
    EnqueueRenderCommand(cmd);
} else {
    // Use existing CMD_DRAW_RECTANGLE
}
```

## Alternative: Use Sprite Instead
As a simpler workaround, you could:
1. Create a rounded rectangle sprite asset in your art software
2. Use a 9-patch sprite that stretches
3. Replace the UIShape with a Sprite component

## Estimated Effort
- **Full Implementation**: 2-3 hours (Metal shader work, testing, etc.)
- **Sprite Workaround**: 30 minutes (create asset, update component)

## Priority
**LOW** - The square rectangle works fine functionally. This is purely a visual polish item.
