#include "PlatformAPI.h"
#include "Game.h"

// ============================================================================
// GLOBAL FUNCTION IMPLEMENTATIONS USING CURRENTPLATFORMAPI
// ============================================================================

// Window and Screen Functions
void InitWindow(int width, int height, const char* title) {
    CurrentPlatformAPI::GetInstance().InitWindow(width, height, title);
}

void SetWindowSize(int width, int height) {
    CurrentPlatformAPI::GetInstance().SetWindowSize(width, height);
}

void ToggleFullscreen() {
    CurrentPlatformAPI::GetInstance().ToggleFullscreen();
}

void CloseWindow() {
    CurrentPlatformAPI::GetInstance().CloseWindow();
}

int GetScreenWidth() {
    return CurrentPlatformAPI::GetInstance().GetScreenWidth();
}

int GetScreenHeight() {
    return CurrentPlatformAPI::GetInstance().GetScreenHeight();
}

float GetScreenScale() {
    return CurrentPlatformAPI::GetInstance().GetScreenScale();
}

bool WindowShouldClose() {
    return CurrentPlatformAPI::GetInstance().WindowShouldClose();
}

// Rendering Functions
void BeginDrawing() {
    CurrentPlatformAPI::GetInstance().BeginDrawing();
}

void EndDrawing() {
    CurrentPlatformAPI::GetInstance().EndDrawing();
}

void ClearBackground(Color color) {
    CurrentPlatformAPI::GetInstance().ClearBackground(color);
}

void DrawRectangle(float x, float y, float width, float height, Color color) {
    CurrentPlatformAPI::GetInstance().DrawRectangle(x, y, width, height, color);
}

void DrawRectangleRec(Rectangle rec, Color color) {
    CurrentPlatformAPI::GetInstance().DrawRectangleRec(rec, color);
}

void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) {
    CurrentPlatformAPI::GetInstance().DrawRectangleLinesEx(rec, lineThick, color);
}

void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
    CurrentPlatformAPI::GetInstance().DrawRectangleRounded(rec, roundness, segments, color);
}

void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    CurrentPlatformAPI::GetInstance().DrawRectangleRoundedLines(rec, roundness, segments, lineThick, color);
}

void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    CurrentPlatformAPI::GetInstance().DrawRectangleRoundedLinesEx(rec, roundness, segments, lineThick, color);
}

void DrawCircle(float centerX, float centerY, float radius, Color color) {
    CurrentPlatformAPI::GetInstance().DrawCircle(centerX, centerY, radius, color);
}

void DrawCircleV(Vector2 center, float radius, Color color) {
    CurrentPlatformAPI::GetInstance().DrawCircleV(center, radius, color);
}

void DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color) {
    CurrentPlatformAPI::GetInstance().DrawLine(startPosX, startPosY, endPosX, endPosY, color);
}

void DrawLineV(Vector2 startPos, Vector2 endPos, Color color) {
    CurrentPlatformAPI::GetInstance().DrawLineV(startPos, endPos, color);
}

void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {
    CurrentPlatformAPI::GetInstance().DrawLineEx(startPos, endPos, thick, color);
}

void DrawTexture(Texture2D texture, float x, float y, Color tint) {
    CurrentPlatformAPI::GetInstance().DrawTexture(texture, x, y, tint);
}

void DrawTextureV(Texture2D texture, Vector2 position, Color tint) {
    CurrentPlatformAPI::GetInstance().DrawTextureV(texture, position, tint);
}

void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) {
    CurrentPlatformAPI::GetInstance().DrawTextureRec(texture, source, position, tint);
}

void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
    CurrentPlatformAPI::GetInstance().DrawTexturePro(texture, source, dest, origin, rotation, tint);
}

void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) {
    CurrentPlatformAPI::GetInstance().DrawTextureEx(texture, position, rotation, scale, tint);
}

void DrawText(const char* text, float x, float y, float fontSize, Color color) {
    CurrentPlatformAPI::GetInstance().DrawText(text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(fontSize), color);
}

void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
    CurrentPlatformAPI::GetInstance().DrawTextEx(font, text, position, fontSize, spacing, tint);
}

int MeasureText(const char* text, int fontSize) {
    return CurrentPlatformAPI::GetInstance().MeasureText(text, fontSize);
}

