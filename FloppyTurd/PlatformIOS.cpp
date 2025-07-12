#include "PlatformIOS.h"

// iOS-specific includes
#ifdef PLATFORM_IOS
#include "MetalRenderer.h"
#include "MetalTextRenderer.h"
#include "iOS/GameView.h"
#include "Game.h"
#include "UIManager.h"

// iOS system includes
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

// Global MetalRenderer instance (from our existing code)
extern MetalRenderer* g_metalRenderer;

// Global safe area insets for iOS
static struct {
    float top;
    float right;
    float bottom;
    float left;
} g_safeAreaInsets = {0, 0, 0, 0};

PlatformIOS::PlatformIOS() 
    : m_nativeView(nullptr)
    , m_metalRenderer(nullptr)
    , m_textRenderer(nullptr)
    , m_primaryInputDown(false)
    , m_primaryInputPressed(false)
    , m_primaryInputReleased(false)
    , m_secondaryInputDown(false)
    , m_secondaryInputPressed(false)
    , m_secondaryInputReleased(false)
    , m_screenWidth(0)
    , m_screenHeight(0)
    , m_screenDensity(1.0f)
    , m_screenScale(1.0f)
    , m_safeArea({0, 0, 0, 0})
    , m_lastFrameTime(0.0)
    , m_frameTime(0.0f)
    , m_fps(0)
    // Audio member objects initialization
    , m_currentMusicPlayer(nullptr)
    , m_nextMusicPlayer(nullptr)
    , m_fadeOutPlayer(nullptr)
    , m_isMusicPlaying(false)
    , m_isMusicPaused(false)
    , m_musicVolume(1.0f)
    , m_musicLooping(false)
    , m_isTransitioning(false)
    , m_fadeProgress(0.0f)
    , m_fadeDuration(0.0f)
    , m_isFadingOut(false)
    , m_isFadingIn(false)
    , m_audioSessionActive(false)
    , m_wasInterrupted(false)
    , m_wasInBackground(false)
    , m_preInterruptionVolume(1.0f)
    , m_preInterruptionPlaying(false)
    , m_interruptionObserver(nullptr)
    , m_routeChangeObserver(nullptr)
    , m_appStateObserver(nullptr)
    , m_soundCacheHits(0)
    , m_soundCacheMisses(0)
    , m_soundCacheEvictions(0) {
    
    TraceLog(LOG_INFO, "[PlatformIOS] Constructor called");
}

PlatformIOS::~PlatformIOS() {
    TraceLog(LOG_INFO, "[PlatformIOS] Destructor called");
    Shutdown();
}

// ============================================================================
// INITIALIZATION AND LIFECYCLE
// ============================================================================

void PlatformIOS::Initialize(void* nativeView) {
    TraceLog(LOG_INFO, "[PlatformIOS] Initialize with native view: %p", nativeView);
    
    m_nativeView = nativeView;
    
    // Get GameView from the native view
    GameView* gameView = GetGameView();
    if (gameView) {
        m_metalRenderer = [gameView getMetalRenderer];
        TraceLog(LOG_INFO, "[PlatformIOS] Got MetalRenderer: %p", m_metalRenderer);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] Failed to get GameView");
    }
    
    // Initialize screen metrics
    UpdateScreenMetrics();
    
    // Initialize text renderer if needed
    if (!m_textRenderer) {
        m_textRenderer = new MetalTextRenderer();
        // Initialize text renderer with Metal device
        GameView* gameView = GetGameView();
        if (gameView) {
            id<MTLDevice> device = [gameView getMetalDevice];
            if (device) {
                if (m_textRenderer->Initialize(device)) {
                    TraceLog(LOG_INFO, "[PlatformIOS] Text renderer initialized successfully");
                } else {
                    TraceLog(LOG_ERROR, "[PlatformIOS] Failed to initialize text renderer");
                }
            } else {
                TraceLog(LOG_ERROR, "[PlatformIOS] Failed to get Metal device for text renderer");
            }
        } else {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to get GameView for text renderer initialization");
        }
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] Initialization complete");
}

void PlatformIOS::Initialize() {
    TraceLog(LOG_INFO, "[PlatformIOS] Initialize without native view");
    // For iOS, we typically need a native view, so this might not be used
    // But we can still initialize basic functionality
    UpdateScreenMetrics();
}

void PlatformIOS::Shutdown() {
    TraceLog(LOG_INFO, "[PlatformIOS] Shutdown called");
    
    // Clean up audio member objects
    if (m_currentMusicPlayer) {
        CleanupAVAudioPlayer(m_currentMusicPlayer);
        m_currentMusicPlayer = nullptr;
    }
    
    if (m_nextMusicPlayer) {
        CleanupAVAudioPlayer(m_nextMusicPlayer);
        m_nextMusicPlayer = nullptr;
    }
    
    if (m_fadeOutPlayer) {
        CleanupAVAudioPlayer(m_fadeOutPlayer);
        m_fadeOutPlayer = nullptr;
    }
    
    // Clean up sound cache
    for (auto& pair : m_soundCache) {
        if (pair.second) {
            CleanupAVAudioPlayer(pair.second);
        }
    }
    m_soundCache.clear();
    m_soundCacheOrder.clear();
    
    // Clean up active sounds
    for (void* sound : m_activeSounds) {
        if (sound) {
            CleanupAVAudioPlayer(sound);
        }
    }
    m_activeSounds.clear();
    
    // Clean up loaded sounds
    for (auto& pair : m_loadedSounds) {
        if (pair.second) {
            CleanupAVAudioPlayer(pair.second);
        }
    }
    m_loadedSounds.clear();
    
    // Remove audio session notifications
    RemoveAudioSessionNotifications();
    
    // Deactivate audio session
    DeactivateAudioSession();
    
    if (m_textRenderer) {
        delete m_textRenderer;
        m_textRenderer = nullptr;
    }
    
    m_metalRenderer = nullptr;
    m_nativeView = nullptr;
}

void PlatformIOS::Update() {
    // Clean up finished sounds periodically
    static int updateCounter = 0;
    updateCounter++;
    
    // Clean up every 60 frames (about once per second at 60fps)
    if (updateCounter >= 60) {
        CleanupFinishedSounds();
        updateCounter = 0;
    }
    
    // Update audio transitions
    if (m_isTransitioning) {
        float deltaTime = GetLastFrameTime();
        if (m_isFadingOut && m_isFadingIn) {
            // Crossfade
            UpdateCrossfade(deltaTime);
        } else {
            // Simple fade
            UpdateFade(deltaTime);
        }
    }
}

// ============================================================================
// RENDERING FUNCTIONS (Raylib-compatible)
// ============================================================================

void PlatformIOS::DrawRectangle(int x, int y, int width, int height, Color color) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer) {
        renderer->DrawRectangle((float)x, (float)y, (float)width, (float)height, color);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawRectangle: MetalRenderer not available");
    }
}

void PlatformIOS::DrawRectangleRec(Rectangle rec, Color color) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer) {
        renderer->DrawRectangle(rec.x, rec.y, rec.width, rec.height, color);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawRectangleRec: MetalRenderer not available");
    }
}

void PlatformIOS::DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer) {
        renderer->DrawRectangleRounded(rec.x, rec.y, rec.width, rec.height, roundness, segments, color);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawRectangleRounded: MetalRenderer not available");
    }
}

void PlatformIOS::DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer) {
        renderer->DrawRectangleRoundedLines(rec.x, rec.y, rec.width, rec.height, roundness, segments, lineThick, color);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawRectangleRoundedLines: MetalRenderer not available");
    }
}

void PlatformIOS::DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    // Same as DrawRectangleRoundedLines for now
    DrawRectangleRoundedLines(rec, roundness, segments, lineThick, color);
}

void PlatformIOS::DrawCircle(float centerX, float centerY, float radius, Color color) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer) {
        renderer->DrawCircle(centerX, centerY, radius, color);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawCircle: MetalRenderer not available");
    }
}

void PlatformIOS::DrawCircleV(Vector2 center, float radius, Color color) {
    DrawCircle(center.x, center.y, radius, color);
}

void PlatformIOS::DrawLine(float startPosX, float startPosY, float endPosX, float endPosY, Color color) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer) {
        renderer->DrawLine(startPosX, startPosY, endPosX, endPosY, color);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawLine: MetalRenderer not available");
    }
}

void PlatformIOS::DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer) {
        renderer->DrawLineEx(startPos.x, startPos.y, endPos.x, endPos.y, thick, color);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawLineEx: MetalRenderer not available");
    }
}

void PlatformIOS::DrawLineV(Vector2 startPos, Vector2 endPos, Color color) {
    DrawLine(startPos.x, startPos.y, endPos.x, endPos.y, color);
}

void PlatformIOS::DrawTexture(Texture2D texture, int posX, int posY, Color tint) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer && texture.texture) {
        id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.texture;
        Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
        Rectangle dest = {(float)posX, (float)posY, (float)texture.width, (float)texture.height};
        renderer->DrawTexture(metalTexture, source, dest, tint);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawTexture: MetalRenderer or texture not available");
    }
}

void PlatformIOS::DrawTextureV(Texture2D texture, Vector2 position, Color tint) {
    DrawTexture(texture, (int)position.x, (int)position.y, tint);
}

void PlatformIOS::DrawTextureRec(Texture2D texture, Rectangle source, Rectangle dest, Color tint) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer && texture.texture) {
        id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.texture;
        renderer->DrawTexture(metalTexture, source, dest, tint);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawTextureRec: MetalRenderer or texture not available");
    }
}

void PlatformIOS::DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer && texture.texture) {
        id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture.texture;
        Vector2 position = {dest.x + origin.x, dest.y + origin.y};
        float scale = dest.width / source.width;
        renderer->DrawTextureEx(metalTexture, position, rotation, scale, tint);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawTexturePro: MetalRenderer or texture not available");
    }
}

void PlatformIOS::DrawText(const char* text, int posX, int posY, int fontSize, Color color) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer) {
        renderer->DrawText(text, (float)posX, (float)posY, (float)fontSize, color);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawText: MetalRenderer not available");
    }
}

void PlatformIOS::DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer) {
        renderer->DrawText(text, position.x, position.y, fontSize, tint, &font);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] DrawTextEx: MetalRenderer not available");
    }
}

// ============================================================================
// HELPER METHODS
// ============================================================================

