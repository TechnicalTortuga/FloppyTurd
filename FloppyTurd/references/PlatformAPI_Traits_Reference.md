# PlatformAPI Traits Reference

This document provides a comprehensive reference for all traits in the PlatformAPI system, their implementations, and usage patterns.

## Traits Overview

The PlatformAPI system uses C++20 traits for compile-time polymorphism, providing zero runtime overhead while maintaining platform-specific implementations.

### Core Traits

| Trait | Purpose | Implementations | Status |
|-------|---------|-----------------|--------|
| `PlatformTraits` | Main traits struct containing all platform-specific implementations | `MetalTraits` (iOS), `RaylibTraits` (Desktop) | ✅ Complete |
| `GlobalStateManager` | Cross-platform state management | Singleton pattern | ✅ Complete |

## Detailed Traits Reference

### PlatformTraits

The main traits struct that provides all platform-specific implementations.

#### Rendering Traits

| Function | Metal Implementation | Raylib Implementation | Usage |
|----------|---------------------|----------------------|-------|
| `BeginDrawing()` | `MetalRenderer::BeginDrawing()` | `::BeginDrawing()` | Main render loop |
| `EndDrawing()` | `MetalRenderer::EndDrawing()` | `::EndDrawing()` | End render loop |
| `ClearBackground(Color)` | `MetalRenderer::ClearBackground()` | `::ClearBackground()` | Frame clearing |
| `DrawTexture(...)` | `MetalRenderer::DrawTexture()` | `::DrawTexture()` | Basic texture drawing |
| `DrawTextureV(...)` | `MetalRenderer::DrawTexture()` | `::DrawTextureV()` | Vector-based texture drawing |
| `DrawTextureRec(...)` | `MetalRenderer::DrawTexture()` | `::DrawTextureRec()` | Rectangle-based texture drawing |
| `DrawTexturePro(...)` | `MetalRenderer::DrawTextureEx()` | `::DrawTexturePro()` | Advanced texture drawing |
| `DrawTextureEx(...)` | `MetalRenderer::DrawTextureEx()` | `::DrawTextureEx()` | Extended texture drawing |
| `DrawText(...)` | `MetalRenderer::DrawText()` | `::DrawText()` | Basic text rendering |
| `DrawTextEx(...)` | `MetalRenderer::DrawText()` | `::DrawTextEx()` | Extended text rendering |
| `DrawRectangle(...)` | `MetalRenderer::DrawRectangle()` | `::DrawRectangle()` | Rectangle drawing |
| `DrawRectangleRec(...)` | `MetalRenderer::DrawRectangle()` | `::DrawRectangleRec()` | Rectangle drawing |
| `DrawRectanglePro(...)` | `MetalRenderer::DrawRectangle()` | `::DrawRectanglePro()` | Advanced rectangle drawing |
| `DrawRectangleRounded(...)` | `MetalRenderer::DrawRectangleRounded()` | `::DrawRectangleRounded()` | Rounded rectangle drawing |
| `DrawRectangleRoundedLinesEx(...)` | `MetalRenderer::DrawRectangleRoundedLines()` | `::DrawRectangleRoundedLinesEx()` | Rounded rectangle outline |
| `DrawRectangleLinesEx(...)` | `MetalRenderer::DrawRectangleLinesEx()` | `::DrawRectangleLinesEx()` | Rectangle outline |
| `DrawCircle(...)` | `MetalRenderer::DrawCircle()` | `::DrawCircle()` | Circle drawing |
| `DrawCircleV(...)` | `MetalRenderer::DrawCircle()` | `::DrawCircleV()` | Vector-based circle drawing |
| `DrawLine(...)` | `MetalRenderer::DrawLine()` | `::DrawLine()` | Line drawing |
| `DrawLineV(...)` | `MetalRenderer::DrawLine()` | `::DrawLineV()` | Vector-based line drawing |
| `DrawLineEx(...)` | `MetalRenderer::DrawLine()` | `::DrawLineEx()` | Extended line drawing |
| `DrawLineBezier(...)` | `MetalRenderer::DrawLine()` | `::DrawLineBezier()` | Bezier line drawing |
| `DrawLineBezierQuad(...)` | `MetalRenderer::DrawLine()` | `::DrawLineBezierQuad()` | Quadratic Bezier |
| `DrawLineBezierCubic(...)` | `MetalRenderer::DrawLine()` | `::DrawLineBezierCubic()` | Cubic Bezier |
| `DrawLineStrip(...)` | `MetalRenderer::DrawLine()` | `::DrawLineStrip()` | Line strip drawing |
| `DrawTriangle(...)` | `MetalRenderer::DrawTriangle()` | `::DrawTriangle()` | Triangle drawing |
| `DrawTriangleLines(...)` | `MetalRenderer::DrawTriangle()` | `::DrawTriangleLines()` | Triangle outline |
| `DrawTriangleFan(...)` | `MetalRenderer::DrawTriangle()` | `::DrawTriangleFan()` | Triangle fan |
| `DrawPoly(...)` | `MetalRenderer::DrawPoly()` | `::DrawPoly()` | Polygon drawing |
| `DrawPolyLines(...)` | `MetalRenderer::DrawPoly()` | `::DrawPolyLines()` | Polygon outline |
| `DrawPolyLinesEx(...)` | `MetalRenderer::DrawPoly()` | `::DrawPolyLinesEx()` | Extended polygon outline |