// Variadic version for compatibility with raylib-style usage
const char* TextFormat(const char* text, ...) {
    va_list args;
    va_start(args, text);
    const char* result = CurrentPlatformAPI::GetInstance().TextFormat(text, args);
    va_end(args);
    return result;
}

// Input Functions
bool IsKeyPressed(int key) {
    return CurrentPlatformAPI::GetInstance().IsKeyPressed(key);
}

bool IsKeyDown(int key) {
    return CurrentPlatformAPI::GetInstance().IsKeyDown(key);
}

bool IsKeyReleased(int key) {
    return CurrentPlatformAPI::GetInstance().IsKeyReleased(key);
}

bool IsMouseButtonDown(int button) {
    return CurrentPlatformAPI::GetInstance().IsMouseButtonDown(button);
}

bool IsMouseButtonPressed(int button) {
    return CurrentPlatformAPI::GetInstance().IsMouseButtonPressed(button);
}

bool IsMouseButtonReleased(int button) {
    return CurrentPlatformAPI::GetInstance().IsMouseButtonReleased(button);
}

Vector2 GetMousePosition() {
    return CurrentPlatformAPI::GetInstance().GetMousePosition();
}

Vector2 GetMouseDelta() {
    return CurrentPlatformAPI::GetInstance().GetMouseDelta();
}

Vector2 GetTouchPosition(int index) {
    return CurrentPlatformAPI::GetInstance().GetTouchPosition(index);
}

bool IsPrimaryInputPressed() {
    return CurrentPlatformAPI::GetInstance().IsPrimaryInputPressed();
}

bool IsPrimaryInputReleased() {
    return CurrentPlatformAPI::GetInstance().IsPrimaryInputReleased();
}

bool IsPrimaryInputDown() {
    return CurrentPlatformAPI::GetInstance().IsPrimaryInputDown();
}

// Texture Functions
Texture2D LoadTexture(const char* fileName) {
    return CurrentPlatformAPI::GetInstance().LoadTexture(fileName);
}

void UnloadTexture(Texture2D texture) {
    CurrentPlatformAPI::GetInstance().UnloadTexture(texture);
}

Image LoadImage(const char* fileName) {
    return CurrentPlatformAPI::GetInstance().LoadImage(fileName);
}

void UnloadImage(Image image) {
    CurrentPlatformAPI::GetInstance().UnloadImage(image);
}

void SetTextureWrap(Texture2D texture, int wrap) {
    CurrentPlatformAPI::GetInstance().SetTextureWrap(texture, wrap);
}

Texture2D LoadTextureFromImage(Image image) {
    return CurrentPlatformAPI::GetInstance().LoadTextureFromImage(image);
}

Image LoadImageFromTexture(Texture2D texture) {
    return CurrentPlatformAPI::GetInstance().LoadImageFromTexture(texture);
}

Rectangle GetTextureRec(Texture2D texture) {
    return CurrentPlatformAPI::GetInstance().GetTextureRec(texture);
}

// Font Functions
Font LoadFont(const char* fileName) {
    return CurrentPlatformAPI::GetInstance().LoadFont(fileName);
}

Font LoadFontEx(const char* fileName, int fontSize, int* fontChars, int glyphCount) {
    return CurrentPlatformAPI::GetInstance().LoadFontEx(fileName, fontSize, fontChars, glyphCount);
}

void UnloadFont(Font font) {
    CurrentPlatformAPI::GetInstance().UnloadFont(font);
}

Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
    return CurrentPlatformAPI::GetInstance().MeasureTextEx(font, text, fontSize, spacing);
}

// Rendering Functions
void BeginScissorMode(int x, int y, int width, int height) {
    CurrentPlatformAPI::GetInstance().BeginScissorMode(x, y, width, height);
}

void EndScissorMode() {
    CurrentPlatformAPI::GetInstance().EndScissorMode();
}

void DrawFPS(int posX, int posY) {
    CurrentPlatformAPI::GetInstance().DrawFPS(posX, posY);
}

// Audio Functions
void InitAudioDevice() {
    CurrentPlatformAPI::GetInstance().InitAudioDevice();
}

