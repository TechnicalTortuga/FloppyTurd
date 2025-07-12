#include "PlatformAPI.h"
#include "PlatformSpecific.h"

// Platform-specific implementations
#ifdef PLATFORM_IOS
#include "PlatformIOS.h"
#else
#include "PlatformRaylib.h"
#endif

// Singleton instance
PlatformAPI* PlatformAPI::s_instance = nullptr;

// ============================================================================
// SINGLETON IMPLEMENTATION
// ============================================================================

PlatformAPI& PlatformAPI::GetInstance() {
    if (!s_instance) {
        s_instance = new PlatformAPI();
    }
    return *s_instance;
}

PlatformAPI::PlatformAPI() : m_platformImpl(nullptr) {
    // Create platform-specific implementation
#ifdef PLATFORM_IOS
    m_platformImpl = new PlatformIOS();
#else
    m_platformImpl = new PlatformRaylib();
#endif
}

PlatformAPI::~PlatformAPI() {
    if (m_platformImpl) {
        delete m_platformImpl;
        m_platformImpl = nullptr;
    }
}

PlatformSpecific* PlatformAPI::GetPlatformImpl() {
    return GetInstance().m_platformImpl;
}

// ============================================================================
// INITIALIZATION AND LIFECYCLE
// ============================================================================

void PlatformAPI::Initialize(void* nativeView) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->Initialize(nativeView);
    }
}

void PlatformAPI::Initialize() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->Initialize();
    }
}

void PlatformAPI::Shutdown() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->Shutdown();
    }
}

// ============================================================================
// RENDERING FUNCTIONS (Raylib-compatible)
// ============================================================================

void PlatformAPI::DrawRectangle(int x, int y, int width, int height, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawRectangle(x, y, width, height, color);
    }
}

void PlatformAPI::DrawRectangleRec(Rectangle rec, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawRectangleRec(rec, color);
    }
}

void PlatformAPI::DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawRectangleRounded(rec, roundness, segments, color);
    }
}

void PlatformAPI::DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawRectangleRoundedLines(rec, roundness, segments, lineThick, color);
    }
}

void PlatformAPI::DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawRectangleRoundedLinesEx(rec, roundness, segments, lineThick, color);
    }
}

void PlatformAPI::DrawCircle(float centerX, float centerY, float radius, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawCircle(centerX, centerY, radius, color);
    }
}

void PlatformAPI::DrawCircleV(Vector2 center, float radius, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawCircleV(center, radius, color);
    }
}

void PlatformAPI::DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawLine(startPosX, startPosY, endPosX, endPosY, color);
    }
}

void PlatformAPI::DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawLineEx(startPos, endPos, thick, color);
    }
}

void PlatformAPI::DrawLineV(Vector2 startPos, Vector2 endPos, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawLineV(startPos, endPos, color);
    }
}

void PlatformAPI::DrawTexture(Texture2D texture, int posX, int posY, Color tint) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawTexture(texture, posX, posY, tint);
    }
}

void PlatformAPI::DrawTextureV(Texture2D texture, Vector2 position, Color tint) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawTextureV(texture, position, tint);
    }
}

void PlatformAPI::DrawTextureRec(Texture2D texture, Rectangle source, Rectangle dest, Color tint) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawTextureRec(texture, source, dest, tint);
    }
}

void PlatformAPI::DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawTexturePro(texture, source, dest, origin, rotation, tint);
    }
}

void PlatformAPI::DrawText(const char* text, int posX, int posY, int fontSize, Color color) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawText(text, posX, posY, fontSize, color);
    }
}

void PlatformAPI::DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawTextEx(font, text, position, fontSize, spacing, tint);
    }
}

void PlatformAPI::BeginDrawing(void* renderTexture) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->BeginDrawing(renderTexture);
    }
}

void PlatformAPI::EndDrawing(void* renderTexture) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->EndDrawing(renderTexture);
    }
}

void* PlatformAPI::LoadRenderTexture(int width, int height) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->LoadRenderTexture(width, height);
    }
    return nullptr;
}

void PlatformAPI::UnloadRenderTexture(void* renderTexture) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->UnloadRenderTexture(renderTexture);
    }
}

// ============================================================================
// TEXTURE AND IMAGE FUNCTIONS
// ============================================================================

Texture2D PlatformAPI::LoadTexture(const char* fileName) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->LoadTexture(fileName);
    }
    return { 0 };
}

void PlatformAPI::UnloadTexture(Texture2D texture) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->UnloadTexture(texture);
    }
}

void* PlatformAPI::LoadTextureFromImage(void* imageData, int width, int height, int format) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->LoadTextureFromImage(imageData, width, height, format);
    }
    return nullptr;
}