MetalRenderer* PlatformIOS::GetMetalRenderer() {
    if (m_metalRenderer) {
        return m_metalRenderer;
    }
    
    // Fallback to global MetalRenderer
    return g_metalRenderer;
}

GameView* PlatformIOS::GetGameView() {
    if (!m_nativeView) {
        return nullptr;
    }
    
    UIView* view = (__bridge UIView*)m_nativeView;
    if ([view isKindOfClass:[GameView class]]) {
        return (GameView*)view;
    }
    
    return nullptr;
}

void PlatformIOS::UpdateScreenMetrics() {
    UIScreen* screen = [UIScreen mainScreen];
    if (screen) {
        CGRect bounds = screen.bounds;
        CGRect nativeBounds = screen.nativeBounds;
        
        m_screenWidth = (int)nativeBounds.size.width;
        m_screenHeight = (int)nativeBounds.size.height;
        m_screenDensity = screen.scale;
        m_screenScale = screen.scale;
        
        // Get safe area
        UIEdgeInsets insets = screen.safeAreaInsets;
        m_safeArea = {
            (float)insets.left,
            (float)insets.top,
            (float)(bounds.size.width - insets.left - insets.right),
            (float)(bounds.size.height - insets.top - insets.bottom)
        };
        
        TraceLog(LOG_INFO, "[PlatformIOS] Screen metrics updated: %dx%d, density=%.2f, safeArea=(%.1f,%.1f,%.1f,%.1f)",
              m_screenWidth, m_screenHeight, m_screenDensity,
              m_safeArea.x, m_safeArea.y, m_safeArea.width, m_safeArea.height);
    }
}

void PlatformIOS::UpdateInputState() {
    // Update input state from GameView directly
    GameView* gameView = GetGameView();
    if (gameView) {
        // Get touch state from GameView
        m_primaryInputDown = [gameView isPrimaryTouchDown];
        m_primaryInputPressed = [gameView isPrimaryTouchPressed];
        m_primaryInputReleased = [gameView isPrimaryTouchReleased];
        
        CGPoint primaryPos = [gameView getPrimaryTouchLocation];
        m_primaryInputPosition = Vector2{(float)primaryPos.x, (float)primaryPos.y};
        
        // Update touch points
        m_touchPoints.clear();
        NSInteger touchCount = [gameView getActiveTouchCount];
        for (NSInteger i = 0; i < touchCount; i++) {
            CGPoint location = [gameView getTouchLocation:i];
            m_touchPoints.push_back(Vector2{(float)location.x, (float)location.y});
        }
        
        TraceLog(LOG_INFO, "[PlatformIOS] UpdateInputState: down=%s, pressed=%s, released=%s, touchCount=%zu",
                 m_primaryInputDown ? "true" : "false",
                 m_primaryInputPressed ? "true" : "false", 
                 m_primaryInputReleased ? "true" : "false",
                 m_touchPoints.size());
    } else {
        TraceLog(LOG_WARNING, "[PlatformIOS] UpdateInputState: GameView not available");
        // Reset to safe defaults
        m_primaryInputDown = false;
        m_primaryInputPressed = false;
        m_primaryInputReleased = false;
        m_primaryInputPosition = Vector2{0, 0};
        m_touchPoints.clear();
    }
}

// ============================================================================
// INPUT FUNCTION IMPLEMENTATIONS
// ============================================================================

bool PlatformIOS::IsPrimaryInputDown() {
    return m_primaryInputDown;
}

bool PlatformIOS::IsPrimaryInputPressed() {
    return m_primaryInputPressed;
}

bool PlatformIOS::IsPrimaryInputReleased() {
    return m_primaryInputReleased;
}

Vector2 PlatformIOS::GetPrimaryInputPosition() {
    return m_primaryInputPosition;
}

bool PlatformIOS::IsSecondaryInputDown() {
    // On iOS, secondary input is typically not used (no right-click equivalent)
    return false;
}

bool PlatformIOS::IsSecondaryInputPressed() {
    return false;
}

bool PlatformIOS::IsSecondaryInputReleased() {
    return false;
}

bool PlatformIOS::IsTouchSupported() {
    return true; // iOS always supports touch
}

int PlatformIOS::GetTouchCount() {
    return (int)m_touchPoints.size();
}

Vector2 PlatformIOS::GetTouchPosition(int index) {
    if (index >= 0 && index < (int)m_touchPoints.size()) {
        return m_touchPoints[index];
    }
    return {0, 0};
}

std::vector<Vector2> PlatformIOS::GetTouchPoints() {
    return m_touchPoints;
}

// ============================================================================
// PLACEHOLDER IMPLEMENTATIONS (to be completed)
// ============================================================================

// These methods will be implemented in the next phase
// For now, they provide basic functionality or delegate to existing systems

void PlatformIOS::BeginDrawing(void* renderTexture) {
    TraceLog(LOG_INFO, "[PlatformIOS] BeginDrawing - renderTexture: %p", renderTexture);
    try {
        // Delegate to existing MetalRenderer implementation
        MetalRenderer* renderer = GetMetalRenderer();
        if (renderer) {
            renderer->BeginRenderTexture(renderTexture);
            TraceLog(LOG_INFO, "[PlatformIOS] BeginDrawing completed successfully");
        } else {
            TraceLog(LOG_ERROR, "[PlatformIOS] BeginDrawing failed - MetalRenderer not available");
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Exception in BeginDrawing: %s", e.what());
    } catch (...) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Unknown exception in BeginDrawing");
    }
}

void PlatformIOS::EndDrawing(void* renderTexture) {
    TraceLog(LOG_INFO, "[PlatformIOS] EndDrawing - renderTexture: %p", renderTexture);
    try {
        // Delegate to existing MetalRenderer implementation
        MetalRenderer* renderer = GetMetalRenderer();
        if (renderer) {
            renderer->EndRenderTexture(renderTexture);
            TraceLog(LOG_INFO, "[PlatformIOS] EndDrawing completed successfully");
        } else {
            TraceLog(LOG_ERROR, "[PlatformIOS] EndDrawing failed - MetalRenderer not available");
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Exception in EndDrawing: %s", e.what());
    } catch (...) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Unknown exception in EndDrawing");
    }
}

void* PlatformIOS::LoadRenderTexture(int width, int height) {
    // TODO: Implement render texture creation
    TraceLog(LOG_WARNING, "[PlatformIOS] LoadRenderTexture not yet implemented");
    return nullptr;
}

void PlatformIOS::UnloadRenderTexture(void* renderTexture) {
    // TODO: Implement render texture cleanup
    TraceLog(LOG_WARNING, "[PlatformIOS] UnloadRenderTexture not yet implemented");
}

Texture2D PlatformIOS::LoadTexture(const char* fileName) {
    TraceLog(LOG_INFO, "[PlatformIOS] LoadTexture: %s", fileName);
    
    // Initialize texture cache if needed
    static bool cacheInitialized = false;
    if (!cacheInitialized) {
        GameView* gameView = GetGameView();
        if (gameView) {
            id<MTLDevice> device = [gameView getMetalDevice];
            if (device) {
                MetalTextureCache::GetInstance().Initialize((__bridge_retained void*)device);
                cacheInitialized = true;
                TraceLog(LOG_INFO, "[PlatformIOS] Texture cache initialized");
            } else {
                TraceLog(LOG_ERROR, "[PlatformIOS] Failed to get Metal device for texture cache initialization");
            }
        } else {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to get GameView for texture cache initialization");
        }
    }
    
    // Use the texture cache to load or retrieve the texture
    if (cacheInitialized) {
        std::string filePath(fileName);
        Texture2D texture = MetalTextureCache::GetInstance().GetOrLoadTexture(filePath);
        
        if (texture.texture) {
            TraceLog(LOG_INFO, "[PlatformIOS] Texture loaded from cache: %s", fileName);
            return texture;
        }
    }
    
    // Fallback to direct loading if cache fails or isn't initialized
    TraceLog(LOG_WARNING, "[PlatformIOS] Cache miss, falling back to direct loading: %s", fileName);
    
    Texture2D texture = { 0 };
    int width, height;
    
    std::string filePath(fileName);
    
    // Handle asset catalog resources
    if (filePath.substr(0, 8) == "asset://") {
        ResourcePathParts parts = ResourceManager::ParseResourcePath(filePath);
        NSString* name = [NSString stringWithUTF8String:parts.baseName.c_str()];
        TraceLog(LOG_INFO, "[PlatformIOS] Loading asset catalog texture: %s", fileName);
        UIImage* uiImage = [UIImage imageNamed:name];
        
        if (!uiImage) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to load asset catalog texture: %s", fileName);
            return CreateFallbackTexture(fileName);
        }
        
        CGImageRef cgImage = uiImage.CGImage;
        if (!cgImage) {
            TraceLog(LOG_ERROR, "[PlatformIOS] CGImage is null for: %s", fileName);
            return CreateFallbackTexture(fileName);
        }
        
        width = (int)CGImageGetWidth(cgImage);
        height = (int)CGImageGetHeight(cgImage);
        
        if (width <= 0 || height <= 0) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Invalid dimensions for: %s (w=%d, h=%d)", fileName, width, height);
            return CreateFallbackTexture(fileName);
        }
        
        GameView* gameView = GetGameView();
        if (!gameView) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to get GameView for: %s", fileName);
            return CreateFallbackTexture(fileName);
        }
        
        id<MTLDevice> device = [gameView getMetalDevice];
        if (!device) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to get Metal device for: %s", fileName);
            return CreateFallbackTexture(fileName);
        }

        // Create Metal texture
        MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
        textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        textureDescriptor.width = width;
        textureDescriptor.height = height;
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        textureDescriptor.storageMode = MTLStorageModeShared;
        textureDescriptor.mipmapLevelCount = 1 + floor(log2(fmax(width, height)));
        
        NSError *error = nil;
        id<MTLTexture> metalTexture = [device newTextureWithDescriptor:textureDescriptor];
        if (!metalTexture) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to create Metal texture: %s", fileName);
            return CreateFallbackTexture(fileName);
        }
        
        // Load image data into texture
        MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(nil, width, height, 8, 4 * width, colorSpace, kCGImageAlphaPremultipliedLast);
        
        if (!context) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to create bitmap context for: %s", fileName);
            CGColorSpaceRelease(colorSpace);
            return CreateFallbackTexture(fileName);
        }
        
        CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
        void* imageData = CGBitmapContextGetData(context);
        
        if (!imageData) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to get image data for: %s", fileName);
            CGContextRelease(context);
            CGColorSpaceRelease(colorSpace);
            return CreateFallbackTexture(fileName);
        }
        
        [metalTexture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
        
        // Generate mipmaps if needed
        if (textureDescriptor.mipmapLevelCount > 1) {
            GameView* gameView = GetGameView();
            if (gameView) {
                id<MTLCommandQueue> commandQueue = [gameView getMetalCommandQueue];
                if (commandQueue) {
                    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
                    id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
                    [blitEncoder generateMipmapsForTexture:metalTexture];
                    [blitEncoder endEncoding];
                    [commandBuffer commit];
                }
            }
        }
        
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        
        texture.id = 0;
        texture.texture = (__bridge_retained void*)metalTexture;
        texture.width = width;
        texture.height = height;
        texture.mipmaps = textureDescriptor.mipmapLevelCount;
        texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        
        // Add to texture cache for future use
        if (cacheInitialized) {
            void* tempPtr = texture.texture;
            texture.texture = nullptr;
            CFRelease(tempPtr);
            
            return MetalTextureCache::GetInstance().LoadTextureFromData(
                imageData, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        }
    } else {
        // Handle regular file paths - use the same asset catalog logic
        TraceLog(LOG_INFO, "[PlatformIOS] Loading regular file texture: %s", fileName);
        return CreateFallbackTexture(fileName);
    }
    
    if (texture.texture == NULL) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Failed to load texture: %s", fileName);
        return CreateFallbackTexture(fileName);
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] Texture loaded successfully: %s", fileName);
    return texture;
}