void InitializeAudio() {
    CurrentPlatformAPI::GetInstance().InitializeAudio();
}

void CloseAudioDevice() {
    CurrentPlatformAPI::GetInstance().CloseAudioDevice();
}

void ShutdownAudio() {
    CurrentPlatformAPI::GetInstance().ShutdownAudio();
}

bool IsAudioDeviceReady() {
    return CurrentPlatformAPI::GetInstance().IsAudioDeviceReady();
}

Sound LoadSound(const char* fileName) {
    return CurrentPlatformAPI::GetInstance().LoadSound(fileName);
}

void UnloadSound(Sound sound) {
    CurrentPlatformAPI::GetInstance().UnloadSound(sound);
}

void PlaySound(Sound sound) {
    CurrentPlatformAPI::GetInstance().PlaySound(sound);
}

void StopSound(Sound sound) {
    CurrentPlatformAPI::GetInstance().StopSound(sound);
}

void PauseSound(Sound sound) {
    CurrentPlatformAPI::GetInstance().PauseSound(sound);
}

void ResumeSound(Sound sound) {
    CurrentPlatformAPI::GetInstance().ResumeSound(sound);
}

void SetSoundVolume(Sound sound, float volume) {
    CurrentPlatformAPI::GetInstance().SetSoundVolume(sound, volume);
}

bool IsSoundPlaying(Sound sound) {
    return CurrentPlatformAPI::GetInstance().IsSoundPlaying(sound);
}

Music LoadMusicStream(const char* fileName) {
    return CurrentPlatformAPI::GetInstance().LoadMusicStream(fileName);
}

Music LoadMusic(const char* fileName) {
    return CurrentPlatformAPI::GetInstance().LoadMusic(fileName);
}

void UnloadMusicStream(Music music) {
    CurrentPlatformAPI::GetInstance().UnloadMusicStream(music);
}

void UnloadMusic(Music music) {
    CurrentPlatformAPI::GetInstance().UnloadMusic(music);
}

void PlayMusicStream(Music music) {
    CurrentPlatformAPI::GetInstance().PlayMusicStream(music);
}

void PlayMusic(Music music) {
    CurrentPlatformAPI::GetInstance().PlayMusic(music);
}

void StopMusicStream(Music music) {
    CurrentPlatformAPI::GetInstance().StopMusicStream(music);
}

void StopMusic(Music music) {
    CurrentPlatformAPI::GetInstance().StopMusic(music);
}

void PauseMusicStream(Music music) {
    CurrentPlatformAPI::GetInstance().PauseMusicStream(music);
}

void PauseMusic(Music music) {
    CurrentPlatformAPI::GetInstance().PauseMusic(music);
}

void ResumeMusicStream(Music music) {
    CurrentPlatformAPI::GetInstance().ResumeMusicStream(music);
}

void ResumeMusic(Music music) {
    CurrentPlatformAPI::GetInstance().ResumeMusic(music);
}

void UpdateMusicStream(Music music) {
    CurrentPlatformAPI::GetInstance().UpdateMusicStream(music);
}

void UpdateMusic(Music music) {
    CurrentPlatformAPI::GetInstance().UpdateMusic(music);
}

void SetMusicVolume(Music music, float volume) {
    CurrentPlatformAPI::GetInstance().SetMusicVolume(music, volume);
}

bool IsMusicStreamPlaying(Music music) {
    return CurrentPlatformAPI::GetInstance().IsMusicStreamPlaying(music);
}

bool IsMusicPlaying(Music music) {
    return CurrentPlatformAPI::GetInstance().IsMusicPlaying(music);
}

void SetMusicLooping(Music music, bool looping) {
    CurrentPlatformAPI::GetInstance().SetMusicLooping(music, looping);
}

float GetMusicDuration(Music music) {
    return CurrentPlatformAPI::GetInstance().GetMusicDuration(music);
}

// Utility Functions
double GetTime() {
    return CurrentPlatformAPI::GetInstance().GetTime();
}

void TraceLog(int logLevel, const char* text, ...) {
    va_list args;
    va_start(args, text);
    CurrentPlatformAPI::GetInstance().TraceLog(logLevel, text, args);
    va_end(args);
}



int GetRandomValue(int min, int max) {
    return CurrentPlatformAPI::GetInstance().GetRandomValue(min, max);
}