void* PlatformAPI::CreateTextureFromImage(void* image, int* width, int* height) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->CreateTextureFromImage(image, width, height);
    }
    return nullptr;
}

Image PlatformAPI::LoadImage(const char* fileName) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->LoadImage(fileName);
    }
    return { 0 };
}

void PlatformAPI::UnloadImage(Image image) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->UnloadImage(image);
    }
}

void* PlatformAPI::CreateSolidColorImage(int width, int height, Color color) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->CreateSolidColorImage(width, height, color);
    }
    return nullptr;
}

// ============================================================================
// AUDIO FUNCTIONS
// ============================================================================

void PlatformAPI::InitializeAudio() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->InitializeAudio();
    }
}

void PlatformAPI::ShutdownAudio() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->ShutdownAudio();
    }
}

void* PlatformAPI::LoadSound(const char* fileName) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->LoadSound(fileName);
    }
    return nullptr;
}

void PlatformAPI::UnloadSound(void* sound) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->UnloadSound(sound);
    }
}

void PlatformAPI::PlaySound(void* sound) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->PlaySound(sound);
    }
}

void PlatformAPI::SetSoundVolume(void* sound, float volume) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->SetSoundVolume(sound, volume);
    }
}

void* PlatformAPI::LoadMusic(const char* fileName) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->LoadMusic(fileName);
    }
    return nullptr;
}

void PlatformAPI::UnloadMusic(void* music) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->UnloadMusic(music);
    }
}

void PlatformAPI::PlayMusic(void* music) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->PlayMusic(music);
    }
}

void PlatformAPI::StopMusic(void* music) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->StopMusic(music);
    }
}

void PlatformAPI::UpdateMusic(void* music) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->UpdateMusic(music);
    }
}

bool PlatformAPI::IsMusicPlaying(void* music) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsMusicPlaying(music);
    }
    return false;
}

void PlatformAPI::SetMusicVolume(void* music, float volume) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->SetMusicVolume(music, volume);
    }
}

void PlatformAPI::PauseMusic(void* music) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->PauseMusic(music);
    }
}

void PlatformAPI::ResumeMusic(void* music) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->ResumeMusic(music);
    }
}

void PlatformAPI::SetMusicLooping(void* music, bool looping) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->SetMusicLooping(music, looping);
    }
}

// ============================================================================
// ADVANCED AUDIO FUNCTIONS
// ============================================================================

void* PlatformAPI::PreloadNextTrack(const char* fileName) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->PreloadNextTrack(fileName);
    }
    return nullptr;
}

void PlatformAPI::SwitchToNextTrack() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->SwitchToNextTrack();
    }
}

void PlatformAPI::ClearNextTrack() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->ClearNextTrack();
    }
}

void PlatformAPI::StartCrossfade(float duration) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->StartCrossfade(duration);
    }
}

void PlatformAPI::UpdateCrossfade(float deltaTime) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->UpdateCrossfade(deltaTime);
    }
}

void PlatformAPI::CompleteCrossfade() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->CompleteCrossfade();
    }
}

void PlatformAPI::FadeOutMusic(float duration) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->FadeOutMusic(duration);
    }
}

void PlatformAPI::FadeInMusic(float duration) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->FadeInMusic(duration);
    }
}

void PlatformAPI::UpdateFade(float deltaTime) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->UpdateFade(deltaTime);
    }
}

// ============================================================================
// FONT AND TEXT FUNCTIONS
// ============================================================================

void* PlatformAPI::LoadFont(const char* fileName, int size) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->LoadFont(fileName, size);
    }
    return nullptr;
}

void PlatformAPI::UnloadFont(void* font) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->UnloadFont(font);
    }
}

Vector2 PlatformAPI::MeasureText(const char* text, void* font, float fontSize, float spacing) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->MeasureText(text, font, fontSize, spacing);
    }
    return { 0, 0 };
}

void PlatformAPI::DrawText(const char* text, float x, float y, float fontSize, Color color, void* font) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->DrawText(text, x, y, fontSize, color, font);
    }
}

// ============================================================================
// INPUT FUNCTIONS
// ============================================================================

bool PlatformAPI::IsPrimaryInputDown() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsPrimaryInputDown();
    }
    return false;
}

bool PlatformAPI::IsPrimaryInputPressed() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsPrimaryInputPressed();
    }
    return false;
}

bool PlatformAPI::IsPrimaryInputReleased() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsPrimaryInputReleased();
    }
    return false;
}

Vector2 PlatformAPI::GetPrimaryInputPosition() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetPrimaryInputPosition();
    }
    return { 0, 0 };
}

bool PlatformAPI::IsSecondaryInputDown() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsSecondaryInputDown();
    }
    return false;
}

bool PlatformAPI::IsSecondaryInputPressed() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsSecondaryInputPressed();
    }
    return false;
}

