# Quick Fix: Reduce MetalRenderer Log Spam

## 🎯 **Problem**
MetalRenderer debug logs are drowning out critical input system logs, making it impossible to debug the coordinate mismatch issue.

## 🔧 **Immediate Solution**

### **File to Edit**: `src/iOS/Rendering/MetalRenderer.swift`

Comment out or change to TRACE level these specific debug log lines:

#### **Lines ~945-948: Matrix Debug Logs**
```swift
// BEFORE:
log("DEBUG: MVP Matrix for sprite at (\(x), \(y)):", level: .debug)
log("DEBUG: Projection Matrix: \(projectionMatrix)", level: .debug)
log("DEBUG: Model Matrix: \(modelMatrix)", level: .debug)
log("DEBUG: Final MVP Matrix: \(mvpMatrix)", level: .debug)

// AFTER:
// log("DEBUG: MVP Matrix for sprite at (\(x), \(y)):", level: .debug)
// log("DEBUG: Projection Matrix: \(projectionMatrix)", level: .debug)
// log("DEBUG: Model Matrix: \(modelMatrix)", level: .debug)
// log("DEBUG: Final MVP Matrix: \(mvpMatrix)", level: .debug)
```

#### **Line ~975: Sprite Render Debug**
```swift
// BEFORE:
log("drawSpriteScaled: Rendered sprite \(textureHandle) at (\(x), \(y)) with scale (\(scaleX), \(scaleY)) rotation \(rotation)°", level: .debug)

// AFTER:
// log("drawSpriteScaled: Rendered sprite \(textureHandle) at (\(x), \(y)) with scale (\(scaleX), \(scaleY)) rotation \(rotation)°", level: .debug)
```

#### **Lines ~1067, ~1129: Other Sprite Debug Logs**
```swift
// BEFORE:
log("drawSpriteScaledWithSource: Rendered sprite \(textureHandle) with source rect at (\(x), \(y))", level: .debug)
log("drawSpriteScaledCentered: Rendered sprite \(textureHandle) centered at (\(x), \(y))", level: .debug)

// AFTER:
// log("drawSpriteScaledWithSource: Rendered sprite \(textureHandle) with source rect at (\(x), \(y))", level: .debug)
// log("drawSpriteScaledCentered: Rendered sprite \(textureHandle) centered at (\(x), \(y))", level: .debug)
```

#### **🖼️ Sprite Drawing Debug Logs**
Find and comment out logs that start with:
```swift
log("🖼️ Drawing sprite...", level: .debug)
```

## 🎯 **Alternative: Use TRACE Level**
Instead of commenting out, change `.debug` to `.trace` to keep logs but at a lower priority:

```swift
// BEFORE:
log("DEBUG: MVP Matrix for sprite at (\(x), \(y)):", level: .debug)

// AFTER:
log("DEBUG: MVP Matrix for sprite at (\(x), \(y)):", level: .trace)
```

## ✅ **After This Fix**
You should see input logs clearly:
- 🔗 ThreadingProxy logs
- 🎮 InputManager logs  
- 🎯 MainMenuState logs
- 🔄 TouchInputHandler logs

## 🚀 **Next Steps**
1. Apply this fix immediately
2. Run the app and touch the screen
3. Check logs for input pipeline visibility
4. Implement the comprehensive logging from `INPUT_PIPELINE_ANALYSIS.md`