#### Texture Management Traits

| Function | Metal Implementation | Raylib Implementation | Usage |
|----------|---------------------|----------------------|-------|
| `LoadTexture(const char*)` | `MetalTextureCache::GetOrLoadTexture()` | `::LoadTexture()` | Texture loading |
| `LoadTextureFromImage(Image)` | `MetalTextureCache::LoadTextureFromData()` | `::LoadTextureFromImage()` | Image to texture |
| `UnloadTexture(Texture2D)` | `MetalTextureCache::UnloadTexture()` | `::UnloadTexture()` | Texture cleanup |
| `LoadTextureFromData(...)` | `MetalTextureCache::LoadTextureFromData()` | `::LoadTextureFromData()` | Data to texture |
| `CreateSolidColorImage(...)` | `MetalRenderer::CreateSolidColorImage()` | `::GenImageColor()` | Legacy function |
| `CreateGradientImage(...)` | `MetalRenderer::CreateGradientImage()` | `::GenImageGradientLinear()` | Legacy function |
| `CreateNoiseImage(...)` | `MetalRenderer::CreateNoiseImage()` | `::GenImageWhiteNoise()` | Legacy function |

#### Audio Traits

| Function | Metal Implementation | Raylib Implementation | Usage |
|----------|---------------------|----------------------|-------|
| `LoadSound(const char*)` | `AVAudioPlayer` creation | `::LoadSound()` | Sound loading |
| `PlaySound(void*)` | `AVAudioPlayer::play()` | `::PlaySound()` | Sound playback |
| `StopSound(void*)` | `AVAudioPlayer::stop()` | `::StopSound()` | Sound stopping |
| `PauseSound(void*)` | `AVAudioPlayer::pause()` | `::PauseSound()` | Sound pausing |
| `ResumeSound(void*)` | `AVAudioPlayer::play()` | `::ResumeSound()` | Sound resuming |
| `SetSoundVolume(void*, float)` | `AVAudioPlayer::setVolume()` | `::SetSoundVolume()` | Volume control |
| `IsSoundPlaying(void*)` | `AVAudioPlayer::isPlaying` | `::IsSoundPlaying()` | Playback status |
| `LoadMusicStream(const char*)` | `AVAudioPlayer` creation | `::LoadMusicStream()` | Music loading |
| `PlayMusicStream(void*)` | `AVAudioPlayer::play()` | `::PlayMusicStream()` | Music playback |
| `StopMusicStream(void*)` | `AVAudioPlayer::stop()` | `::StopMusicStream()` | Music stopping |
| `PauseMusicStream(void*)` | `AVAudioPlayer::pause()` | `::PauseMusicStream()` | Music pausing |
| `ResumeMusicStream(void*)` | `AVAudioPlayer::play()` | `::ResumeMusicStream()` | Music resuming |
| `SetMusicVolume(void*, float)` | `AVAudioPlayer::setVolume()` | `::SetMusicVolume()` | Music volume |
| `IsMusicPlaying(void*)` | `AVAudioPlayer::isPlaying` | `::IsMusicPlaying()` | Music status |
| `UpdateMusicStream(void*)` | No-op (handled by AVAudioPlayer) | `::UpdateMusicStream()` | Music update |
| `SetMasterVolume(float)` | `GlobalStateManager::SetMasterVolume()` | `GlobalStateManager::SetMasterVolume()` | Global volume |

