#import "MetalRaylibCompat.h"
#import "MetalRenderer.h"
#import "MetalTexture.h"
#import "MetalShader.h"
#import "MetalTextRenderer.h"
#import "iOS/GameViewController.h"
#import "PlatformLayer.h"
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>

#if defined(__APPLE__) && TARGET_OS_IOS && defined(USE_METAL_RENDERER)

// Global state
static MetalRenderer* g_metalRenderer = nullptr;
static UIWindow* g_window = nullptr;
static GameViewController* g_viewController = nullptr;
static bool g_shouldClose = false;
static int g_targetFPS = 60;
static double g_frameStartTime = 0;
static Rectangle g_safeAreaInsets = {0, 0, 0, 0};

// Input state
struct TouchState {
    Vector2 position = {0, 0};
    Vector2 prevPosition = {0, 0};
    bool is_down = false;
    bool was_down = false;
};

static std::unordered_map<int, TouchState> g_touchStates;
const int MAX_TOUCHES = 10;

// Audio state
static AVAudioEngine* g_audioEngine = nullptr;
static AVAudioPlayerNode* g_soundPlayerNode = nullptr;
static AVAudioMixerNode* g_mainMixer = nullptr;
static std::unordered_map<std::string, AVAudioPCMBuffer*> g_soundBuffers;

// Default font
static Font g_defaultFont = {nullptr, 16, 0, 16, {nullptr, 0, 0}};

// ========== WINDOW FUNCTIONS ==========

void InitWindow(int width, int height, const char* title) {
    @autoreleasepool {
        // Create Metal renderer
        g_metalRenderer = new MetalRenderer();
        
        // Get the shared application and its key window
        UIApplication* app = [UIApplication sharedApplication];
        g_window = app.keyWindow;
        
        if (!g_window) {
            // If no key window, we might be initializing too early
            // The window will be set up by AppDelegate/GameViewController
            return;
        }
        
        // Get the view controller (should be GameViewController)
        g_viewController = (GameViewController*)g_window.rootViewController;
        
        // Initialize Metal renderer with the MTKView
        if (g_viewController && [g_viewController isKindOfClass:[GameViewController class]]) {
            MTKView* metalView = (MTKView*)g_viewController.view;
            g_metalRenderer->Initialize(metalView);
            
            // Initialize text renderer
            g_textRenderer = new MetalTextRenderer();
            g_textRenderer->Initialize(g_metalRenderer->GetDevice());
        }
        
        TraceLog(LOG_INFO, "Window initialized: %dx%d - %s", width, height, title);
    }
}

void CloseWindow() {
    if (g_textRenderer) {
        g_textRenderer->Shutdown();
        delete g_textRenderer;
        g_textRenderer = nullptr;
    }
    
    if (g_metalRenderer) {
        g_metalRenderer->Shutdown();
        delete g_metalRenderer;
        g_metalRenderer = nullptr;
    }
    
    CloseAudioDevice(); // Clean up audio resources

    if (g_defaultFont.ctFont) {
        CFRelease(g_defaultFont.ctFont);
        g_defaultFont.ctFont = nullptr;
    }
}

bool WindowShouldClose() {
    // Process any pending UI events
    @autoreleasepool {
        // In iOS, we don't typically "close" windows
        // This would be triggered by app lifecycle events
        return g_shouldClose;
    }
}

void SetTargetFPS(int fps) {
    g_targetFPS = fps;
    if (g_viewController && [g_viewController isKindOfClass:[GameViewController class]]) {
        MTKView* metalView = (MTKView*)g_viewController.view;
        metalView.preferredFramesPerSecond = fps;
    }
}

int GetScreenWidth() {
    if (g_window) {
        return g_window.bounds.size.width * g_window.screen.scale;
    }
    return 0;
}

int GetScreenHeight() {
    if (g_window) {
        return g_window.bounds.size.height * g_window.screen.scale;
    }
    return 0;
}

// ========== DRAWING FUNCTIONS ==========