float GetRandomFloat(float min, float max) {
    return CurrentPlatformAPI::GetInstance().GetRandomFloat(min, max);
}

// Additional Utility Functions
void SetTraceLogLevel(int logLevel) {
    CurrentPlatformAPI::GetInstance().SetTraceLogLevel(logLevel);
}

void SetConfigFlags(unsigned int flags) {
    CurrentPlatformAPI::GetInstance().SetConfigFlags(flags);
}

void SetRandomSeed(unsigned int seed) {
    CurrentPlatformAPI::GetInstance().SetRandomSeed(seed);
}

Vector2 GetRandomVector2(Vector2 min, Vector2 max) {
    return CurrentPlatformAPI::GetInstance().GetRandomVector2(min, max);
}

Color GetRandomColor() {
    return CurrentPlatformAPI::GetInstance().GetRandomColor();
}

// Vector Math Functions
float Vector2Length(Vector2 v) {
    return CurrentPlatformAPI::GetInstance().Vector2Length(v);
}

Vector2 Vector2Normalize(Vector2 v) {
    return CurrentPlatformAPI::GetInstance().Vector2Normalize(v);
}

Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
    return CurrentPlatformAPI::GetInstance().Vector2Add(v1, v2);
}

Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
    return CurrentPlatformAPI::GetInstance().Vector2Subtract(v1, v2);
}

Vector2 Vector2Scale(Vector2 v, float scale) {
    return CurrentPlatformAPI::GetInstance().Vector2Scale(v, scale);
}

float Vector2Distance(Vector2 v1, Vector2 v2) {
    return CurrentPlatformAPI::GetInstance().Vector2Distance(v1, v2);
}

// Collision Detection Functions
bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
    return CurrentPlatformAPI::GetInstance().CheckCollisionRecs(rec1, rec2);
}

bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) {
    return CurrentPlatformAPI::GetInstance().CheckCollisionCircleRec(center, radius, rec);
}

bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
    return CurrentPlatformAPI::GetInstance().CheckCollisionPointRec(point, rec);
}

// Color Functions
Color ColorAlpha(Color color, float alpha) {
    return CurrentPlatformAPI::GetInstance().ColorAlpha(color, alpha);
}

Color ColorLerp(Color color1, Color color2, float amount) {
    return CurrentPlatformAPI::GetInstance().ColorLerp(color1, color2, amount);
}

Color Fade(Color color, float alpha) {
    return CurrentPlatformAPI::GetInstance().Fade(color, alpha);
}

// Math Utility Functions - These are already defined as inline functions in PlatformTraits.h
// No need to redefine them here as they're available directly

// Sqrt is already defined as inline function in PlatformTraits.h
// float Sqrt(float value) {
//     return CurrentPlatformAPI::GetInstance().Sqrt(value);
// }

float Pow(float base, float exponent) {
    return CurrentPlatformAPI::GetInstance().Pow(base, exponent);
}

// Time and Performance Functions
float GetFrameTime() {
    return CurrentPlatformAPI::GetInstance().GetFrameTime();
}

int GetCurrentFPS() {
    return CurrentPlatformAPI::GetInstance().GetCurrentFPS();
}

float GetCurrentFrameTime() {
    return CurrentPlatformAPI::GetInstance().GetCurrentFrameTime();
}

// Rectangle Utility Functions
Rectangle RectangleNew(float x, float y, float width, float height) {
    return CurrentPlatformAPI::GetInstance().RectangleNew(x, y, width, height);
}

Rectangle RectangleFromVector2(Vector2 position, Vector2 size) {
    return CurrentPlatformAPI::GetInstance().RectangleFromVector2(position, size);
}

// Platform-Specific Utility Functions
std::string GetResourcePath(const char* resourceName) {
    return CurrentPlatformAPI::GetInstance().GetResourcePath(resourceName);
}

std::string GetSaveDataPath(const char* filename) {
    return CurrentPlatformAPI::GetInstance().GetSaveDataPath(filename);
}

bool PreferLowPowerMode() {
    return CurrentPlatformAPI::GetInstance().PreferLowPowerMode();
}

int GetRecommendedTextureSize() {
    return CurrentPlatformAPI::GetInstance().GetRecommendedTextureSize();
}