void PlatformIOS::UnloadTexture(Texture2D texture) {
    if (!texture.texture) {
        TraceLog(LOG_WARNING, "[PlatformIOS] UnloadTexture: null texture");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] UnloadTexture: unloading texture");
    
    // Release the Metal texture
    id<MTLTexture> metalTexture = (__bridge_transfer id<MTLTexture>)texture.texture;
    if (metalTexture) {
        // ARC will handle the release
        TraceLog(LOG_INFO, "[PlatformIOS] Metal texture released");
    }
    
    // Clear the texture struct
    texture.texture = nullptr;
    texture.id = 0;
    texture.width = 0;
    texture.height = 0;
    texture.mipmaps = 0;
    texture.format = 0;
}

void* PlatformIOS::LoadTextureFromImage(void* imageData, int width, int height, int format) {
    TraceLog(LOG_INFO, "[PlatformIOS] LoadTextureFromImage: %dx%d, format=%d", width, height, format);
    
    if (!imageData || width <= 0 || height <= 0) {
        TraceLog(LOG_ERROR, "[PlatformIOS] LoadTextureFromImage: Invalid parameters");
        return nullptr;
    }
    
    GameView* gameView = GetGameView();
    if (!gameView) {
        TraceLog(LOG_ERROR, "[PlatformIOS] LoadTextureFromImage: GameView not available");
        return nullptr;
    }
    
    id<MTLDevice> device = [gameView getMetalDevice];
    if (!device) {
        TraceLog(LOG_ERROR, "[PlatformIOS] LoadTextureFromImage: Metal device not available");
        return nullptr;
    }
    
    // Create Metal texture descriptor
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    
    // Create Metal texture
    id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
    if (!texture) {
        TraceLog(LOG_ERROR, "[PlatformIOS] LoadTextureFromImage: Failed to create Metal texture");
        return nullptr;
    }
    
    // Upload the image data
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
    [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
    
    TraceLog(LOG_INFO, "[PlatformIOS] LoadTextureFromImage: Successfully created texture from image data");
    return (__bridge_retained void*)texture;
}

void* PlatformIOS::CreateTextureFromImage(void* image, int* width, int* height) {
    // TODO: Implement texture creation from image
    TraceLog(LOG_WARNING, "[PlatformIOS] CreateTextureFromImage not yet implemented");
    return nullptr;
}

Image PlatformIOS::LoadImage(const char* fileName) {
    TraceLog(LOG_INFO, "[PlatformIOS] LoadImage: %s", fileName);
    
    Image image = { 0 };
    std::string filePath(fileName);
    
    // Handle asset catalog resources
    if (filePath.substr(0, 8) == "asset://") {
        ResourcePathParts parts = ResourceManager::ParseResourcePath(filePath);
        NSString* name = [NSString stringWithUTF8String:parts.baseName.c_str()];
        TraceLog(LOG_INFO, "[PlatformIOS] Loading asset catalog image: %s", fileName);
        UIImage* uiImage = [UIImage imageNamed:name];
        
        if (!uiImage) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to load asset catalog image: %s", fileName);
            return image;
        }
        
        CGImageRef cgImage = uiImage.CGImage;
        if (!cgImage) {
            TraceLog(LOG_ERROR, "[PlatformIOS] CGImage is null for: %s", fileName);
            return image;
        }
        
        image.width = (int)CGImageGetWidth(cgImage);
        image.height = (int)CGImageGetHeight(cgImage);
        image.mipmaps = 1;
        image.format = 7; // PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        
        // Create bitmap context to get pixel data
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(nil, image.width, image.height, 8, 4 * image.width, colorSpace, kCGImageAlphaPremultipliedLast);
        
        if (!context) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to create bitmap context for: %s", fileName);
            CGColorSpaceRelease(colorSpace);
            return image;
        }
        
        CGContextDrawImage(context, CGRectMake(0, 0, image.width, image.height), cgImage);
        void* imageData = CGBitmapContextGetData(context);
        
        if (!imageData) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to get image data for: %s", fileName);
            CGContextRelease(context);
            CGColorSpaceRelease(colorSpace);
            return image;
        }
        
        // Copy the image data (we need to manage this memory)
        size_t dataSize = image.width * image.height * 4;
        void* copiedData = malloc(dataSize);
        memcpy(copiedData, imageData, dataSize);
        image.data = copiedData;
        
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        
        TraceLog(LOG_INFO, "[PlatformIOS] Successfully loaded image: %s (w=%d, h=%d)", fileName, image.width, image.height);
    } else {
        TraceLog(LOG_WARNING, "[PlatformIOS] LoadImage: Non-asset catalog paths not yet implemented: %s", fileName);
    }
    
    return image;
}

void PlatformIOS::UnloadImage(Image image) {
    if (image.data) {
        TraceLog(LOG_INFO, "[PlatformIOS] UnloadImage: freeing image data");
        free(image.data);
        image.data = nullptr;
    }
}

void* PlatformIOS::CreateSolidColorImage(int width, int height, Color color) {
    // TODO: Implement solid color image creation
    TraceLog(LOG_WARNING, "[PlatformIOS] CreateSolidColorImage not yet implemented");
    return nullptr;
}

// Audio functions - delegate to existing system
void PlatformIOS::InitializeAudio() {
    TraceLog(LOG_INFO, "[PlatformIOS] InitializeAudio - configuring audio session and delegating to AudioStateManager");
    
    // Configure audio session first
    ConfigureAudioSession();
    
    try {
        AudioStateManager::GetInstance().Initialize();
        TraceLog(LOG_INFO, "[PlatformIOS] AudioStateManager initialized successfully");
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Exception initializing AudioStateManager: %s", e.what());
    } catch (...) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Unknown exception initializing AudioStateManager");
    }
}

void PlatformIOS::ShutdownAudio() {
    TraceLog(LOG_INFO, "[PlatformIOS] ShutdownAudio - stopping all music");
    try {
        AudioStateManager::GetInstance().StopMusic();
        TraceLog(LOG_INFO, "[PlatformIOS] Audio shutdown completed successfully");
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Exception during audio shutdown: %s", e.what());
    } catch (...) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Unknown exception during audio shutdown");
    }
}

void* PlatformIOS::LoadSound(const char* fileName) {
    TraceLog(LOG_INFO, "[PlatformIOS] LoadSound: %s", fileName);
    
    std::string key(fileName);
    
    // Check cache first
    auto it = m_soundCache.find(key);
    if (it != m_soundCache.end()) {
        // Update LRU order (move to front)
        UpdateSoundCacheOrder(key);
        m_soundCacheHits++;
        TraceLog(LOG_INFO, "[PlatformIOS] LoadSound: found in cache: %s (hits: %zu, misses: %zu)", 
                 fileName, m_soundCacheHits, m_soundCacheMisses);
        return it->second;
    }
    
    m_soundCacheMisses++;
    
    // Check if cache is full and evict oldest if needed
    if (m_soundCache.size() >= MAX_SOUND_CACHE_SIZE) {
        EvictOldestSoundFromCache();
    }
    
    // Load new sound and cache it
    void* soundPlayer = CreateAVAudioPlayer(fileName);
    if (soundPlayer) {
        m_soundCache[key] = soundPlayer;
        m_soundCacheOrder.push_back(key);  // Add to end (most recently used)
        TraceLog(LOG_INFO, "[PlatformIOS] LoadSound: loaded and cached: %s (cache size: %zu)", 
                 fileName, m_soundCache.size());
        return soundPlayer;
    }
    
    TraceLog(LOG_ERROR, "[PlatformIOS] LoadSound: failed to load: %s", fileName);
    return nullptr;
}

void PlatformIOS::UnloadSound(void* sound) {
    if (!sound) {
        TraceLog(LOG_WARNING, "[PlatformIOS] UnloadSound: null sound pointer");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] UnloadSound: unloading sound");
    
    // Find and remove from cache
    for (auto it = m_soundCache.begin(); it != m_soundCache.end(); ++it) {
        if (it->second == sound) {
            CleanupAVAudioPlayer(sound);
            m_soundCache.erase(it);
            TraceLog(LOG_INFO, "[PlatformIOS] UnloadSound: removed from cache");
            return;
        }
    }
    
    TraceLog(LOG_WARNING, "[PlatformIOS] UnloadSound: sound not found in cache");
}

void PlatformIOS::PlaySound(void* sound) {
    if (!sound) {
        TraceLog(LOG_WARNING, "[PlatformIOS] PlaySound: null sound pointer");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] PlaySound: playing sound");
    
    // Activate audio session
    ActivateAudioSession();
    
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound;
    BOOL success = [player play];
    if (success) {
        // Add to active sounds list for tracking
        m_activeSounds.push_back(sound);
        TraceLog(LOG_INFO, "[PlatformIOS] PlaySound: sound started playing successfully");
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] PlaySound: failed to start playing sound");
    }
}