void BeginDrawing() {
    g_frameStartTime = CACurrentMediaTime();

    // Update was_down state for all touches
    for (auto& pair : g_touchStates) {
        pair.second.was_down = pair.second.is_down;
    }

    if (g_metalRenderer) {
        g_metalRenderer->BeginFrame();
    }
}

void EndDrawing() {
    if (g_metalRenderer) {
        g_metalRenderer->EndFrame();
        g_metalRenderer->Present();
    }
    
    // Frame rate limiting
    double frameTime = CACurrentMediaTime() - g_frameStartTime;
    double targetFrameTime = 1.0 / g_targetFPS;
    if (frameTime < targetFrameTime) {
        usleep((targetFrameTime - frameTime) * 1000000);
    }

    // After rendering, update previous positions
    for (auto& pair : g_touchStates) {
        if (pair.second.is_down) {
            pair.second.prevPosition = pair.second.position;
        }
    }
}

void ClearBackground(Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->Clear(color);
    }
}

// ========== TEXTURE FUNCTIONS ==========

Texture2D LoadTexture(const char* fileName) {
    Texture2D texture = {nullptr, 0, 0};
    
    if (!g_metalRenderer) {
        TraceLog(LOG_ERROR, "MetalRenderer not initialized");
        return texture;
    }
    
    id<MTLTexture> metalTexture = MetalTexture::LoadFromFile(fileName, g_metalRenderer->GetDevice());
    if (metalTexture) {
        texture.texture = (__bridge_retained void*)metalTexture;
        texture.width = metalTexture.width;
        texture.height = metalTexture.height;
        TraceLog(LOG_INFO, "Texture loaded: %s (%dx%d)", fileName, texture.width, texture.height);
    } else {
        TraceLog(LOG_ERROR, "Failed to load texture: %s", fileName);
    }
    
    return texture;
}

void UnloadTexture(Texture2D texture) {
    if (texture.texture) {
        id<MTLTexture> metalTexture = (__bridge_transfer id<MTLTexture>)texture.texture;
        metalTexture = nil;
    }
}

// ========== DRAWING PRIMITIVES ==========

void DrawRectangle(int posX, int posY, int width, int height, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawRectangle(posX, posY, width, height, color);
    }
}

void DrawRectangleRec(Rectangle rec, Color color) {
    DrawRectangle(rec.x, rec.y, rec.width, rec.height, color);
}

void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawRectangleRounded(rec.x, rec.y, rec.width, rec.height, roundness, segments, color);
    }
}

void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    if (!g_metalRenderer) return;
    
    if (roundness <= 0 || segments <= 0) {
        // Draw regular rectangle outline
        DrawLine(rec.x, rec.y, rec.x + rec.width, rec.y, color); // top
        DrawLine(rec.x + rec.width, rec.y, rec.x + rec.width, rec.y + rec.height, color); // right
        DrawLine(rec.x + rec.width, rec.y + rec.height, rec.x, rec.y + rec.height, color); // bottom
        DrawLine(rec.x, rec.y + rec.height, rec.x, rec.y, color); // left
        return;
    }
    
    // Clamp roundness to reasonable values
    float maxRadius = fminf(rec.width, rec.height) * 0.5f;
    float radius = fminf(roundness, maxRadius);
    
    if (radius <= 0) {
        // Fall back to regular rectangle outline
        DrawLine(rec.x, rec.y, rec.x + rec.width, rec.y, color);
        DrawLine(rec.x + rec.width, rec.y, rec.x + rec.width, rec.y + rec.height, color);
        DrawLine(rec.x + rec.width, rec.y + rec.height, rec.x, rec.y + rec.height, color);
        DrawLine(rec.x, rec.y + rec.height, rec.x, rec.y, color);
        return;
    }
    
    // Calculate corner centers
    float left = rec.x + radius;
    float right = rec.x + rec.width - radius;
    float top = rec.y + radius;
    float bottom = rec.y + rec.height - radius;
    
    // Draw straight lines
    DrawLine(left, rec.y, right, rec.y, color); // top
    DrawLine(rec.x + rec.width, top, rec.x + rec.width, bottom, color); // right
    DrawLine(right, rec.y + rec.height, left, rec.y + rec.height, color); // bottom
    DrawLine(rec.x, bottom, rec.x, top, color); // left
    
    // Draw rounded corners (simplified - just quarter circles)
    // This is a basic implementation; a full implementation would draw arc segments
    for (int corner = 0; corner < 4; corner++) {
        float centerX, centerY;
        switch (corner) {
            case 0: centerX = left; centerY = top; break;     // top-left
            case 1: centerX = right; centerY = top; break;    // top-right
            case 2: centerX = right; centerY = bottom; break; // bottom-right
            case 3: centerX = left; centerY = bottom; break;  // bottom-left
        }
        
        // Draw a small circle at each corner (simplified)
        g_metalRenderer->DrawCircle(centerX, centerY, radius * 0.1f, color);
    }
}

