# Platform Types Cleanup & Refactor

## Objective
Remove all Raylib struct definitions from `PlatformTypes.h` and `PlatformAPI.h`, ensuring that only platform-agnostic wrappers and helpers remain. All Raylib types (e.g., `Texture2D`, `Color`, `Vector2`, etc.) should be used directly from Raylib headers.

---

## Why?
- Prevent ODR/ABI issues and subtle bugs.
- Ensure compatibility with Raylib updates.
- Maintain a clean separation between platform abstraction and third-party library types.

---

## Raylib Types to Remove from Platform Abstraction
```
struct Vector2;
struct Vector3;
struct Vector4;
struct Matrix;
struct Color;
struct Rectangle;
struct Image;
struct Texture;
struct RenderTexture;
struct NPatchInfo;
struct GlyphInfo;
struct Font;
struct Camera2D;
struct Camera3D;
struct Shader;
struct MaterialMap;
struct Material;
struct Mesh;
struct Model;
struct ModelAnimation;
struct Transform;
struct BoneInfo;
struct Ray;
struct RayCollision;
struct BoundingBox;
struct Wave;
struct AudioStream;
struct Sound;
struct Music;
struct VrDeviceInfo;
struct VrStereoConfig;
struct FilePathList;
struct AutomationEvent;
struct AutomationEventList;
```

---

## Refactor Plan

### 1. Remove Raylib Struct Definitions
- [ ] Remove all Raylib struct definitions from `PlatformTypes.h` and `PlatformAPI.h`.

### 2. Update Includes
- [ ] Ensure all files that use these types include the correct Raylib headers (e.g., `#include "raylib.h"`).

### 3. Audit for Redefinitions
- [ ] Search for accidental redefinitions or shadowing of Raylib types in the codebase.

### 4. Keep Only Platform Wrappers
- [ ] Ensure only platform-agnostic wrappers, helpers, and utility functions remain in `PlatformAPI`/`PlatformTypes`.

---

## Progress Tracker

| Task | Status |
|------|--------|
| Remove struct definitions from PlatformTypes.h | ⬜ |
| Remove struct definitions from PlatformAPI.h   | ⬜ |
| Update includes in all usage sites            | ⬜ |
| Audit for accidental redefinitions            | ⬜ |
| Keep only wrappers/utilities in abstraction   | ⬜ |

---

## Notes
- If a file needs a Raylib type, always include the Raylib header.
- Only forward-declare Raylib types if absolutely necessary (and never access members in that case).
- iOS-specific implementations should wrap Raylib types only in implementation files, not in cross-platform headers.

---

## Next Steps
- Begin with `PlatformTypes.h` and `PlatformAPI.h` cleanup.
- Then update all usage sites and audit the codebase. 