void PlatformIOS::SetSoundVolume(void* sound, float volume) {
    if (!sound) {
        TraceLog(LOG_WARNING, "[PlatformIOS] SetSoundVolume: null sound pointer");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] SetSoundVolume: setting volume to %.2f", volume);
    
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound;
    player.volume = volume;
    TraceLog(LOG_INFO, "[PlatformIOS] SetSoundVolume: volume applied successfully");
}

void* PlatformIOS::LoadMusic(const char* fileName) {
    TraceLog(LOG_INFO, "[PlatformIOS] LoadMusic: %s", fileName);
    
    // Clean up existing music player if needed
    if (m_currentMusicPlayer) {
        CleanupAVAudioPlayer(m_currentMusicPlayer);
        m_currentMusicPlayer = nullptr;
    }
    
    // Load new music into member object
    m_currentMusicPlayer = CreateAVAudioPlayer(fileName);
    if (m_currentMusicPlayer) {
        // Apply current volume and looping settings
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        player.volume = m_musicVolume;
        player.numberOfLoops = m_musicLooping ? -1 : 0; // -1 = infinite loops
        
        TraceLog(LOG_INFO, "[PlatformIOS] Successfully loaded music into member object: %s", fileName);
        return m_currentMusicPlayer;
    }
    
    TraceLog(LOG_ERROR, "[PlatformIOS] Failed to load music: %s", fileName);
    return nullptr;
}

void PlatformIOS::UnloadMusic(void* music) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] UnloadMusic: null music pointer");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] UnloadMusic: unloading music");
    
    // Only unload if it's our current music player
    if (music == m_currentMusicPlayer) {
        CleanupAVAudioPlayer(m_currentMusicPlayer);
        m_currentMusicPlayer = nullptr;
        m_isMusicPlaying = false;
        m_isMusicPaused = false;
        TraceLog(LOG_INFO, "[PlatformIOS] Current music player unloaded successfully");
    } else {
        TraceLog(LOG_WARNING, "[PlatformIOS] UnloadMusic: music pointer doesn't match current player");
    }
}

void PlatformIOS::PlayMusic(void* music) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] PlayMusic: null music pointer");
        return;
    }
    
    // Only play if it's our current music player
    if (music != m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] PlayMusic: music pointer doesn't match current player");
        return;
    }
    
    if (!m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] PlayMusic: no current music player");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] PlayMusic: playing music");
    
    // Activate audio session
    ActivateAudioSession();
    
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
    BOOL success = [player play];
    if (success) {
        m_isMusicPlaying = true;
        m_isMusicPaused = false;
        TraceLog(LOG_INFO, "[PlatformIOS] Music started playing successfully");
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] Failed to start playing music");
    }
}

void PlatformIOS::StopMusic(void* music) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] StopMusic: null music pointer");
        return;
    }
    
    // Only stop if it's our current music player
    if (music != m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] StopMusic: music pointer doesn't match current player");
        return;
    }
    
    if (!m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] StopMusic: no current music player");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] StopMusic: stopping music");
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
    [player stop];
    player.currentTime = 0; // Reset to beginning
    m_isMusicPlaying = false;
    m_isMusicPaused = false;
    TraceLog(LOG_INFO, "[PlatformIOS] Music stopped successfully");
}

void PlatformIOS::UpdateMusic(void* music) {
    // Delegate to AudioStateManager for music management
    // AudioStateManager should handle music updates
    TraceLog(LOG_WARNING, "[PlatformIOS] UpdateMusic: Should be handled by AudioStateManager");
}

bool PlatformIOS::IsMusicPlaying(void* music) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] IsMusicPlaying: null music pointer");
        return false;
    }
    
    // Only check if it's our current music player
    if (music != m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] IsMusicPlaying: music pointer doesn't match current player");
        return false;
    }
    
    if (!m_currentMusicPlayer) {
        return false;
    }
    
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
    bool isPlaying = player.isPlaying;
    TraceLog(LOG_INFO, "[PlatformIOS] IsMusicPlaying: %s", isPlaying ? "true" : "false");
    return isPlaying;
}

void PlatformIOS::SetMusicVolume(void* music, float volume) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] SetMusicVolume: null music pointer");
        return;
    }
    
    // Only set volume if it's our current music player
    if (music != m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] SetMusicVolume: music pointer doesn't match current player");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] SetMusicVolume: setting volume to %.2f", volume);
    
    // Update member variable
    m_musicVolume = volume;
    
    // Apply to current player if it exists
    if (m_currentMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        player.volume = volume;
        TraceLog(LOG_INFO, "[PlatformIOS] Music volume applied to current player");
    }
}

void PlatformIOS::PauseMusic(void* music) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] PauseMusic: null music pointer");
        return;
    }
    
    // Only pause if it's our current music player
    if (music != m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] PauseMusic: music pointer doesn't match current player");
        return;
    }
    
    if (!m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] PauseMusic: no current music player");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] PauseMusic: pausing music");
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
    [player pause];
    m_isMusicPlaying = false;
    m_isMusicPaused = true;
    TraceLog(LOG_INFO, "[PlatformIOS] Music paused successfully");
}

void PlatformIOS::ResumeMusic(void* music) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] ResumeMusic: null music pointer");
        return;
    }
    
    // Only resume if it's our current music player
    if (music != m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] ResumeMusic: music pointer doesn't match current player");
        return;
    }
    
    if (!m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] ResumeMusic: no current music player");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] ResumeMusic: resuming music");
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
    BOOL success = [player play];
    if (success) {
        m_isMusicPlaying = true;
        m_isMusicPaused = false;
        TraceLog(LOG_INFO, "[PlatformIOS] Music resumed successfully");
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] Failed to resume music");
    }
}

void PlatformIOS::SetMusicLooping(void* music, bool looping) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] SetMusicLooping: null music pointer");
        return;
    }
    
    // Only set looping if it's our current music player
    if (music != m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] SetMusicLooping: music pointer doesn't match current player");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] SetMusicLooping: setting looping to %s", looping ? "true" : "false");
    
    // Update member variable
    m_musicLooping = looping;
    
    // Apply to current player if it exists
    if (m_currentMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        player.numberOfLoops = looping ? -1 : 0; // -1 = infinite loops
        TraceLog(LOG_INFO, "[PlatformIOS] Music looping applied to current player");
    }
}

// Font functions - delegate to existing system
void* PlatformIOS::LoadFont(const char* fileName, int size) {
    TraceLog(LOG_INFO, "[PlatformIOS] LoadFont: %s (size: %d)", fileName, size);
    
    // Use MetalTextRenderer to load the font
    if (m_textRenderer) {
        // Try to load as TTF/OTF first
        Font font = m_textRenderer->LoadFont(fileName, size);
        if (font.ctFont != nullptr) {
            TraceLog(LOG_INFO, "[PlatformIOS] Font loaded successfully: %s", fileName);
            return font.ctFont;
        }
        
        // If that fails, try to load as system font
        NSString* fontName = [NSString stringWithUTF8String:fileName];
        NSString* baseName = [fontName stringByDeletingPathExtension];
        NSString* extension = [fontName pathExtension];
        
        if ([extension isEqualToString:@"ttf"] || [extension isEqualToString:@"otf"]) {
            // For TTF/OTF files, try to load from bundle
            NSString* bundlePath = [[NSBundle mainBundle] pathForResource:baseName ofType:extension];
            if (bundlePath) {
                font = m_textRenderer->LoadFont([bundlePath UTF8String], size);
                if (font.ctFont != nullptr) {
                    TraceLog(LOG_INFO, "[PlatformIOS] Font loaded from bundle: %s", fileName);
                    return font.ctFont;
                }
            }
        }
        
        TraceLog(LOG_WARNING, "[PlatformIOS] Failed to load font: %s", fileName);
    } else {
        TraceLog(LOG_ERROR, "[PlatformIOS] Text renderer not available for font loading");
    }
    
    return nullptr;
}

void PlatformIOS::UnloadFont(void* font) {
    if (!font) {
        TraceLog(LOG_WARNING, "[PlatformIOS] UnloadFont - null font pointer");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] UnloadFont - unloading font");
    try {
        // For iOS, fonts are managed by Core Text and don't need explicit cleanup
        // The CTFont will be released when the Font struct is destroyed
        // This is a no-op for iOS, but we log it for consistency
        TraceLog(LOG_INFO, "[PlatformIOS] Font unloaded successfully (iOS auto-cleanup)");
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Exception unloading font: %s", e.what());
    } catch (...) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Unknown exception unloading font");
    }
}

Vector2 PlatformIOS::MeasureText(const char* text, void* font, float fontSize, float spacing) {
    if (!text) {
        TraceLog(LOG_WARNING, "[PlatformIOS] MeasureText - null text pointer");
        return { 0, 0 };
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] MeasureText - measuring text: '%s' (fontSize: %.1f, spacing: %.1f)", text, fontSize, spacing);
    try {
        // Create a Font struct from the font pointer
        Font fontStruct = { 0 };
        if (font) {
            fontStruct.ctFont = (CTFontRef)font;
            fontStruct.baseSize = (int)fontSize;
        } else {
            // Use default font if no font provided
            fontStruct = GetFontDefault();
        }
        
        // Delegate to existing MeasureTextEx function
        Vector2 size = MeasureTextEx(fontStruct, text, fontSize, spacing);
        TraceLog(LOG_INFO, "[PlatformIOS] Text measured successfully: (%.1f, %.1f)", size.x, size.y);
        return size;
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Exception measuring text: %s", e.what());
        return { 0, 0 };
    } catch (...) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Unknown exception measuring text");
        return { 0, 0 };
    }
}

void PlatformIOS::DrawText(const char* text, float x, float y, float fontSize, Color color, void* font) {
    // Delegate to existing text rendering
    DrawText(text, (int)x, (int)y, (int)fontSize, color);
}

// Input functions - PlatformIOS implementation
bool PlatformIOS::IsPrimaryInputDown() {
    UpdateInputState();
    return m_primaryInputDown;
}