void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawLine(startPosX, startPosY, endPosX, endPosY, color);
    }
}

void DrawCircleV(Vector2 center, float radius, Color color) {
    if (g_metalRenderer) {
        g_metalRenderer->DrawCircle(center.x, center.y, radius, color);
    }
}

void DrawCircle(int centerX, int centerY, float radius, Color color) {
    DrawCircleV({(float)centerX, (float)centerY}, radius, color);
}

// ========== INPUT FUNCTIONS ==========

void UpdateTouchState(int touchId, float x, float y, bool is_down) {
    if (g_touchStates.find(touchId) == g_touchStates.end() && g_touchStates.size() >= MAX_TOUCHES) {
        return; // Max touches reached
    }
    
    g_touchStates[touchId].position = {x, y};
    g_touchStates[touchId].is_down = is_down;
}

void ClearAllTouchStates() {
    g_touchStates.clear();
}

Vector2 GetMousePosition() {
    if (!g_touchStates.empty()) {
        // Return position of the first active touch
        for (const auto& pair : g_touchStates) {
            if (pair.second.is_down) {
                return pair.second.position;
            }
        }
    }
    return { -1.0f, -1.0f }; // No active touch
}

Vector2 GetMouseDelta() {
    if (!g_touchStates.empty()) {
        for (const auto& pair : g_touchStates) {
            if (pair.second.is_down) {
                return Vector2Subtract(pair.second.position, pair.second.prevPosition);
            }
        }
    }
    return { 0.0f, 0.0f };
}

bool IsMouseButtonPressed(int button) {
    if (button == MOUSE_BUTTON_LEFT && !g_touchStates.empty()) {
        for (const auto& pair : g_touchStates) {
            if (pair.second.is_down && !pair.second.was_down) {
                return true; // Pressed this frame
            }
        }
    }
    return false;
}

bool IsMouseButtonReleased(int button) {
    if (button == MOUSE_BUTTON_LEFT && !g_touchStates.empty()) {
        for (const auto& pair : g_touchStates) {
            if (!pair.second.is_down && pair.second.was_down) {
                return true; // Released this frame
            }
        }
    }
    return false;
}

bool IsMouseButtonDown(int button) {
    if (button == MOUSE_BUTTON_LEFT && !g_touchStates.empty()) {
        for (const auto& pair : g_touchStates) {
            if (pair.second.is_down) {
                return true;
            }
        }
    }
    return false;
}

// ========== TOUCH FUNCTIONS ==========

int GetTouchPointCount() {
    return g_touchStates.size();
}

Vector2 GetTouchPosition(int index) {
    if (index >= 0 && index < g_touchStates.size()) {
        UITouch* touch = g_activeTouches[index];
        CGPoint location = [touch locationInView:g_viewController.view];
        float scale = g_window.screen.scale;
        return {location.x * scale, location.y * scale};
    }
    return {0, 0};
}

// Multi-touch functions
static std::vector<TouchState*> get_active_touches() {
    std::vector<TouchState*> active_touches;
    for (auto& pair : g_touchStates) {
        if (pair.second.is_down || pair.second.was_down) { // Include touches that were just released
            active_touches.push_back(&pair.second);
        }
    }
    return active_touches;
}

int GetTouchCount() {
    int count = 0;
    for (const auto& pair : g_touchStates) {
        if (pair.second.is_down) {
            count++;
        }
    }
    return count;
}