bool PlatformAPI::IsSecondaryInputReleased() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsSecondaryInputReleased();
    }
    return false;
}

bool PlatformAPI::IsTouchSupported() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsTouchSupported();
    }
    return false;
}

int PlatformAPI::GetTouchCount() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetTouchCount();
    }
    return 0;
}

Vector2 PlatformAPI::GetTouchPosition(int index) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetTouchPosition(index);
    }
    return { 0, 0 };
}

std::vector<Vector2> PlatformAPI::GetTouchPoints() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetTouchPoints();
    }
    return {};
}

// ============================================================================
// SCREEN AND WINDOW FUNCTIONS
// ============================================================================

void PlatformAPI::InitWindow(int width, int height, const char* title) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->Initialize();
    }
}

void PlatformAPI::CloseWindow() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->Shutdown();
    }
}

bool PlatformAPI::WindowShouldClose() {
    // iOS doesn't have a close window concept
    return false;
}

void PlatformAPI::SetTargetFPS(int fps) {
    // PlatformIOS will handle FPS limiting internally
    // This could be implemented if needed
}

void PlatformAPI::BeginDrawing() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->BeginDrawing(nullptr);
    }
}

void PlatformAPI::EndDrawing() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->EndDrawing(nullptr);
    }
}

void PlatformAPI::ClearBackground(Color color) {
    if (GetPlatformImpl()) {
        // Draw a full-screen rectangle to clear the background
        int width = GetScreenWidth();
        int height = GetScreenHeight();
        GetPlatformImpl()->DrawRectangle(0, 0, width, height, color);
    }
}

int PlatformAPI::GetScreenWidth() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetScreenWidth();
    }
    return 0;
}

int PlatformAPI::GetScreenHeight() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetScreenHeight();
    }
    return 0;
}

Vector2 PlatformAPI::GetScreenSize() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetScreenSize();
    }
    return { 0, 0 };
}

float PlatformAPI::GetScreenDensity() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetScreenDensity();
    }
    return 1.0f;
}

float PlatformAPI::GetScreenScale() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetScreenScale();
    }
    return 1.0f;
}

Rectangle PlatformAPI::GetSafeArea() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetSafeArea();
    }
    return { 0, 0, 0, 0 };
}

bool PlatformAPI::IsWindowFullscreen() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsWindowFullscreen();
    }
    return false;
}

void PlatformAPI::ToggleFullscreen() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->ToggleFullscreen();
    }
}

void PlatformAPI::SetWindowTitle(const std::string& title) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->SetWindowTitle(title);
    }
}

void PlatformAPI::SetWindowSize(int width, int height) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->SetWindowSize(width, height);
    }
}

bool PlatformAPI::SupportsFullscreen() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->SupportsFullscreen();
    }
    return false;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

double PlatformAPI::GetCurrentTime() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetCurrentTime();
    }
    return 0.0;
}

float PlatformAPI::GetLastFrameTime() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetLastFrameTime();
    }
    return 0.0f;
}

int PlatformAPI::GetLastFPS() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetLastFPS();
    }
    return 0;
}

std::string PlatformAPI::GetResourcePath(const std::string& relativePath) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetResourcePath(relativePath);
    }
    return "";
}

std::string PlatformAPI::GetSavePath(const std::string& filename) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetSavePath(filename);
    }
    return "";
}

std::string PlatformAPI::GetPlatformResourcePath(const std::string& relativePath) {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetPlatformResourcePath(relativePath);
    }
    return "";
}

bool PlatformAPI::PreferLowPowerMode() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->PreferLowPowerMode();
    }
    return false;
}

int PlatformAPI::GetRecommendedTextureSize() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->GetRecommendedTextureSize();
    }
    return 1024;
}

void PlatformAPI::SetOrientation(bool landscape) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->SetOrientation(landscape);
    }
}

void PlatformAPI::ShowVirtualKeyboard(bool show) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->ShowVirtualKeyboard(show);
    }
}

bool PlatformAPI::IsVirtualKeyboardShown() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsVirtualKeyboardShown();
    }
    return false;
}

void PlatformAPI::Vibrate(int milliseconds) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->Vibrate(milliseconds);
    }
}

// ============================================================================
// APP LIFECYCLE
// ============================================================================

void PlatformAPI::OnAppWillResignActive() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->OnAppWillResignActive();
    }
}

void PlatformAPI::OnAppDidBecomeActive() {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->OnAppDidBecomeActive();
    }
}

// ============================================================================
// PLATFORM DETECTION
// ============================================================================

bool PlatformAPI::IsMobilePlatform() {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsMobilePlatform();
    }
    return false;
}

bool PlatformAPI::IsTouchSupported() const {
    if (GetPlatformImpl()) {
        return GetPlatformImpl()->IsTouchSupported();
    }
    return false;
} 