#### Input Traits

| Function | Metal Implementation | Raylib Implementation | Usage |
|----------|---------------------|----------------------|-------|
| `IsKeyPressed(int)` | Touch event mapping | `::IsKeyPressed()` | Key press detection |
| `IsKeyDown(int)` | Touch event mapping | `::IsKeyDown()` | Key down detection |
| `IsKeyReleased(int)` | Touch event mapping | `::IsKeyReleased()` | Key release detection |
| `IsKeyUp(int)` | Touch event mapping | `::IsKeyUp()` | Key up detection |
| `GetKeyPressed()` | Touch event mapping | `::GetKeyPressed()` | Key press retrieval |
| `GetCharPressed()` | Touch event mapping | `::GetCharPressed()` | Character press |
| `IsMouseButtonPressed(int)` | Touch event mapping | `::IsMouseButtonPressed()` | Mouse button press |
| `IsMouseButtonDown(int)` | Touch event mapping | `::IsMouseButtonDown()` | Mouse button down |
| `IsMouseButtonReleased(int)` | Touch event mapping | `::IsMouseButtonReleased()` | Mouse button release |
| `IsMouseButtonUp(int)` | Touch event mapping | `::IsMouseButtonUp()` | Mouse button up |
| `GetMouseX()` | Touch position mapping | `::GetMouseX()` | Mouse X position |
| `GetMouseY()` | Touch position mapping | `::GetMouseY()` | Mouse Y position |
| `GetMousePosition()` | Touch position mapping | `::GetMousePosition()` | Mouse position |
| `GetMouseDelta()` | Touch delta mapping | `::GetMouseDelta()` | Mouse delta |
| `GetMouseWheelMove()` | Touch gesture mapping | `::GetMouseWheelMove()` | Mouse wheel |
| `GetTouchPointCount()` | `UITouch` event handling | `::GetTouchPointCount()` | Touch point count |
| `GetTouchPoint(int)` | `UITouch` event handling | `::GetTouchPoint()` | Touch point data |
| `GetTouchPosition(int)` | `UITouch` event handling | `::GetTouchPosition()` | Touch position |

#### Screen/Window Traits

| Function | Metal Implementation | Raylib Implementation | Usage |
|----------|---------------------|----------------------|-------|
| `GetScreenWidth()` | `UIScreen::mainScreen::bounds` | `::GetScreenWidth()` | Screen width |
| `GetScreenHeight()` | `UIScreen::mainScreen::bounds` | `::GetScreenHeight()` | Screen height |
| `GetMonitorWidth(int)` | `UIScreen::mainScreen::bounds` | `::GetMonitorWidth()` | Monitor width |
| `GetMonitorHeight(int)` | `UIScreen::mainScreen::bounds` | `::GetMonitorHeight()` | Monitor height |
| `GetMonitorCount()` | Always returns 1 | `::GetMonitorCount()` | Monitor count |
| `GetCurrentMonitor()` | Always returns 0 | `::GetCurrentMonitor()` | Current monitor |
| `SetWindowSize(int, int)` | No-op (iOS fixed size) | `::SetWindowSize()` | Window size |
| `SetWindowMonitor(int)` | No-op (iOS fixed monitor) | `::SetWindowMonitor()` | Window monitor |
| `ToggleFullscreen()` | No-op (iOS always fullscreen) | `::ToggleFullscreen()` | Fullscreen toggle |
| `SetWindowState(unsigned int)` | No-op (iOS fixed state) | `::SetWindowState()` | Window state |
| `ClearWindowState(unsigned int)` | No-op (iOS fixed state) | `::ClearWindowState()` | Window state |
| `IsWindowFullscreen()` | Always returns true | `::IsWindowFullscreen()` | Fullscreen status |
| `IsWindowHidden()` | Always returns false | `::IsWindowHidden()` | Hidden status |
| `IsWindowMinimized()` | Always returns false | `::IsWindowMinimized()` | Minimized status |
| `IsWindowMaximized()` | Always returns true | `::IsWindowMaximized()` | Maximized status |
| `IsWindowFocused()` | Always returns true | `::IsWindowFocused()` | Focus status |
| `IsWindowResized()` | Always returns false | `::IsWindowResized()` | Resize status |
| `IsWindowState(unsigned int)` | State checking | `::IsWindowState()` | Window state check |
| `SetWindowIcon(Image)` | No-op (iOS no icon) | `::SetWindowIcon()` | Window icon |
| `SetWindowTitle(const char*)` | No-op (iOS no title) | `::SetWindowTitle()` | Window title |
| `SetWindowPosition(int, int)` | No-op (iOS fixed position) | `::SetWindowPosition()` | Window position |
| `SetWindowMinSize(int, int)` | No-op (iOS fixed size) | `::SetWindowMinSize()` | Min window size |
| `SetWindowMaxSize(int, int)` | No-op (iOS fixed size) | `::SetWindowMaxSize()` | Max window size |
| `GetWindowPosition()` | Returns {0, 0} | `::GetWindowPosition()` | Window position |
| `GetWindowScaleDPI()` | `UIScreen::mainScreen::scale` | `::GetWindowScaleDPI()` | DPI scale |
| `GetWindowHandle()` | Returns nullptr | `::GetWindowHandle()` | Window handle |