bool IsMobilePlatform() {
    return CurrentPlatformAPI::GetInstance().IsMobilePlatform();
}

// Initialization and Lifecycle Functions
void Initialize(void* nativeView) {
    CurrentPlatformAPI::GetInstance().Initialize(nativeView);
}

void Initialize() {
    CurrentPlatformAPI::GetInstance().Initialize();
}

void Shutdown() {
    CurrentPlatformAPI::GetInstance().Shutdown();
}

// Complex Audio Functions
void StartCrossfade(float duration) {
    CurrentPlatformAPI::StartCrossfade(duration);
}

void UpdateCrossfade(float deltaTime) {
    CurrentPlatformAPI::UpdateCrossfade(deltaTime);
}

void FadeOutMusic(float duration) {
    CurrentPlatformAPI::FadeOutMusic(duration);
}

void FadeInMusic(float duration) {
    CurrentPlatformAPI::FadeInMusic(duration);
}

void UpdateFade(float deltaTime) {
    CurrentPlatformAPI::UpdateFade(deltaTime);
}

// Complex Image Functions
void ImageResize(Image* image, int newWidth, int newHeight) {
    CurrentTraits::ImageResize(image, newWidth, newHeight);
}

void ImageDraw(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint) {
    CurrentTraits::ImageDraw(dst, src, srcRec, dstRec, tint);
}

Image ImageCopy(Image image) {
    return CurrentTraits::ImageCopy(image);
}

Image ImageFromImage(Image image, Rectangle rec) {
    return CurrentTraits::ImageFromImage(image, rec);
}

void ImageFlipVertical(Image* image) {
    CurrentTraits::ImageFlipVertical(image);
}

void ImageFlipHorizontal(Image* image) {
    CurrentTraits::ImageFlipHorizontal(image);
}

// Add these global function implementations:
void SetTextureFilter(Texture2D texture, int filter) {
    CurrentTraits::SetTextureFilter(texture, filter);
}

Image GenImageColor(int width, int height, Color color) {
    return CurrentTraits::GenImageColor(width, height, color);
}

Font GetFontDefault() {
    return CurrentTraits::GetFontDefault();
}

// Platform-Specific Functions
void SetOrientation(bool landscape) {
    CurrentPlatformAPI::SetOrientation(landscape);
}

void ShowVirtualKeyboard(bool show) {
    CurrentPlatformAPI::ShowVirtualKeyboard(show);
}

void Vibrate(int milliseconds) {
    CurrentPlatformAPI::Vibrate(milliseconds);
}

void UpdateSafeAreaInsets(float top, float right, float bottom, float left) {
    CurrentTraits::UpdateSafeAreaInsets(top, right, bottom, left);
}

// Render Texture Functions
RenderTexture2D LoadRenderTexture(int width, int height) {
    return CurrentPlatformAPI::GetInstance().LoadRenderTexture(width, height);
}

void UnloadRenderTexture(RenderTexture2D target) {
    CurrentPlatformAPI::GetInstance().UnloadRenderTexture(target);
}

void BeginTextureMode(RenderTexture2D target) {
    CurrentPlatformAPI::GetInstance().BeginTextureMode(target);
}

void EndTextureMode() {
    CurrentPlatformAPI::GetInstance().EndTextureMode();
}

// Additional functions that might be needed
void SetTargetFPS(int fps) {
    CurrentPlatformAPI::GetInstance().SetTargetFPS(fps);
}

void SetScreenSize(int width, int height) {
    CurrentPlatformAPI::GetInstance().SetScreenSize(width, height);
}

void SetScreenScale(float scale) {
    CurrentPlatformAPI::GetInstance().SetScreenScale(scale);
}

Vector2 GetScreenCenter() {
    return Vector2{static_cast<float>(GetScreenWidth()) / 2.0f, 
                   static_cast<float>(GetScreenHeight()) / 2.0f};
}

Vector2 GetRenderScale() {
    return CurrentTraits::GetRenderScale();
}

Rectangle GetSafeArea() {
    return CurrentTraits::GetSafeArea();
}

float GetScreenDensity() {
    return CurrentTraits::GetScreenDensity();
}