bool IsTouchDown(int index) {
    auto active_touches = get_active_touches();
    if (index >= 0 && index < active_touches.size()) {
        return active_touches[index]->is_down;
    }
    return false;
}

bool IsTouchPressed(int index) {
    auto active_touches = get_active_touches();
    if (index >= 0 && index < active_touches.size()) {
        return active_touches[index]->is_down && !active_touches[index]->was_down;
    }
    return false;
}

bool IsTouchReleased(int index) {
    auto active_touches = get_active_touches();
    if (index >= 0 && index < active_touches.size()) {
        return !active_touches[index]->is_down && active_touches[index]->was_down;
    }
    return false;
}

// ========== UTILITY FUNCTIONS ==========

void TraceLog(int logLevel, const char* text, ...) {
    va_list args;
    va_start(args, text);
    
    NSString* format = [NSString stringWithUTF8String:text];
    NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
    
    va_end(args);
    
    switch (logLevel) {
        case LOG_INFO:
            NSLog(@"[INFO] %@", message);
            break;
        case LOG_WARNING:
            NSLog(@"[WARNING] %@", message);
            break;
        case LOG_ERROR:
            NSLog(@"[ERROR] %@", message);
            break;
        default:
            NSLog(@"%@", message);
            break;
    }
}

// ========== MATH FUNCTIONS ==========

float Lerp(float start, float end, float amount) {
    return start + (end - start) * amount;
}

int GetRandomValue(int min, int max) {
    return min + arc4random_uniform(max - min + 1);
}

// ========== VECTOR2 FUNCTIONS ==========

Vector2 Vector2Zero() {
    return {0.0f, 0.0f};
}

Vector2 Vector2One() {
    return {1.0f, 1.0f};
}

Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
    return {v1.x + v2.x, v1.y + v2.y};
}

Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
    return {v1.x - v2.x, v1.y - v2.y};
}

Vector2 Vector2Scale(Vector2 v, float scale) {
    return {v.x * scale, v.y * scale};
}

float Vector2Length(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

float Vector2Distance(Vector2 v1, Vector2 v2) {
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    return sqrtf(dx * dx + dy * dy);
}

// ========== COLLISION FUNCTIONS ==========

bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
    return (point.x >= rec.x) && (point.x <= (rec.x + rec.width)) &&
           (point.y >= rec.y) && (point.y <= (rec.y + rec.height));
}

bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
    return (rec1.x < rec2.x + rec2.width && rec1.x + rec1.width > rec2.x &&
            rec1.y < rec2.y + rec2.height && rec1.y + rec1.height > rec2.y);
}

// ========== TIME FUNCTIONS ==========

float GetFrameTime() {
    static double lastTime = 0;
    double currentTime = CACurrentMediaTime();
    float deltaTime = (lastTime > 0) ? (currentTime - lastTime) : 0.016f;
    lastTime = currentTime;
    return deltaTime;
}

double GetTime() {
    return CACurrentMediaTime();
}

// ========== MORE TEXTURE FUNCTIONS ==========

void DrawTextureV(Texture2D texture, Vector2 position, Color tint) {
    DrawTexture(texture, position.x, position.y, tint);
}

void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) {
    if (!texture.texture || !g_metalRenderer) return;
    
    // For now, implement basic scaling without rotation
    Rectangle dest = {position.x, position.y, texture.width * scale, texture.height * scale};
    Rectangle source = {0, 0, 1, 1}; // Full texture
    
    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.texture;
    g_metalRenderer->DrawTexture(metalTexture, source, dest, tint);
}

void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) {
    if (!texture.texture || !g_metalRenderer) return;
    
    // Convert source from pixel coordinates to normalized coordinates
    Rectangle normalizedSource = {
        source.x / texture.width,
        source.y / texture.height,
        source.width / texture.width,
        source.height / texture.height
    };
    
    Rectangle dest = {position.x, position.y, source.width, source.height};
    
    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.texture;
    g_metalRenderer->DrawTexture(metalTexture, normalizedSource, dest, tint);
}