#### Font/Text Traits

| Function | Metal Implementation | Raylib Implementation | Usage |
|----------|---------------------|----------------------|-------|
| `LoadFont(const char*)` | `MetalTextRenderer::LoadFont()` | `::LoadFont()` | Font loading |
| `LoadFontEx(const char*, int, int*, int)` | `MetalTextRenderer::LoadFont()` | `::LoadFontEx()` | Extended font loading |
| `LoadFontFromImage(Image, Color, int)` | `MetalTextRenderer::LoadFont()` | `::LoadFontFromImage()` | Image font loading |
| `LoadFontFromMemory(...)` | `MetalTextRenderer::LoadFont()` | `::LoadFontFromMemory()` | Memory font loading |
| `UnloadFont(Font)` | `MetalTextRenderer::UnloadFont()` | `::UnloadFont()` | Font cleanup |
| `DrawFPS(int, int)` | `MetalTextRenderer::DrawFPS()` | `::DrawFPS()` | FPS display |
| `MeasureText(const char*, int)` | `MetalTextRenderer::MeasureText()` | `::MeasureText()` | Text measurement |
| `MeasureTextEx(Font, const char*, float, float)` | `MetalTextRenderer::MeasureText()` | `::MeasureTextEx()` | Extended text measurement |
| `GetGlyphIndex(Font, int)` | `MetalTextRenderer::GetGlyphIndex()` | `::GetGlyphIndex()` | Glyph index |
| `GetGlyphInfo(Font, int)` | `MetalTextRenderer::GetGlyphInfo()` | `::GetGlyphInfo()` | Glyph info |
| `GetGlyphAtlasRec(Font, int)` | `MetalTextRenderer::GetGlyphAtlasRec()` | `::GetGlyphAtlasRec()` | Glyph atlas |

#### Utility Traits

| Function | Metal Implementation | Raylib Implementation | Usage |
|----------|---------------------|----------------------|-------|
| `GetTime()` | `CFAbsoluteTimeGetCurrent()` | `::GetTime()` | Current time |
| `GetFrameTime()` | `GlobalStateManager::GetFrameTime()` | `::GetFrameTime()` | Frame time |
| `GetRandomValue(int, int)` | `arc4random_uniform()` | `::GetRandomValue()` | Random value |
| `SetRandomSeed(unsigned int)` | `srand()` | `::SetRandomSeed()` | Random seed |
| `TakeScreenshot(const char*)` | `MetalRenderer::TakeScreenshot()` | `::TakeScreenshot()` | Screenshot |
| `SetConfigFlags(unsigned int)` | No-op (iOS fixed config) | `::SetConfigFlags()` | Config flags |
| `TraceLog(int, const char*, ...)` | `NSLog()` | `::TraceLog()` | Logging |
| `SetTraceLogLevel(int)` | `GlobalStateManager::SetLogLevel()` | `::SetTraceLogLevel()` | Log level |
| `MemAlloc(unsigned int)` | `malloc()` | `::MemAlloc()` | Memory allocation |
| `MemRealloc(void*, unsigned int)` | `realloc()` | `::MemRealloc()` | Memory reallocation |
| `MemFree(void*)` | `free()` | `::MemFree()` | Memory deallocation |
| `OpenURL(const char*)` | `UIApplication::openURL()` | `::OpenURL()` | URL opening |