bool IsLandscape() {
    return CurrentTraits::IsLandscape();
}

bool IsPortrait() {
    return CurrentTraits::IsPortrait();
}

void SetPreferredOrientation(bool landscape) {
    CurrentTraits::SetPreferredOrientation(landscape);
}

bool ShouldUseLargerTouchTargets() {
    return CurrentTraits::ShouldUseLargerTouchTargets();
}

int GetRecommendedFontSize() {
    return CurrentTraits::GetRecommendedFontSize();
}

// Additional functions that might be missing
bool IsWindowFullscreen() {
    // This would need to be implemented in the traits
    return false;
}

void SetWindowFocused() {
    // This would need to be implemented in the traits
}

void SetExitKey(int key) {
    // This would need to be implemented in the traits
}

int GetCurrentMonitor() {
    // This would need to be implemented in the traits
    return 0;
}

int GetMonitorWidth(int monitor) {
    // This would need to be implemented in the traits
    return 1920;
}

int GetMonitorHeight(int monitor) {
    // This would need to be implemented in the traits
    return 1080;
}

void SetWindowPosition(int x, int y) {
    // This would need to be implemented in the traits
}

void* CreateTextureFromImage(void* image, int* width, int* height) {
    return CurrentTraits::CreateTextureFromImage(image, width, height);
} 

// ============================================================================
// GLOBAL GAME INSTANCE MANAGEMENT (for iOS integration)
// ============================================================================

// Global game instance pointer
static Game* g_gameInstance = nullptr;

extern "C" Game* GetGameInstance() {
    return g_gameInstance;
}

extern "C" void SetGameInstance(Game* game) {
    g_gameInstance = game;
}

extern "C" void OnAppPause() {
    if (g_gameInstance) {
        // Pause the game when app goes to background
        // This could call a pause method on the game instance
    }
}

extern "C" void OnAppResume() {
    if (g_gameInstance) {
        // Resume the game when app comes to foreground
        // This could call a resume method on the game instance
    }
}

// Global game view management for iOS
static void* g_globalGameView = nullptr;

extern "C" void SetGlobalGameView(void* gameView) {
    g_globalGameView = gameView;
    TraceLog(LOG_INFO, "[PLATFORM] Global game view set: %p", gameView);
}

extern "C" void* GetGlobalGameView() {
    return g_globalGameView;
}

extern "C" int game_main(int argc, char* argv[]) {
    // Suppress unused parameter warnings
    (void)argc;
    (void)argv;
    
    TraceLog(LOG_INFO, "[GAME] game_main() STARTING");
    
    // Initialize platform layer
    TraceLog(LOG_INFO, "[GAME] About to initialize platform layer");
    CurrentPlatformAPI::GetInstance().Initialize();
    TraceLog(LOG_INFO, "[GAME] Platform layer initialized successfully");
    
    // Create the game instance
    TraceLog(LOG_INFO, "[GAME] About to create Game instance");
    Game* game = new Game();
    if (!game) {
        TraceLog(LOG_ERROR, "[GAME] Failed to create Game instance");
        return -1;
    }
    TraceLog(LOG_INFO, "[GAME] Game instance created successfully: %p", game);
    
    // Set the game instance globally for iOS integration
    TraceLog(LOG_INFO, "[GAME] About to set game instance globally");
    SetGameInstance(game);
    TraceLog(LOG_INFO, "[GAME] Game instance set globally: %p", game);
    
    // Initialize the game
    TraceLog(LOG_INFO, "[GAME] About to call game->Initialize()");
    if (!game->Initialize()) {
        TraceLog(LOG_ERROR, "[GAME] Failed to initialize Game instance");
        delete game;
        SetGameInstance(nullptr);
        return -2;
    }
    TraceLog(LOG_INFO, "[GAME] Game->Initialize() completed successfully");
    
    TraceLog(LOG_INFO, "[GAME] Game initialization successful");
    TraceLog(LOG_INFO, "[GAME] game_main() returning 0");
    
    int result = 0;
    TraceLog(LOG_INFO, "[GAME] game_main() final result: %d", result);
    return result;
} 

// RaylibTraits implementations
// ColorFade is no longer a global function, so this implementation is removed.
// The fade functionality is now handled by IOSTraits::Fade.