void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
    if (g_metalRenderer && texture.texture) {
        id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.texture;
        g_metalRenderer->DrawTexture(metalTexture, source, dest, origin, rotation, tint);
    }
}

// ========== IMAGE FUNCTIONS ==========

Image LoadImage(const char* fileName) {
    Image image = {nullptr, 0, 0};
    
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:fileName];
        UIImage* uiImage = [UIImage imageNamed:path];
        
        if (!uiImage) {
            uiImage = [UIImage imageWithContentsOfFile:path];
        }
        
        if (uiImage) {
            image.cgImage = CGImageRetain(uiImage.CGImage);
            image.width = uiImage.size.width * uiImage.scale;
            image.height = uiImage.size.height * uiImage.scale;
        }
    }
    
    return image;
}

void UnloadImage(Image image) {
    if (image.cgImage) {
        CGImageRelease(image.cgImage);
    }
}

Texture2D LoadTextureFromImage(Image image) {
    Texture2D texture = {nullptr, 0, 0};
    
    if (!image.cgImage || !g_metalRenderer) {
        return texture;
    }
    
    id<MTLTexture> metalTexture = MetalTexture::CreateFromCGImage(image.cgImage, g_metalRenderer->GetDevice());
    if (metalTexture) {
        texture.texture = (__bridge_retained void*)metalTexture;
        texture.width = metalTexture.width;
        texture.height = metalTexture.height;
    }
    
    return texture;
}

Image GenImageColor(int width, int height, Color color) {
    Image image = {nullptr, width, height};
    
    if (!g_metalRenderer) {
        return image;
    }
    
    // Create a solid color CGImage
    @autoreleasepool {
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        uint8_t* pixelData = (uint8_t*)malloc(width * height * 4);
        
        for (int i = 0; i < width * height; i++) {
            pixelData[i * 4 + 0] = color.r;
            pixelData[i * 4 + 1] = color.g;
            pixelData[i * 4 + 2] = color.b;
            pixelData[i * 4 + 3] = color.a;
        }
        
        CGContextRef context = CGBitmapContextCreate(pixelData, width, height, 8, width * 4,
                                                   colorSpace, kCGImageAlphaPremultipliedLast);
        
        if (context) {
            image.cgImage = CGBitmapContextCreateImage(context);
            CGContextRelease(context);
        }
        
        CGColorSpaceRelease(colorSpace);
        free(pixelData);
    }
    
    return image;
}

Image LoadImageFromTexture(Texture2D texture) {
    Image image = {nullptr, 0, 0};
    
    if (!texture.texture) {
        return image;
    }
    
    // This is a complex operation on Metal - would require reading back from GPU
    // For now, return an empty image with the texture dimensions
    image.width = texture.width;
    image.height = texture.height;
    
    TraceLog(LOG_WARNING, "LoadImageFromTexture: GPU texture readback not implemented on Metal");
    
    return image;
}

// ========== AUDIO FUNCTIONS ==========

void InitAudioDevice() {
    @autoreleasepool {
        g_audioEngine = [[AVAudioEngine alloc] init];
        g_soundPlayerNode = [[AVAudioPlayerNode alloc] init];
        g_mainMixer = [g_audioEngine mainMixerNode];

        [g_audioEngine attachNode:g_soundPlayerNode];
        [g_audioEngine connect:g_soundPlayerNode to:g_mainMixer format:nil];

        NSError* error = nil;
        if (![g_audioEngine startAndReturnError:&error]) {
            TraceLog(LOG_ERROR, "Failed to start audio engine: %s", [[error localizedDescription] UTF8String]);
            return;
        }

        // Configure audio session
        AVAudioSession* session = [AVAudioSession sharedInstance];
        [session setCategory:AVAudioSessionCategoryAmbient error:nil];
        [session setActive:YES error:nil];
        
        TraceLog(LOG_INFO, "Audio device initialized.");
    }
}

void CloseAudioDevice() {
    @autoreleasepool {
        if (g_audioEngine && [g_audioEngine isRunning]) {
            [g_audioEngine stop];
        }
        for (auto const& [key, val] : g_soundBuffers) {
            val = nil;
        }
        g_soundBuffers.clear();

        g_soundPlayerNode = nil;
        g_mainMixer = nil;
        g_audioEngine = nil;
        
        TraceLog(LOG_INFO, "Audio device closed.");
    }
}