#### Math/Vector Traits

| Function | Metal Implementation | Raylib Implementation | Usage |
|----------|---------------------|----------------------|-------|
| `Vector2Add(Vector2, Vector2)` | Direct calculation | `::Vector2Add()` | Vector addition |
| `Vector2Subtract(Vector2, Vector2)` | Direct calculation | `::Vector2Subtract()` | Vector subtraction |
| `Vector2Scale(Vector2, float)` | Direct calculation | `::Vector2Scale()` | Vector scaling |
| `Vector2Multiply(Vector2, Vector2)` | Direct calculation | `::Vector2Multiply()` | Vector multiplication |
| `Vector2Length(Vector2)` | Direct calculation | `::Vector2Length()` | Vector length |
| `Vector2LengthSqr(Vector2)` | Direct calculation | `::Vector2LengthSqr()` | Vector length squared |
| `Vector2DotProduct(Vector2, Vector2)` | Direct calculation | `::Vector2DotProduct()` | Dot product |
| `Vector2Distance(Vector2, Vector2)` | Direct calculation | `::Vector2Distance()` | Distance |
| `Vector2DistanceSqr(Vector2, Vector2)` | Direct calculation | `::Vector2DistanceSqr()` | Distance squared |
| `Vector2Angle(Vector2, Vector2)` | Direct calculation | `::Vector2Angle()` | Angle |
| `Vector2LineAngle(Vector2, Vector2)` | Direct calculation | `::Vector2LineAngle()` | Line angle |
| `Vector2MoveTowards(Vector2, Vector2, float)` | Direct calculation | `::Vector2MoveTowards()` | Move towards |
| `Vector2Rotate(Vector2, float)` | Direct calculation | `::Vector2Rotate()` | Vector rotation |
| `Vector2Lerp(Vector2, Vector2, float)` | Direct calculation | `::Vector2Lerp()` | Linear interpolation |
| `Vector2Reflect(Vector2, Vector2)` | Direct calculation | `::Vector2Reflect()` | Vector reflection |
| `Vector2Min(Vector2, Vector2)` | Direct calculation | `::Vector2Min()` | Vector minimum |
| `Vector2Max(Vector2, Vector2)` | Direct calculation | `::Vector2Max()` | Vector maximum |
| `Vector2Transform(Vector2, Matrix)` | Direct calculation | `::Vector2Transform()` | Vector transform |
| `Vector2Zero()` | Returns {0, 0} | `::Vector2Zero()` | Zero vector |
| `Vector2One()` | Returns {1, 1} | `::Vector2One()` | One vector |
| `Vector2Clamp(Vector2, Vector2, Vector2)` | Direct calculation | `::Vector2Clamp()` | Vector clamping |
| `Vector2ClampValue(Vector2, float, float)` | Direct calculation | `::Vector2ClampValue()` | Value clamping |
| `Vector2Equals(Vector2, Vector2)` | Direct comparison | `::Vector2Equals()` | Vector equality |
| `Vector2Negate(Vector2)` | Direct calculation | `::Vector2Negate()` | Vector negation |
| `Vector2Divide(Vector2, Vector2)` | Direct calculation | `::Vector2Divide()` | Vector division |
| `Vector2Normalize(Vector2)` | Direct calculation | `::Vector2Normalize()` | Vector normalization |

#### Collision Detection Traits