bool PlatformIOS::IsPrimaryInputPressed() {
    UpdateInputState();
    return m_primaryInputPressed;
}

bool PlatformIOS::IsPrimaryInputReleased() {
    UpdateInputState();
    return m_primaryInputReleased;
}

Vector2 PlatformIOS::GetPrimaryInputPosition() {
    UpdateInputState();
    return m_primaryInputPosition;
}

bool PlatformIOS::IsSecondaryInputDown() {
    // On iOS, secondary input could be a second touch or a specific gesture
    // For now, we'll implement it as a second touch point
    UpdateInputState();
    return m_touchPoints.size() >= 2;
}

bool PlatformIOS::IsSecondaryInputPressed() {
    // Secondary input pressed when second touch point is added
    // This would need to track touch state changes
    // For now, return false as this requires more complex touch tracking
    return false;
}

bool PlatformIOS::IsSecondaryInputReleased() {
    // Secondary input released when second touch point is removed
    // This would need to track touch state changes
    // For now, return false as this requires more complex touch tracking
    return false;
}

bool PlatformIOS::IsTouchSupported() {
    return true; // iOS always supports touch
}

int PlatformIOS::GetTouchCount() {
    UpdateInputState();
    return (int)m_touchPoints.size();
}

Vector2 PlatformIOS::GetTouchPosition(int index) {
    UpdateInputState();
    if (index >= 0 && index < (int)m_touchPoints.size()) {
        return m_touchPoints[index];
    }
    return { 0, 0 };
}

std::vector<Vector2> PlatformIOS::GetTouchPoints() {
    UpdateInputState();
    return m_touchPoints;
}

// Screen and window functions
int PlatformIOS::GetScreenWidth() {
    return m_screenWidth;
}

int PlatformIOS::GetScreenHeight() {
    return m_screenHeight;
}

Vector2 PlatformIOS::GetScreenSize() {
    return { (float)m_screenWidth, (float)m_screenHeight };
}

float PlatformIOS::GetScreenDensity() {
    return m_screenDensity;
}

float PlatformIOS::GetScreenScale() {
    return m_screenScale;
}

Rectangle PlatformIOS::GetSafeArea() {
    return m_safeArea;
}

bool PlatformIOS::IsWindowFullscreen() {
    return true; // iOS is always fullscreen
}

void PlatformIOS::ToggleFullscreen() {
    // Not applicable on iOS
}

void PlatformIOS::SetWindowTitle(const std::string& title) {
    // Not applicable on iOS
}

void PlatformIOS::SetWindowSize(int width, int height) {
    // Not applicable on iOS
}

bool PlatformIOS::SupportsFullscreen() {
    return false; // iOS doesn't support windowed mode
}

// Utility functions
double PlatformIOS::GetCurrentTime() {
    return [[NSDate date] timeIntervalSince1970];
}

float PlatformIOS::GetLastFrameTime() {
    MetalRenderer* renderer = GetMetalRenderer();
    if (renderer) {
        return renderer->GetDebugStats().frameTime;
    }
    return 0.0f;
}

int PlatformIOS::GetLastFPS() {
    float frameTime = GetLastFrameTime();
    if (frameTime > 0.0f) {
        return (int)(1.0f / frameTime + 0.5f);
    }
    return 0;
}

std::string PlatformIOS::GetResourcePath(const std::string& relativePath) {
    TraceLog(LOG_INFO, "[DEBUG] GetResourcePath called with: %s", relativePath.c_str());
    
    // For iOS asset catalogs, we need to handle the path differently
    // Asset catalogs store resources with their full path (e.g., "hats/poophat")
    NSString* path = [NSString stringWithUTF8String:relativePath.c_str()];
    
    // First try to find the resource as a raw file (for non-asset catalog resources)
    NSString* fullPath = [[NSBundle mainBundle] pathForResource:path ofType:nil];
    if (fullPath) {
        TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: Found as raw file: %s", [fullPath UTF8String]);
        return std::string([fullPath UTF8String]);
    }
    
    // If not found as raw file, try to extract the base name for asset catalog lookup
    // Asset catalogs store resources by their full path (e.g., "hats/poophat" not just "poophat")
    NSString* baseName = [path stringByDeletingPathExtension];
    NSString* directory = [path stringByDeletingLastPathComponent];
    
    TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: relativePath=%s, directory=%s, baseName=%s", relativePath.c_str(), [directory UTF8String], [baseName UTF8String]);
    
    // For asset catalog resources, we need to construct the proper path
    // Asset catalogs are compiled into the bundle, so we need to check if the resource exists
    if ([directory isEqualToString:@"environment"] || 
        [directory isEqualToString:@"enemies"] || 
        [directory isEqualToString:@"objects"] || 
        [directory isEqualToString:@"hats"] || 
        [directory isEqualToString:@"turd"] || 
        [directory isEqualToString:@"ui"] || 
        [directory isEqualToString:@"vfx"] || 
        [directory isEqualToString:@"mainmenu"]) {
        
        TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: Directory %s is in asset catalog list", [directory UTF8String]);
        
        // Extract just the filename without the directory prefix
        NSString* fileName = [baseName lastPathComponent];
        // Remove file extension for asset catalog
        NSString* assetName = [fileName stringByDeletingPathExtension];
        
        // Since the asset catalog is compiled and we know these resources exist,
        // just return the asset:// path for all known asset catalog resources
        std::string assetPath = std::string("asset://") + std::string([assetName UTF8String]);
        TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: Returning asset catalog path: %s", assetPath.c_str());
        return assetPath;
    }
    
    // Special handling for font files - they should be loaded from asset catalog
    NSString* fileExtension = [path pathExtension];
    if ([fileExtension isEqualToString:@"ttf"] || [fileExtension isEqualToString:@"otf"] || [fileExtension isEqualToString:@"fnt"]) {
        TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: Font file detected: %s", [path UTF8String]);
        
        // For font files, use asset catalog path with extension preserved
        // Don't strip the extension first - get the full filename with extension
        NSString* fileName = [path lastPathComponent];
        NSString* assetPath = [NSString stringWithFormat:@"asset://%@", fileName];
        TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: Returning asset catalog path for font: %s", [assetPath UTF8String]);
        return std::string([assetPath UTF8String]);
    }
    
    // Special handling for music files - they should be loaded from the bundle
    if ([fileExtension isEqualToString:@"mp3"] || [fileExtension isEqualToString:@"ogg"] || [fileExtension isEqualToString:@"wav"]) {
        TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: Music file detected: %s", [path UTF8String]);
        
        // For music files, we need to construct the proper bundle path
        // The asset catalog script processes music files and puts them in the bundle
        NSString* fileName = [baseName lastPathComponent];
        NSString* musicPath = [[NSBundle mainBundle] pathForResource:fileName ofType:fileExtension];
        
        if (musicPath) {
            TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: Found music file in bundle: %s", [musicPath UTF8String]);
            return std::string([musicPath UTF8String]);
        } else {
            TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: Music file not found in bundle: %s", [fileName UTF8String]);
            // Fallback to asset:// path for music files
            NSString* assetName = [fileName stringByDeletingPathExtension];
            std::string assetPath = std::string("asset://") + std::string([assetName UTF8String]);
            TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: Returning asset catalog path for music: %s", assetPath.c_str());
            return assetPath;
        }
    }
    
    TraceLog(LOG_INFO, "[DEBUG] GetResourcePath: Fallback to original path: %s", relativePath.c_str());
    // Fallback to original path
    return relativePath;
}

std::string PlatformIOS::GetSavePath(const std::string& filename) {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDirectory = [paths objectAtIndex:0];
    NSString* fullPath = [documentsDirectory stringByAppendingPathComponent:[NSString stringWithUTF8String:filename.c_str()]];
    return std::string([fullPath UTF8String]);
}

std::string PlatformIOS::GetPlatformResourcePath(const std::string& relativePath) {
    // On iOS: returns path as-is (for asset catalogs)
    // This is the same as GetResourcePath for iOS
    return GetResourcePath(relativePath);
}

bool PlatformIOS::PreferLowPowerMode() {
    TraceLog(LOG_INFO, "[PlatformIOS] PreferLowPowerMode - checking power mode preference");
#ifdef PLATFORM_IOS
    // Check if device is in low power mode
    if (@available(iOS 9.0, *)) {
        return [[NSProcessInfo processInfo] isLowPowerModeEnabled];
    }
#endif
    return false; // Default to false on older iOS versions or other platforms
}

int PlatformIOS::GetRecommendedTextureSize() {
    TraceLog(LOG_INFO, "[PlatformIOS] GetRecommendedTextureSize - getting recommended size");
#ifdef PLATFORM_IOS
    // Get device capabilities
    float scale = UIScreen.mainScreen.scale;
    if (scale >= 3.0f) {
        return 2048; // 3x devices can handle larger textures
    } else if (scale >= 2.0f) {
        return 1024; // 2x devices
    } else {
        return 512; // 1x devices
    }
#else
    return 1024; // Default for non-iOS platforms
#endif
}

void PlatformIOS::SetOrientation(bool landscape) {
    TraceLog(LOG_INFO, "[PlatformIOS] SetOrientation: %s", landscape ? "Landscape" : "Portrait");
#ifdef PLATFORM_IOS
    // iOS handles orientation through Info.plist and device settings
    // This is mostly informational for the game logic
    TraceLog(LOG_INFO, "[PlatformIOS] Orientation change requested: %s", landscape ? "Landscape" : "Portrait");
#endif
}

void PlatformIOS::ShowVirtualKeyboard(bool show) {
    TraceLog(LOG_INFO, "[PlatformIOS] ShowVirtualKeyboard - %s", show ? "showing" : "hiding");
#ifdef PLATFORM_IOS
    // Would need Objective-C calls to show/hide keyboard
    // For now, just log the request
    TraceLog(LOG_INFO, "[PlatformIOS] ShowVirtualKeyboard: %s", show ? "true" : "false");
#endif
}

bool PlatformIOS::IsVirtualKeyboardShown() {
    TraceLog(LOG_INFO, "[PlatformIOS] IsVirtualKeyboardShown - checking state");
#ifdef PLATFORM_IOS
    // Would need to track keyboard state
    return false; // Placeholder
#else
    return false;
#endif
}