Sound LoadSound(const char* fileName) {
    Sound sound = {nullptr, 0, 0};
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:fileName];
        NSURL* url = [NSURL fileURLWithPath:path];
        NSError* error = nil;
        
        AVAudioFile* audioFile = [[AVAudioFile alloc] initForReading:url error:&error];
        if (!audioFile) {
            TraceLog(LOG_ERROR, "Failed to load audio file: %s", [[error localizedDescription] UTF8String]);
            return sound;
        }
        
        AVAudioFormat* format = audioFile.processingFormat;
        AVAudioFrameCount capacity = (AVAudioFrameCount)audioFile.length;
        AVAudioPCMBuffer* buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:format frameCapacity:capacity];
        
        if (![audioFile readIntoBuffer:buffer error:&error]) {
            TraceLog(LOG_ERROR, "Failed to read audio buffer: %s", [[error localizedDescription] UTF8String]);
            return sound;
        }
        
        sound.chunk = (__bridge_retained void*)buffer;
        sound.sampleCount = capacity;
    }
    return sound;
}

void UnloadSound(Sound sound) {
    if (sound.chunk) {
        AVAudioPCMBuffer* buffer = (__bridge_transfer AVAudioPCMBuffer*)sound.chunk;
        buffer = nil;
    }
}

void PlaySound(Sound sound) {
    if (!g_soundPlayerNode || !sound.chunk) return;
    
    AVAudioPCMBuffer* buffer = (__bridge AVAudioPCMBuffer*)sound.chunk;
    [g_soundPlayerNode scheduleBuffer:buffer completionHandler:nil];
    [g_soundPlayerNode play];
}

void SetSoundVolume(Sound sound, float volume) {
    // This is more complex as individual sounds don't have volume controls this way
    // We control the volume on the player node itself.
    if (g_soundPlayerNode) {
        g_soundPlayerNode.volume = volume;
    }
}

// ========== MUSIC STREAM FUNCTIONS ==========

Music LoadMusicStream(const char* fileName) {
    Music music = {nullptr, 1.0f, false};
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:fileName];
        NSURL* url = [NSURL fileURLWithPath:path];
        NSError* error = nil;
        AVAudioPlayer* player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
        if (player) {
            music.music = (__bridge_retained void*)player;
            [player prepareToPlay];
        } else {
            TraceLog(LOG_ERROR, "Failed to load music stream: %s", [[error localizedDescription] UTF8String]);
        }
    }
    return music;
}

void UnloadMusicStream(Music music) {
    if (music.music) {
        AVAudioPlayer* player = (__bridge_transfer AVAudioPlayer*)music.music;
        player = nil;
    }
}

void PlayMusicStream(Music music) {
    if (music.music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.music;
        [player play];
    }
}

void StopMusicStream(Music music) {
    if (music.music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.music;
        [player stop];
        player.currentTime = 0;
        [player prepareToPlay];
    }
}

void PauseMusicStream(Music music) {
    if (music.music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.music;
        [player pause];
    }
}

void ResumeMusicStream(Music music) {
    if (music.music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.music;
        [player play];
    }
}

void SetMusicVolume(Music music, float volume) {
    if (music.music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.music;
        player.volume = volume;
    }
}

bool IsMusicStreamPlaying(Music music) {
    if (music.music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.music;
        return player.isPlaying;
    }
    return false;
}

void UpdateMusicStream(Music music) {
    if (music.music && music.looping) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music.music;
        if (!player.isPlaying) {
            player.currentTime = 0;
            [player play];
        }
    }
}

// ========== REMAINING VECTOR2 FUNCTIONS ==========

Vector2 Vector2AddValue(Vector2 v, float add) {
    return {v.x + add, v.y + add};
}

Vector2 Vector2SubtractValue(Vector2 v, float sub) {
    return {v.x - sub, v.y - sub};
}