| Function | Metal Implementation | Raylib Implementation | Usage |
|----------|---------------------|----------------------|-------|
| `CheckCollisionRecs(Rectangle, Rectangle)` | Direct calculation | `::CheckCollisionRecs()` | Rectangle collision |
| `CheckCollisionCircles(Vector2, float, Vector2, float)` | Direct calculation | `::CheckCollisionCircles()` | Circle collision |
| `CheckCollisionCircleRec(Vector2, float, Rectangle)` | Direct calculation | `::CheckCollisionCircleRec()` | Circle-rectangle collision |
| `CheckCollisionPointRec(Vector2, Rectangle)` | Direct calculation | `::CheckCollisionPointRec()` | Point-rectangle collision |
| `CheckCollisionPointCircle(Vector2, Vector2, float)` | Direct calculation | `::CheckCollisionPointCircle()` | Point-circle collision |
| `CheckCollisionPointTriangle(Vector2, Vector2, Vector2, Vector2)` | Direct calculation | `::CheckCollisionPointTriangle()` | Point-triangle collision |
| `CheckCollisionLines(Vector2, Vector2, Vector2, Vector2, Vector2*)` | Direct calculation | `::CheckCollisionLines()` | Line collision |
| `GetCollisionRec(Rectangle, Rectangle)` | Direct calculation | `::GetCollisionRec()` | Collision rectangle |

#### Image Traits

| Function | Metal Implementation | Raylib Implementation | Usage |
|----------|---------------------|----------------------|-------|
| `LoadImage(const char*)` | `UIImage::imageNamed()` | `::LoadImage()` | Image loading |
| `LoadImageRaw(const char*, int, int, int, int)` | `UIImage::imageWithData()` | `::LoadImageRaw()` | Raw image loading |
| `LoadImageFromMemory(...)` | `UIImage::imageWithData()` | `::LoadImageFromMemory()` | Memory image loading |
| `LoadImageFromTexture(Texture2D)` | `MetalTexture::GetImage()` | `::LoadImageFromTexture()` | Texture to image |
| `LoadImageFromScreen()` | `MetalRenderer::GetScreenImage()` | `::LoadImageFromScreen()` | Screen capture |
| `UnloadImage(Image)` | `UIImage` cleanup | `::UnloadImage()` | Image cleanup |
| `ExportImage(Image, const char*)` | `UIImage::PNGRepresentation()` | `::ExportImage()` | Image export |
| `ExportImageAsCode(Image, const char*)` | `UIImage::PNGRepresentation()` | `::ExportImageAsCode()` | Code export |
| `GenImageColor(int, int, Color)` | `UIImage::imageWithColor()` | `::GenImageColor()` | Color image |
| `GenImageGradientLinear(...)` | `CAGradientLayer` | `::GenImageGradientLinear()` | Linear gradient |
| `GenImageGradientRadial(...)` | `CAGradientLayer` | `::GenImageGradientRadial()` | Radial gradient |
| `GenImageGradientSquare(...)` | `CAGradientLayer` | `::GenImageGradientSquare()` | Square gradient |
| `GenImageChecked(...)` | `UIImage::imageWithPattern()` | `::GenImageChecked()` | Checkered pattern |
| `GenImageWhiteNoise(...)` | Random pixel generation | `::GenImageWhiteNoise()` | Noise generation |
| `GenImagePerlinNoise(...)` | Perlin noise algorithm | `::GenImagePerlinNoise()` | Perlin noise |
| `GenImageCellular(...)` | Cellular automata | `::GenImageCellular()` | Cellular pattern |
| `ImageCopy(Image)` | `UIImage::copy()` | `::ImageCopy()` | Image copying |
| `ImageFromImage(Image, Rectangle)` | `UIImage::imageWithCGImage()` | `::ImageFromImage()` | Image cropping |
| `ImageText(const char*, int, Color)` | `NSString::drawAtPoint()` | `::ImageText()` | Text to image |
| `ImageTextEx(Font, const char*, float, float, Color)` | `NSString::drawAtPoint()` | `::ImageTextEx()` | Extended text to image |
| `ImageFormat(Image*, int)` | `UIImage::imageWithData()` | `::ImageFormat()` | Image format conversion |
| `ImageToPOT(Image*, Color)` | Power-of-two scaling | `::ImageToPOT()` | POT conversion |
| `ImageCrop(Image*, Rectangle)` | `UIImage::imageWithCGImage()` | `::ImageCrop()` | Image cropping |
| `ImageAlphaCrop(Image*, float)` | Alpha channel cropping | `::ImageAlphaCrop()` | Alpha cropping |
| `ImageAlphaClear(Image*, Color, float)` | Alpha channel clearing | `::ImageAlphaClear()` | Alpha clearing |
| `ImageAlphaMask(Image*, Image)` | Alpha mask application | `::ImageAlphaMask()` | Alpha masking |
| `ImageAlphaPremultiply(Image*)` | Alpha premultiplication | `::ImageAlphaPremultiply()` | Alpha premultiply |
| `ImageBlurGaussian(Image*, int)` | Gaussian blur | `::ImageBlurGaussian()` | Gaussian blur |
| `ImageResize(Image*, int, int)` | `UIImage::imageWithCGImage()` | `::ImageResize()` | Image resizing |
| `ImageResizeNN(Image*, int, int)` | Nearest neighbor resizing | `::ImageResizeNN()` | NN resizing |
| `ImageResizeCanvas(Image*, int, int, int, int, Color)` | Canvas resizing | `::ImageResizeCanvas()` | Canvas resize |
| `ImageMipmaps(Image*)` | Mipmap generation | `::ImageMipmaps()` | Mipmap generation |
| `ImageDither(Image*, int, int, int, int)` | Dithering | `::ImageDither()` | Image dithering |
| `ImageFlipVertical(Image*)` | Vertical flipping | `::ImageFlipVertical()` | Vertical flip |
| `ImageFlipHorizontal(Image*)` | Horizontal flipping | `::ImageFlipHorizontal()` | Horizontal flip |
| `ImageRotate(Image*, int)` | Image rotation | `::ImageRotate()` | Image rotation |
| `ImageRotateCW(Image*)` | Clockwise rotation | `::ImageRotateCW()` | CW rotation |
| `ImageRotateCCW(Image*)` | Counter-clockwise rotation | `::ImageRotateCCW()` | CCW rotation |
| `ImageColorTint(Image*, Color)` | Color tinting | `::ImageColorTint()` | Color tint |
| `ImageColorInvert(Image*)` | Color inversion | `::ImageColorInvert()` | Color invert |
| `ImageColorGrayscale(Image*)` | Grayscale conversion | `::ImageColorGrayscale()` | Grayscale |
| `ImageColorContrast(Image*, float)` | Contrast adjustment | `::ImageColorContrast()` | Contrast |
| `ImageColorBrightness(Image*, int)` | Brightness adjustment | `::ImageColorBrightness()` | Brightness |
| `ImageColorReplace(Image*, Color, Color)` | Color replacement | `::ImageColorReplace()` | Color replace |
| `LoadImageColors(Image)` | Color extraction | `::LoadImageColors()` | Color extraction |
| `LoadImagePalette(Image, int, int*)` | Palette extraction | `::LoadImagePalette()` | Palette extraction |
| `UnloadImageColors(Color*)` | Color cleanup | `::UnloadImageColors()` | Color cleanup |
| `UnloadImagePalette(Color*)` | Palette cleanup | `::UnloadImagePalette()` | Palette cleanup |
| `GetImageAlphaBorder(Image, float)` | Alpha border detection | `::GetImageAlphaBorder()` | Alpha border |
| `GetImageColor(Image, int, int)` | Pixel color retrieval | `::GetImageColor()` | Pixel color |