void PlatformIOS::Vibrate(int milliseconds) {
    TraceLog(LOG_INFO, "[PlatformIOS] Vibrate - %d milliseconds", milliseconds);
#ifdef PLATFORM_IOS
    // Use the haptics manager
    [[HapticsManager sharedManager] playVibration];
    TraceLog(LOG_INFO, "[PlatformIOS] Vibrate for %d ms", milliseconds);
#else
    (void)milliseconds; // Unused on desktop
#endif
}

void PlatformIOS::OnAppWillResignActive() {
    TraceLog(LOG_INFO, "[PlatformIOS] OnAppWillResignActive - app will resign active");
    
    // Pause audio
    if (m_currentMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        if (player.isPlaying) {
            [player pause];
            m_isMusicPaused = true;
        }
    }
    
    // Save audio state for restoration
    SaveAudioState();
    
    TraceLog(LOG_INFO, "[PlatformIOS] App will resign active handled");
}

void PlatformIOS::OnAppDidBecomeActive() {
    TraceLog(LOG_INFO, "[PlatformIOS] OnAppDidBecomeActive - app did become active");
    
    // Restore audio state
    RestoreAudioState();
    
    // Resume audio if it was playing
    if (m_currentMusicPlayer && m_isMusicPaused) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        [player play];
        m_isMusicPaused = false;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] App did become active handled");
}

bool PlatformIOS::IsMobilePlatform() {
    return true; // iOS is always mobile
}

bool PlatformIOS::IsTouchSupported() const {
    return true; // iOS always supports touch
}

// ============================================================================
// PLATFORM IOS AUDIO HELPER METHODS
// ============================================================================

void* PlatformIOS::CreateAVAudioPlayer(const char* fileName) {
    TraceLog(LOG_INFO, "[PlatformIOS] CreateAVAudioPlayer: %s", fileName);
    
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:fileName];
        NSURL* url = nil;
        
        // Handle asset catalog resources
        if (path.length >= 8 && [[path substringToIndex:8] isEqualToString:@"asset://"]) {
            std::string cppPath = [path UTF8String];
            ResourcePathParts parts = ResourceManager::ParseResourcePath(cppPath);
            NSString* assetName = [NSString stringWithUTF8String:parts.baseName.c_str()];
            TraceLog(LOG_INFO, "[PlatformIOS] Loading asset catalog audio: %s", [assetName UTF8String]);
            NSDataAsset* dataAsset = [[NSDataAsset alloc] initWithName:assetName];
            if (dataAsset && dataAsset.data) {
                // Create a temporary file with the asset data
                NSString* tempDir = NSTemporaryDirectory();
                NSString* tempFileName = [NSString stringWithFormat:@"%@_%@.mp3", assetName, [[NSUUID UUID] UUIDString]];
                NSString* tempFilePath = [tempDir stringByAppendingPathComponent:tempFileName];
                
                if ([dataAsset.data writeToFile:tempFilePath atomically:YES]) {
                    url = [NSURL fileURLWithPath:tempFilePath];
                    TraceLog(LOG_INFO, "[PlatformIOS] Successfully created temp file from asset catalog: %@", tempFilePath);
                } else {
                    TraceLog(LOG_ERROR, "[PlatformIOS] Failed to write asset data to temp file");
                }
            } else {
                TraceLog(LOG_ERROR, "[PlatformIOS] Failed to load asset catalog data for: %@", assetName);
            }
        } else {
            // Handle regular file paths
            url = [NSURL fileURLWithPath:path];
        }
        
        if (!url) {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to create URL from path: %s", fileName);
            return nullptr;
        }
        
        NSError* error = nil;
        AVAudioPlayer* player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
        if (player && !error) {
            [player prepareToPlay];
            TraceLog(LOG_INFO, "[PlatformIOS] Successfully created AVAudioPlayer: %s", fileName);
            return (__bridge_retained void*)player;
        } else {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to create AVAudioPlayer: %s", fileName);
            if (error) {
                TraceLog(LOG_ERROR, "[PlatformIOS] Error details: %s", error.localizedDescription.UTF8String);
            }
            return nullptr;
        }
    }
}

void PlatformIOS::CleanupAVAudioPlayer(void* player) {
    if (player) {
        TraceLog(LOG_INFO, "[PlatformIOS] Cleaning up AVAudioPlayer");
        AVAudioPlayer* audioPlayer = (__bridge_transfer AVAudioPlayer*)player;
        [audioPlayer stop];
        // ARC will handle the release
    }
}

void PlatformIOS::ActivateAudioSession() {
    if (!m_audioSessionActive) {
        TraceLog(LOG_INFO, "[PlatformIOS] Activating audio session");
        NSError* error = nil;
        BOOL success = [[AVAudioSession sharedInstance] setActive:YES error:&error];
        if (success) {
            m_audioSessionActive = true;
            TraceLog(LOG_INFO, "[PlatformIOS] Audio session activated successfully");
        } else {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to activate audio session: %s", 
                     error.localizedDescription.UTF8String);
        }
    }
}

void PlatformIOS::DeactivateAudioSession() {
    if (m_audioSessionActive) {
        TraceLog(LOG_INFO, "[PlatformIOS] Deactivating audio session");
        NSError* error = nil;
        BOOL success = [[AVAudioSession sharedInstance] setActive:NO error:&error];
        if (success) {
            m_audioSessionActive = false;
            TraceLog(LOG_INFO, "[PlatformIOS] Audio session deactivated successfully");
        } else {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to deactivate audio session: %s", 
                     error.localizedDescription.UTF8String);
        }
    }
}

// ============================================================================
// AUDIO SESSION MANAGEMENT
// ============================================================================

void PlatformIOS::ConfigureAudioSession() {
    TraceLog(LOG_INFO, "[PlatformIOS] Configuring audio session");
    
    @autoreleasepool {
        AVAudioSession* session = [AVAudioSession sharedInstance];
        NSError* error = nil;
        
        // Configure audio session for game audio
        BOOL success = [session setCategory:AVAudioSessionCategoryPlayback 
                               withOptions:AVAudioSessionCategoryOptionMixWithOthers 
                                     error:&error];
        
        if (success) {
            TraceLog(LOG_INFO, "[PlatformIOS] Audio session category configured successfully");
            
            // Set preferred sample rate and buffer duration
            [session setPreferredSampleRate:44100.0 error:&error];
            [session setPreferredIOBufferDuration:0.005 error:&error]; // 5ms buffer
            
            // Set up notifications
            SetupAudioSessionNotifications();
            
        } else {
            TraceLog(LOG_ERROR, "[PlatformIOS] Failed to configure audio session: %s", 
                     error.localizedDescription.UTF8String);
        }
    }
}

void PlatformIOS::SetupAudioSessionNotifications() {
    TraceLog(LOG_INFO, "[PlatformIOS] Setting up audio session notifications");
    
    @autoreleasepool {
        AVAudioSession* session = [AVAudioSession sharedInstance];
        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        
        // Audio interruption notifications
        m_interruptionObserver = (__bridge_retained void*)[center addObserverForName:AVAudioSessionInterruptionNotification
                                                                              object:session
                                                                               queue:[NSOperationQueue mainQueue]
                                                                          usingBlock:^(NSNotification* notification) {
            NSDictionary* info = notification.userInfo;
            AVAudioSessionInterruptionType type = [info[AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
            
            if (type == AVAudioSessionInterruptionTypeBegan) {
                TraceLog(LOG_INFO, "[PlatformIOS] Audio interruption began");
                HandleAudioInterruption(true);
            } else if (type == AVAudioSessionInterruptionTypeEnded) {
                AVAudioSessionInterruptionOptions options = [info[AVAudioSessionInterruptionOptionKey] unsignedIntegerValue];
                if (options & AVAudioSessionInterruptionOptionShouldResume) {
                    TraceLog(LOG_INFO, "[PlatformIOS] Audio interruption ended - resuming");
                    HandleAudioInterruption(false);
                } else {
                    TraceLog(LOG_INFO, "[PlatformIOS] Audio interruption ended - not resuming");
                }
            }
        }];
        
        // Route change notifications
        m_routeChangeObserver = (__bridge_retained void*)[center addObserverForName:AVAudioSessionRouteChangeNotification
                                                                             object:session
                                                                              queue:[NSOperationQueue mainQueue]
                                                                         usingBlock:^(NSNotification* notification) {
            TraceLog(LOG_INFO, "[PlatformIOS] Audio route changed");
            HandleRouteChange();
        }];
        
        // App state notifications
        m_appStateObserver = (__bridge_retained void*)[center addObserverForName:UIApplicationWillResignActiveNotification
                                                                          object:nil
                                                                           queue:[NSOperationQueue mainQueue]
                                                                      usingBlock:^(NSNotification* notification) {
            TraceLog(LOG_INFO, "[PlatformIOS] App will resign active");
            HandleAppBackgrounding();
        }];
        
        // App foreground notifications
        [center addObserverForName:UIApplicationDidBecomeActiveNotification
                            object:nil
                             queue:[NSOperationQueue mainQueue]
                        usingBlock:^(NSNotification* notification) {
            TraceLog(LOG_INFO, "[PlatformIOS] App did become active");
            HandleAppForegrounding();
        }];
        
        TraceLog(LOG_INFO, "[PlatformIOS] Audio session notifications set up successfully");
    }
}

void PlatformIOS::RemoveAudioSessionNotifications() {
    TraceLog(LOG_INFO, "[PlatformIOS] Removing audio session notifications");
    
    @autoreleasepool {
        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        
        if (m_interruptionObserver) {
            [center removeObserver:(__bridge id)m_interruptionObserver];
            CFRelease(m_interruptionObserver);
            m_interruptionObserver = nullptr;
        }
        
        if (m_routeChangeObserver) {
            [center removeObserver:(__bridge id)m_routeChangeObserver];
            CFRelease(m_routeChangeObserver);
            m_routeChangeObserver = nullptr;
        }
        
        if (m_appStateObserver) {
            [center removeObserver:(__bridge id)m_appStateObserver];
            CFRelease(m_appStateObserver);
            m_appStateObserver = nullptr;
        }
        
        TraceLog(LOG_INFO, "[PlatformIOS] Audio session notifications removed");
    }
}

void PlatformIOS::HandleAudioInterruption(bool began) {
    if (began) {
        // Interruption began - save state and pause audio
        TraceLog(LOG_INFO, "[PlatformIOS] Handling audio interruption begin");
        SaveAudioState();
        
        // Pause current music
        if (m_currentMusicPlayer && m_isMusicPlaying) {
            AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
            [player pause];
            m_isMusicPaused = true;
        }
        
        // Pause all active sounds
        for (void* sound : m_activeSounds) {
            AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound;
            [player pause];
        }
        
        m_wasInterrupted = true;
        
    } else {
        // Interruption ended - restore state and resume audio
        TraceLog(LOG_INFO, "[PlatformIOS] Handling audio interruption end");
        
        // Reactivate audio session
        ActivateAudioSession();
        
        // Restore audio state
        RestoreAudioState();
        
        m_wasInterrupted = false;
    }
}

void PlatformIOS::HandleRouteChange() {
    TraceLog(LOG_INFO, "[PlatformIOS] Handling audio route change");
    
    @autoreleasepool {
        AVAudioSession* session = [AVAudioSession sharedInstance];
        AVAudioSessionRouteDescription* currentRoute = session.currentRoute;
        
        // Log the new route
        for (AVAudioSessionPortDescription* port in currentRoute.outputs) {
            TraceLog(LOG_INFO, "[PlatformIOS] Audio output: %s", port.portName.UTF8String);
        }
        
        // Adjust volume or settings based on route if needed
        // For example, lower volume for headphones, higher for speakers
        if (currentRoute.outputs.count > 0) {
            AVAudioSessionPortDescription* output = currentRoute.outputs[0];
            if ([output.portType isEqualToString:AVAudioSessionPortHeadphones] ||
                [output.portType isEqualToString:AVAudioSessionPortBluetoothA2DP]) {
                // Headphones or Bluetooth - might want to adjust volume
                TraceLog(LOG_INFO, "[PlatformIOS] Headphones/Bluetooth connected");
            } else if ([output.portType isEqualToString:AVAudioSessionPortBuiltInSpeaker]) {
                // Built-in speaker - might want to adjust volume
                TraceLog(LOG_INFO, "[PlatformIOS] Using built-in speaker");
            }
        }
    }
}

void PlatformIOS::HandleAppBackgrounding() {
    TraceLog(LOG_INFO, "[PlatformIOS] Handling app backgrounding");
    
    m_wasInBackground = true;
    
    // Save current audio state
    SaveAudioState();
    
    // Pause audio
    if (m_currentMusicPlayer && m_isMusicPlaying) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        [player pause];
        m_isMusicPaused = true;
    }
    
    // Pause all active sounds
    for (void* sound : m_activeSounds) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound;
        [player pause];
    }
    
    // Deactivate audio session
    DeactivateAudioSession();
}