float Vector2LengthSqr(Vector2 v) {
    return v.x * v.x + v.y * v.y;
}

float Vector2DotProduct(Vector2 v1, Vector2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

float Vector2DistanceSqr(Vector2 v1, Vector2 v2) {
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    return dx * dx + dy * dy;
}

float Vector2Angle(Vector2 v1, Vector2 v2) {
    return atan2f(v2.y - v1.y, v2.x - v1.x);
}

Vector2 Vector2Multiply(Vector2 v1, Vector2 v2) {
    return {v1.x * v2.x, v1.y * v2.y};
}

Vector2 Vector2Negate(Vector2 v) {
    return {-v.x, -v.y};
}

Vector2 Vector2Divide(Vector2 v1, Vector2 v2) {
    return {v1.x / v2.x, v1.y / v2.y};
}

Vector2 Vector2Normalize(Vector2 v) {
    float length = Vector2Length(v);
    if (length > 0) {
        return {v.x / length, v.y / length};
    }
    return {0, 0};
}

Vector2 Vector2Lerp(Vector2 v1, Vector2 v2, float amount) {
    return {
        v1.x + (v2.x - v1.x) * amount,
        v1.y + (v2.y - v1.y) * amount
    };
}

Vector2 Vector2Rotate(Vector2 v, float angle) {
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    return {
        v.x * cos_a - v.y * sin_a,
        v.x * sin_a + v.y * cos_a
    };
}

bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec) {
    float dx = fmaxf(rec.x - center.x, fmaxf(0, center.x - (rec.x + rec.width)));
    float dy = fmaxf(rec.y - center.y, fmaxf(0, center.y - (rec.y + rec.height)));
    return (dx * dx + dy * dy) <= (radius * radius);
}

// ========== MISSING UTILITY FUNCTIONS ==========

void SetConfigFlags(unsigned int flags) {
    // Configuration flags are not applicable on iOS
    TraceLog(LOG_INFO, "SetConfigFlags called (ignored on iOS): %u", flags);
}

void SetExitKey(int key) {
    // Exit key not applicable on iOS
}

void ToggleFullscreen() {
    // Fullscreen toggle not applicable on iOS
}

void SetWindowPosition(int x, int y) {
    // Window position not applicable on iOS
}

int GetCurrentMonitor() {
    return 0; // iOS devices have one screen
}

int GetMonitorWidth(int monitor) {
    return GetScreenWidth();
}

int GetMonitorHeight(int monitor) {
    return GetScreenHeight();
}

void SetWindowSize(int width, int height) {
    if (g_metalRenderer) {
        g_metalRenderer->SetViewport(width, height);
    }
}

bool IsWindowFullscreen() {
    return true; // iOS apps are always "fullscreen"
}

Font GetFontDefault() {
    if (g_textRenderer) {
        return g_textRenderer->GetDefaultFont();
    }
    return g_defaultFont;
}

void BeginScissorMode(int x, int y, int width, int height) {
    if (g_metalRenderer) {
        // Metal scissor test will be implemented in the renderer
        // For now, just note the scissor rect
        TraceLog(LOG_INFO, "BeginScissorMode: %d,%d %dx%d", x, y, width, height);
    }
}

void EndScissorMode() {
    if (g_metalRenderer) {
        // Disable scissor test
        TraceLog(LOG_INFO, "EndScissorMode");
    }
}

int MeasureText(const char* text, int fontSize) {
    if (!text || !g_textRenderer) return 0;
    
    Vector2 size = g_textRenderer->MeasureText(text, fontSize);
    return (int)size.x;
}

Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
    if (!text || !g_textRenderer) return {0, 0};
    
    return g_textRenderer->MeasureTextEx(font, text, fontSize, spacing);
}

Color Fade(Color color, float alpha) {
    Color result = color;
    result.a = (unsigned char)(color.a * Clamp(alpha, 0.0f, 1.0f));
    return result;
}