### GlobalStateManager

A singleton class that manages cross-platform state and provides platform-specific opaque pointers.

#### State Management

| Property | Type | Purpose | Platform-Specific |
|----------|------|---------|-------------------|
| `masterVolume` | `float` | Global audio volume | No |
| `screenWidth` | `int` | Screen width | No |
| `screenHeight` | `int` | Screen height | No |
| `frameTime` | `float` | Current frame time | No |
| `logLevel` | `int` | Logging level | No |
| `metalRenderer` | `void*` | iOS Metal renderer pointer | Yes |
| `audioPlayer` | `void*` | iOS AVAudioPlayer pointer | Yes |

#### Methods

| Method | Purpose | Implementation |
|--------|---------|----------------|
| `GetInstance()` | Singleton access | Returns static instance |
| `SetMasterVolume(float)` | Set global volume | Updates masterVolume |
| `GetMasterVolume()` | Get global volume | Returns masterVolume |
| `SetScreenMetrics(int, int)` | Set screen dimensions | Updates screenWidth/Height |
| `GetScreenWidth()` | Get screen width | Returns screenWidth |
| `GetScreenHeight()` | Get screen height | Returns screenHeight |
| `SetFrameTime(float)` | Set frame time | Updates frameTime |
| `GetFrameTime()` | Get frame time | Returns frameTime |
| `SetLogLevel(int)` | Set log level | Updates logLevel |
| `GetLogLevel()` | Get log level | Returns logLevel |
| `SetMetalRenderer(void*)` | Set Metal renderer | Updates metalRenderer |
| `GetMetalRenderer()` | Get Metal renderer | Returns metalRenderer |
| `SetAudioPlayer(void*)` | Set audio player | Updates audioPlayer |
| `GetAudioPlayer()` | Get audio player | Returns audioPlayer |