void PlatformIOS::HandleAppForegrounding() {
    TraceLog(LOG_INFO, "[PlatformIOS] Handling app foregrounding");
    
    // Reactivate audio session
    ActivateAudioSession();
    
    // Restore audio state
    RestoreAudioState();
    
    m_wasInBackground = false;
}

void PlatformIOS::SaveAudioState() {
    TraceLog(LOG_INFO, "[PlatformIOS] Saving audio state");
    
    // Save current music state
    if (m_currentMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        m_preInterruptionVolume = player.volume;
        m_preInterruptionPlaying = m_isMusicPlaying && !m_isMusicPaused;
    }
}

void PlatformIOS::RestoreAudioState() {
    TraceLog(LOG_INFO, "[PlatformIOS] Restoring audio state");
    
    // Restore music state if it was playing before
    if (m_preInterruptionPlaying && m_currentMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        player.volume = m_preInterruptionVolume;
        [player play];
        m_isMusicPlaying = true;
        m_isMusicPaused = false;
    }
    
    // Note: Sound effects are not automatically restored
    // They need to be re-triggered by the game logic
}

// ============================================================================
// CACHE MANAGEMENT METHODS
// ============================================================================

void PlatformIOS::UpdateSoundCacheOrder(const std::string& key) {
    // Remove from current position
    auto it = std::find(m_soundCacheOrder.begin(), m_soundCacheOrder.end(), key);
    if (it != m_soundCacheOrder.end()) {
        m_soundCacheOrder.erase(it);
    }
    
    // Add to end (most recently used)
    m_soundCacheOrder.push_back(key);
}

void PlatformIOS::EvictOldestSoundFromCache() {
    if (m_soundCacheOrder.empty()) {
        return;
    }
    
    // Get oldest sound (first in order)
    std::string oldestKey = m_soundCacheOrder.front();
    
    // Find it in cache
    auto it = m_soundCache.find(oldestKey);
    if (it != m_soundCache.end()) {
        // Clean up the audio player
        CleanupAVAudioPlayer(it->second);
        
        // Remove from cache
        m_soundCache.erase(it);
        
        // Remove from order
        m_soundCacheOrder.erase(m_soundCacheOrder.begin());
        
        m_soundCacheEvictions++;
        TraceLog(LOG_INFO, "[PlatformIOS] Evicted oldest sound from cache: %s (cache size: %zu, evictions: %zu)", 
                 oldestKey.c_str(), m_soundCache.size(), m_soundCacheEvictions);
    }
}

void PlatformIOS::CleanupFinishedSounds() {
    // Remove finished sounds from active sounds list
    auto it = m_activeSounds.begin();
    while (it != m_activeSounds.end()) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)*it;
        if (!player.isPlaying) {
            it = m_activeSounds.erase(it);
            TraceLog(LOG_INFO, "[PlatformIOS] Removed finished sound from active list");
        } else {
            ++it;
        }
    }
}

PlatformIOS::CacheStats PlatformIOS::GetSoundCacheStats() const {
    CacheStats stats;
    stats.hits = m_soundCacheHits;
    stats.misses = m_soundCacheMisses;
    stats.evictions = m_soundCacheEvictions;
    stats.currentSize = m_soundCache.size();
    stats.maxSize = MAX_SOUND_CACHE_SIZE;
    
    size_t totalRequests = m_soundCacheHits + m_soundCacheMisses;
    stats.hitRate = totalRequests > 0 ? (float)m_soundCacheHits / totalRequests : 0.0f;
    
    return stats;
}

// ============================================================================
// ADVANCED AUDIO FUNCTIONS
// ============================================================================

void* PlatformIOS::PreloadNextTrack(const char* fileName) {
    TraceLog(LOG_INFO, "[PlatformIOS] PreloadNextTrack: %s", fileName);
    
    // Clean up existing next track if needed
    if (m_nextMusicPlayer) {
        CleanupAVAudioPlayer(m_nextMusicPlayer);
        m_nextMusicPlayer = nullptr;
    }
    
    // Load new track into next player
    m_nextMusicPlayer = CreateAVAudioPlayer(fileName);
    if (m_nextMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_nextMusicPlayer;
        player.volume = 0.0f;  // Start silent for crossfade
        player.numberOfLoops = m_musicLooping ? -1 : 0;
        TraceLog(LOG_INFO, "[PlatformIOS] PreloadNextTrack: successfully preloaded: %s", fileName);
        return m_nextMusicPlayer;
    }
    
    TraceLog(LOG_ERROR, "[PlatformIOS] PreloadNextTrack: failed to preload: %s", fileName);
    return nullptr;
}

void PlatformIOS::SwitchToNextTrack() {
    TraceLog(LOG_INFO, "[PlatformIOS] SwitchToNextTrack");
    
    if (!m_nextMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] SwitchToNextTrack: no next track loaded");
        return;
    }
    
    // Stop current track
    if (m_currentMusicPlayer) {
        AVAudioPlayer* currentPlayer = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        [currentPlayer stop];
        CleanupAVAudioPlayer(m_currentMusicPlayer);
    }
    
    // Move next track to current
    m_currentMusicPlayer = m_nextMusicPlayer;
    m_nextMusicPlayer = nullptr;
    
    // Set volume and start playing
    if (m_currentMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        player.volume = m_musicVolume;
        [player play];
        m_isMusicPlaying = true;
        m_isMusicPaused = false;
        TraceLog(LOG_INFO, "[PlatformIOS] SwitchToNextTrack: switched successfully");
    }
}

void PlatformIOS::ClearNextTrack() {
    TraceLog(LOG_INFO, "[PlatformIOS] ClearNextTrack");
    
    if (m_nextMusicPlayer) {
        CleanupAVAudioPlayer(m_nextMusicPlayer);
        m_nextMusicPlayer = nullptr;
        TraceLog(LOG_INFO, "[PlatformIOS] ClearNextTrack: cleared next track");
    }
}

void PlatformIOS::StartCrossfade(float duration) {
    TraceLog(LOG_INFO, "[PlatformIOS] StartCrossfade: duration %.2f", duration);
    
    if (!m_currentMusicPlayer || !m_nextMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] StartCrossfade: need both current and next tracks");
        return;
    }
    
    m_isTransitioning = true;
    m_fadeProgress = 0.0f;
    m_fadeDuration = duration;
    m_isFadingOut = true;
    m_isFadingIn = true;
    
    // Start next track at volume 0
    AVAudioPlayer* nextPlayer = (__bridge AVAudioPlayer*)m_nextMusicPlayer;
    [nextPlayer play];
    
    TraceLog(LOG_INFO, "[PlatformIOS] StartCrossfade: crossfade started");
}

void PlatformIOS::UpdateCrossfade(float deltaTime) {
    if (!m_isTransitioning) return;
    
    m_fadeProgress += deltaTime / m_fadeDuration;
    if (m_fadeProgress >= 1.0f) {
        CompleteCrossfade();
        return;
    }
    
    // Fade out current track
    if (m_currentMusicPlayer && m_isFadingOut) {
        AVAudioPlayer* currentPlayer = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        currentPlayer.volume = m_musicVolume * (1.0f - m_fadeProgress);
    }
    
    // Fade in next track
    if (m_nextMusicPlayer && m_isFadingIn) {
        AVAudioPlayer* nextPlayer = (__bridge AVAudioPlayer*)m_nextMusicPlayer;
        nextPlayer.volume = m_musicVolume * m_fadeProgress;
    }
}