inline float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void DrawText(const char* text, int posX, int posY, int fontSize, Color color) {
    if (!text || !g_metalRenderer || !g_textRenderer) return;
    
    // Render text to texture and draw it
    id<MTLTexture> textTexture = g_textRenderer->RenderTextToTexture(text, fontSize, color);
    if (textTexture) {
        Rectangle source = {0, 0, 1, 1}; // Full texture
        Rectangle dest = {(float)posX, (float)posY, (float)textTexture.width, (float)textTexture.height};
        g_metalRenderer->DrawTexture(textTexture, source, dest, WHITE);
    }
}

void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
    if (g_textRenderer && font.fontData) {
        NSString* nsText = [NSString stringWithUTF8String:text];
        CTFontRef ctFont = (__bridge CTFontRef)font.fontData;
        g_textRenderer->DrawText(nsText, ctFont, position.x, position.y, tint);
    }
}

Font LoadFont(const char* fileName) {
    if (!g_textRenderer) {
        Font font = {nullptr, 16, 0, 16, {nullptr, 0, 0}};
        return font;
    }
    
    return g_textRenderer->LoadFont(fileName, 16);
}

Font LoadFontEx(const char* fileName, int fontSize, int* codepoints, int codepointCount) {
    if (!g_textRenderer) {
        Font font = {nullptr, fontSize, 0, fontSize, {nullptr, 0, 0}};
        return font;
    }
    
    return g_textRenderer->LoadFont(fileName, fontSize);
}

void UnloadFont(Font font) {
    if (g_textRenderer) {
        g_textRenderer->UnloadFont(font);
    }
}

void ImageResize(Image* image, int newWidth, int newHeight) {
    if (!image || !image->cgImage) return;
    
    @autoreleasepool {
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(nullptr, newWidth, newHeight, 8, 
                                                   newWidth * 4, colorSpace, 
                                                   kCGImageAlphaPremultipliedLast);
        
        if (context) {
            CGContextDrawImage(context, CGRectMake(0, 0, newWidth, newHeight), image->cgImage);
            
            CGImageRelease(image->cgImage);
            image->cgImage = CGBitmapContextCreateImage(context);
            image->width = newWidth;
            image->height = newHeight;
            
            CGContextRelease(context);
        }
        
        CGColorSpaceRelease(colorSpace);
    }
}

void ImageDraw(Image* dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint) {
    if (!dst || !dst->cgImage || !src.cgImage) return;
    
    @autoreleasepool {
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(nullptr, dst->width, dst->height, 8, 
                                                   dst->width * 4, colorSpace, 
                                                   kCGImageAlphaPremultipliedLast);
        
        if (context) {
            // Draw the destination image first
            CGContextDrawImage(context, CGRectMake(0, 0, dst->width, dst->height), dst->cgImage);
            
            // Apply tint if needed (simplified implementation)
            if (tint.r != 255 || tint.g != 255 || tint.b != 255 || tint.a != 255) {
                CGContextSetRGBFillColor(context, tint.r/255.0f, tint.g/255.0f, tint.b/255.0f, tint.a/255.0f);
                CGContextSetBlendMode(context, kCGBlendModeMultiply);
            }
            
            // Create cropped source image
            CGImageRef croppedSrc = CGImageCreateWithImageInRect(src.cgImage, 
                CGRectMake(srcRec.x, srcRec.y, srcRec.width, srcRec.height));
            
            if (croppedSrc) {
                CGContextDrawImage(context, CGRectMake(dstRec.x, dstRec.y, dstRec.width, dstRec.height), croppedSrc);
                CGImageRelease(croppedSrc);
            }
            
            CGImageRelease(dst->cgImage);
            dst->cgImage = CGBitmapContextCreateImage(context);
            
            CGContextRelease(context);
        }
        
        CGColorSpaceRelease(colorSpace);
    }
}

void UpdateSafeAreaInsets(float top, float right, float bottom, float left) {
    g_safeAreaInsets = { left, top, right, bottom };
}

void OnAppPause() {
    PlatformLayer::GetInstance().OnAppWillResignActive();
}

void OnAppResume() {
    PlatformLayer::GetInstance().OnAppDidBecomeActive();
}

#endif // defined(__APPLE__) && TARGET_OS_IOS && defined(USE_METAL_RENDERER) 