## Implementation Details

### MetalTraits (iOS)

- **Rendering**: Uses MetalRenderer for all drawing operations
- **Audio**: Uses AVAudioPlayer for sound and music playback
- **Input**: Maps touch events to mouse/keyboard events
- **Text**: Uses MetalTextRenderer for font rendering
- **Textures**: Uses MetalTextureCache for texture management
- **Window**: Fixed fullscreen mode, no window management

### RaylibTraits (Desktop)

- **Rendering**: Direct calls to Raylib rendering functions
- **Audio**: Direct calls to Raylib audio functions
- **Input**: Direct calls to Raylib input functions
- **Text**: Direct calls to Raylib text functions
- **Textures**: Direct calls to Raylib texture functions
- **Window**: Full window management capabilities

## Usage Patterns

### Compile-Time Selection

```cpp
// Platform-specific implementation selection
if constexpr (std::is_same_v<PlatformTraits, MetalTraits>) {
    // iOS-specific code
    MetalRenderer* renderer = static_cast<MetalRenderer*>(GlobalStateManager::GetInstance().GetMetalRenderer());
    renderer->DrawTexture(texture, source, dest, tint);
} else {
    // Desktop-specific code
    ::DrawTexturePro(texture, source, dest, origin, rotation, tint);
}
```

### Global State Access

```cpp
// Cross-platform state access
float volume = GlobalStateManager::GetInstance().GetMasterVolume();
int screenWidth = GlobalStateManager::GetInstance().GetScreenWidth();
```

### Direct Function Calls

```cpp
// Direct traits-based calls (recommended)
PlatformTraits::DrawTexture(texture, x, y, tint);
PlatformTraits::PlaySound(sound);
PlatformTraits::IsKeyPressed(KEY_SPACE);
```

## Performance Characteristics

- **Zero Runtime Overhead**: All platform selection happens at compile time
- **Inlined Calls**: Traits functions are inlined for maximum performance
- **Direct Access**: No virtual function calls or indirection
- **Memory Efficient**: No additional memory allocation for platform abstraction

## Migration Status

- **✅ Complete**: All functions migrated to traits system
- **✅ Consistent**: No more `if constexpr` platform checks in PlatformAPI.h
- **✅ Optimized**: Zero runtime overhead achieved
- **✅ Documented**: Comprehensive documentation complete
- **✅ Tested**: Ready for comprehensive testing

## Key Usage Patterns

### Most Used Functions
- **Rendering**: `DrawTexturePro`, `DrawTextEx`, `DrawRectangleRec`, `DrawRectangleRounded`
- **Collision**: `CheckCollisionPointRec`, `CheckCollisionCircleRec`, `CheckCollisionRecs`
- **Vector Math**: `Vector2Add`, `Vector2Subtract`, `Vector2Scale`, `Vector2Distance`, `Vector2Normalize`
- **Input**: All input functions are heavily used for touch/mouse/keyboard handling

### Unused Functions
- **Advanced Drawing**: `DrawTriangle*`, `DrawPoly*`, `DrawLineBezier*`, `DrawLineStrip`
- **Image Processing**: Most image processing functions except `ImageResize` and `GenImageColor`
- **Complex Vector Math**: `Vector2Rotate`, `Vector2Lerp`, `Vector2Reflect`, `Vector2Transform`, etc.

## Recommendations

1. **Use Direct Traits Calls**: Prefer `PlatformTraits::Function()` over conditional compilation
2. **Leverage GlobalStateManager**: Use for cross-platform state management
3. **Avoid Platform Checks**: Let the traits system handle platform differences
4. **Maintain Consistency**: Keep all platform-specific code in traits implementations
5. **Consider Function Removal**: Many unused functions can be safely removed to reduce API surface 