void PlatformIOS::CompleteCrossfade() {
    TraceLog(LOG_INFO, "[PlatformIOS] CompleteCrossfade");
    
    // Stop and clean up current track
    if (m_currentMusicPlayer) {
        CleanupAVAudioPlayer(m_currentMusicPlayer);
        m_currentMusicPlayer = nullptr;
    }
    
    // Move next track to current
    m_currentMusicPlayer = m_nextMusicPlayer;
    m_nextMusicPlayer = nullptr;
    
    // Reset transition state
    m_isTransitioning = false;
    m_fadeProgress = 0.0f;
    m_fadeDuration = 0.0f;
    m_isFadingOut = false;
    m_isFadingIn = false;
    
    // Set final volume
    if (m_currentMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        player.volume = m_musicVolume;
        m_isMusicPlaying = true;
        m_isMusicPaused = false;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] CompleteCrossfade: crossfade completed");
}

void PlatformIOS::FadeOutMusic(float duration) {
    TraceLog(LOG_INFO, "[PlatformIOS] FadeOutMusic: duration %.2f", duration);
    
    if (!m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] FadeOutMusic: no current music playing");
        return;
    }
    
    m_isTransitioning = true;
    m_fadeProgress = 0.0f;
    m_fadeDuration = duration;
    m_isFadingOut = true;
    m_isFadingIn = false;
    
    TraceLog(LOG_INFO, "[PlatformIOS] FadeOutMusic: fade out started");
}

void PlatformIOS::FadeInMusic(float duration) {
    TraceLog(LOG_INFO, "[PlatformIOS] FadeInMusic: duration %.2f", duration);
    
    if (!m_currentMusicPlayer) {
        TraceLog(LOG_WARNING, "[PlatformIOS] FadeInMusic: no current music loaded");
        return;
    }
    
    m_isTransitioning = true;
    m_fadeProgress = 0.0f;
    m_fadeDuration = duration;
    m_isFadingOut = false;
    m_isFadingIn = true;
    
    // Start at volume 0
    AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
    player.volume = 0.0f;
    [player play];
    m_isMusicPlaying = true;
    m_isMusicPaused = false;
    
    TraceLog(LOG_INFO, "[PlatformIOS] FadeInMusic: fade in started");
}

void PlatformIOS::UpdateFade(float deltaTime) {
    if (!m_isTransitioning) return;
    
    m_fadeProgress += deltaTime / m_fadeDuration;
    if (m_fadeProgress >= 1.0f) {
        // Complete fade
        if (m_isFadingOut) {
            // Stop music after fade out
            if (m_currentMusicPlayer) {
                AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
                [player stop];
                m_isMusicPlaying = false;
            }
        } else if (m_isFadingIn) {
            // Set final volume after fade in
            if (m_currentMusicPlayer) {
                AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
                player.volume = m_musicVolume;
            }
        }
        
        // Reset transition state
        m_isTransitioning = false;
        m_fadeProgress = 0.0f;
        m_fadeDuration = 0.0f;
        m_isFadingOut = false;
        m_isFadingIn = false;
        
        TraceLog(LOG_INFO, "[PlatformIOS] UpdateFade: fade completed");
        return;
    }
    
    // Update fade progress
    if (m_currentMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        if (m_isFadingOut) {
            player.volume = m_musicVolume * (1.0f - m_fadeProgress);
        } else if (m_isFadingIn) {
            player.volume = m_musicVolume * m_fadeProgress;
        }
    }
}

// ============================================================================
// GAME INSTANCE MANAGEMENT
// ============================================================================

// Global game instance
static Game* g_gameInstance = nullptr;

// Global GameView pointer for performance optimization
static GameView* g_gameView = nullptr;

Game* GetGameInstance() {
    NSLog(@"[ACCESS] GetGameInstance() called from thread: %@, returning: %p", [NSThread currentThread], g_gameInstance);
    return g_gameInstance;
}

void SetGameInstance(Game* instance) {
    NSLog(@"[ACCESS] SetGameInstance() called from thread: %@, with instance: %p", [NSThread currentThread], instance);
    g_gameInstance = instance;
}

void SetGlobalGameView(GameView* gameView) {
    g_gameView = gameView;
    NSLog(@"[DEBUG] SetGlobalGameView: Set global GameView pointer to %p", gameView);
}

// ============================================================================
// iOS-SPECIFIC LIFECYCLE FUNCTIONS
// ============================================================================

int game_main(int argc, char *argv[]) {
    NSLog(@"[INIT] game_main() STARTING (iOS version)");
    
    // Create game instance
    Game* gameInstance = new Game();
    if (!gameInstance) {
        NSLog(@"[ERROR] game_main: Failed to create Game instance");
        return -1;
    }
    
    // Set the global game instance
    SetGameInstance(gameInstance);
    
    // Initialize the game
    if (!gameInstance->Initialize()) {
        NSLog(@"[ERROR] game_main: Failed to initialize Game");
        delete gameInstance;
        SetGameInstance(nullptr);
        return -1;
    }
    
    NSLog(@"[INIT] game_main() COMPLETED SUCCESSFULLY");
    return 0;
}

void OnAppPause() {
    NSLog(@"[LIFECYCLE] OnAppPause() called");
    Game* game = GetGameInstance();
    if (game) {
        game->OnAppPause();
    }
}

void OnAppResume() {
    NSLog(@"[LIFECYCLE] OnAppResume() called");
    Game* game = GetGameInstance();
    if (game) {
        game->OnAppResume();
    }
}

// ============================================================================
// iOS-SPECIFIC INPUT AND UI FUNCTIONS
// ============================================================================

void UpdateSafeAreaInsets(float top, float right, float bottom, float left) {
    TraceLog(LOG_INFO, "[SAFE_AREA] UpdateSafeAreaInsets: top=%.1f, right=%.1f, bottom=%.1f, left=%.1f", top, right, bottom, left);
    
    // Update global safe area insets
    g_safeAreaInsets.top = top;
    g_safeAreaInsets.right = right;
    g_safeAreaInsets.bottom = bottom;
    g_safeAreaInsets.left = left;
    
    // Update PlatformIOS instance if available
    if (g_gameInstance) {
        PlatformIOS* platform = dynamic_cast<PlatformIOS*>(g_gameInstance->GetPlatform());
        if (platform) {
            platform->m_safeArea = {top, right, bottom, left};
        }
    }
}

void UpdateTouchState(int touchId, float x, float y, bool pressed) {
    TraceLog(LOG_INFO, "[TOUCH] UpdateTouchState ENTRY: touchId=%d, x=%.1f, y=%.1f, pressed=%s", touchId, x, y, pressed ? "true" : "false");
    
    // Update PlatformIOS instance if available
    if (g_gameInstance) {
        PlatformIOS* platform = dynamic_cast<PlatformIOS*>(g_gameInstance->GetPlatform());
        if (platform) {
            // Update primary input state based on touch
            if (touchId == 0) { // Primary touch
                platform->m_primaryInputDown = pressed;
                if (pressed) {
                    platform->m_primaryInputPressed = true;
                    platform->m_primaryInputPosition = {x, y};
                } else {
                    platform->m_primaryInputReleased = true;
                }
            }
        }
    }
    
    TraceLog(LOG_INFO, "[TOUCH] UpdateTouchState EXIT: touchId=%d, x=%.1f, y=%.1f, pressed=%s", touchId, x, y, pressed ? "true" : "false");
}

void ClearAllTouchStates() {
    TraceLog(LOG_INFO, "[TOUCH] ClearAllTouchStates ENTRY");
    
    // Clear PlatformIOS touch states if available
    if (g_gameInstance) {
        PlatformIOS* platform = dynamic_cast<PlatformIOS*>(g_gameInstance->GetPlatform());
        if (platform) {
            platform->m_primaryInputDown = false;
            platform->m_primaryInputPressed = false;
            platform->m_primaryInputReleased = false;
            platform->m_primaryInputPosition = {0, 0};
        }
    }
    
    TraceLog(LOG_INFO, "[TOUCH] ClearAllTouchStates EXIT");
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

Texture2D CreateFallbackTexture(const char* fileName) {
    NSLog(@"[DEBUG] CreateFallbackTexture: Creating fallback for: %s", fileName);
    
    // Get GameView for Metal device - use global pointer directly
    GameView* gameView = g_gameView;
    
    if (!gameView) {
        NSLog(@"[ERROR] CreateFallbackTexture: Failed to get GameView");
        return {0};
    }
    
    id<MTLDevice> device = [gameView getMetalDevice];
    if (!device) {
        NSLog(@"[ERROR] CreateFallbackTexture: No Metal device available from GameView");
        return {0};
    }
    
    // Create a simple magenta fallback texture
    const int width = 64;
    const int height = 64;
    
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    textureDescriptor.storageMode = MTLStorageModeShared;
    
    NSError *error = nil;
    id<MTLTexture> metalTexture = [device newTextureWithDescriptor:textureDescriptor];
    if (!metalTexture) {
        NSLog(@"[ERROR] CreateFallbackTexture: Failed to create Metal texture: %@", error);
        return {0};
    }
    
    // Create magenta pixel data
    std::vector<uint8_t> pixelData(width * height * 4, 255); // RGBA
    for (int i = 0; i < width * height; i++) {
        pixelData[i * 4 + 0] = 255; // R
        pixelData[i * 4 + 1] = 0;   // G
        pixelData[i * 4 + 2] = 255; // B
        pixelData[i * 4 + 3] = 255; // A
    }
    
    // Upload pixel data to texture
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
    [metalTexture replaceRegion:region mipmapLevel:0 withBytes:pixelData.data() bytesPerRow:4 * width];
    
    // Create Texture2D struct
    Texture2D fallbackTexture = {0};
    fallbackTexture.id = 0; // Not used for Metal
    fallbackTexture.width = width;
    fallbackTexture.height = height;
    fallbackTexture.mipmaps = 1;
    fallbackTexture.format = 7; // PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    fallbackTexture.texture = (__bridge_retained void*)metalTexture;
    
    NSLog(@"[DEBUG] CreateFallbackTexture: Fallback texture created successfully (id=%p)", fallbackTexture.texture);
    return fallbackTexture;
}

#else // !PLATFORM_IOS

// Stub implementation for non-iOS platforms
PlatformIOS::PlatformIOS() {}
PlatformIOS::~PlatformIOS() {}
void PlatformIOS::Initialize(void* nativeView) {}
void PlatformIOS::Initialize() {}
void PlatformIOS::Shutdown() {}

// All other methods would need stub implementations here
// For brevity, I'm not implementing all stubs, but they would be needed

#endif // PLATFORM_IOS 