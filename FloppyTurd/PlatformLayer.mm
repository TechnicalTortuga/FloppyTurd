#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <AVFoundation/AVFoundation.h>
#import "PlatformLayer.h"
#import "RaylibCompat.h"
#import "MetalRenderer.h"
#import "PlatformLayerDelegate.h"
#import "GameViewController.h"
#import "GameView.h"
#import "UIManager.h"
#import "MetalTextureCache.h"
#import "UICoordinateSystem.h"
#import "ResourceManager.h"
#import "TouchControls.h"

// Singleton instance
static PlatformLayer* s_Instance = nullptr;

// ColorToUInt is now defined in RaylibCompat.h

// Error logging for Metal operations
static void LogMetalError(NSError *error, NSString *operation) {
    if (error) {
        TraceLog(LOG_ERROR, "[ERROR] Metal operation failed: %s - Error: %s", [operation UTF8String], [error.localizedDescription UTF8String]);
    }
}

PlatformLayer& PlatformLayer::GetInstance() {
    if (!s_Instance) {
        s_Instance = new PlatformLayer();
    }
    return *s_Instance;
}

PlatformLayer::PlatformLayer() : m_View(nullptr), m_PrimaryInputDown(false), m_PrimaryInputPressed(false), m_PrimaryInputReleased(false), m_SecondaryInputDown(false), m_SecondaryInputPressed(false), m_SecondaryInputReleased(false) {
    TraceLog(LOG_INFO, "[DEBUG] PlatformLayer constructor starting");
    // Metal device management moved to GameView
    TraceLog(LOG_INFO, "[DEBUG] PlatformLayer constructor completed");
}

PlatformLayer::~PlatformLayer() {
    // Clean up Metal resources - ARC will handle the releases automatically
    // Removed static variable initializations as they are managed elsewhere
}

void PlatformLayer::Initialize(void* nativeView) {
    TraceLog(LOG_INFO, "[INIT] PlatformLayer::Initialize(void* nativeView) STARTING");
    TraceLog(LOG_INFO, "[INIT] nativeView: %p", nativeView);
    
    if (!nativeView) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::Initialize: nativeView is null");
        return;
    }
    
    UIView* view = (__bridge UIView*)nativeView;
    if (![view isKindOfClass:[UIView class]]) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::Initialize: nativeView is null or invalid UIView");
        return;
    }
    
    TraceLog(LOG_INFO, "[INIT] Got UIView (GameView): %p", view);
    m_View = nativeView;
    
    // GameView handles its own setup
    TraceLog(LOG_INFO, "[INIT] GameView handles its own properties setup");
    
    // No need to create PlatformLayerDelegate as GameView handles rendering and touch events
    TraceLog(LOG_INFO, "[INIT] Skipping PlatformLayerDelegate creation as GameView handles rendering");
    m_Delegate = nullptr;
    
    // Enable automatic drawing
    if ([view isKindOfClass:[MTKView class]]) {
        ((MTKView*)view).paused = NO;
    }
    
    m_TouchPoints.clear();
    TraceLog(LOG_INFO, "[INIT] ========================================");
    TraceLog(LOG_INFO, "[INIT] PlatformLayer::Initialize(void* nativeView) COMPLETED");
    TraceLog(LOG_INFO, "[INIT] m_View=%p, m_Delegate=%p", m_View, m_Delegate);
    TraceLog(LOG_INFO, "[INIT] ========================================");
}

void PlatformLayer::Initialize(void* nativeView, void* gameViewController, void* metalRenderer) {
    TraceLog(LOG_INFO, "[INIT] PlatformLayer::Initialize(void* nativeView, void* gameViewController, void* metalRenderer) STARTING");
    TraceLog(LOG_INFO, "[INIT] nativeView: %p, gameViewController: %p, metalRenderer: %p", nativeView, gameViewController, metalRenderer);
    
    if (!nativeView) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::Initialize: nativeView is null");
        return;
    }
    
    if (!gameViewController) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::Initialize: gameViewController is null");
        return;
    }
    
    UIView* view = (__bridge UIView*)nativeView;
    GameViewController* gvc = (__bridge GameViewController*)gameViewController;
    
    if (![view isKindOfClass:[UIView class]]) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::Initialize: nativeView is null or invalid UIView");
        return;
    }
    
    if (![gvc isKindOfClass:[GameViewController class]]) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::Initialize: gameViewController is null or invalid GameViewController");
        return;
    }
    
    TraceLog(LOG_INFO, "[INIT] Got UIView (GameView): %p, GameViewController: %p", view, gvc);
    m_View = nativeView;
    
    // GameView handles its own setup
    TraceLog(LOG_INFO, "[INIT] GameView handles its own properties setup");
    
    // No need to create PlatformLayerDelegate as GameView handles rendering and touch events
    TraceLog(LOG_INFO, "[INIT] Skipping PlatformLayerDelegate creation as GameView handles rendering");
    m_Delegate = nullptr;
    
    // Enable automatic drawing
    if ([view isKindOfClass:[MTKView class]]) {
        ((MTKView*)view).paused = NO;
    }
    
    m_TouchPoints.clear();
    TraceLog(LOG_INFO, "[INIT] ========================================");
    TraceLog(LOG_INFO, "[INIT] PlatformLayer::Initialize(void* nativeView, void* gameViewController, void* metalRenderer) COMPLETED");
    TraceLog(LOG_INFO, "[INIT] m_View=%p, m_Delegate=%p", m_View, m_Delegate);
    TraceLog(LOG_INFO, "[INIT] ========================================");
}

void PlatformLayer::Initialize() {
    // Implementation for initialization if needed beyond what's in Initialize(void*)
}

void PlatformLayer::Shutdown() {
    // Clean up MetalTextureCache
    MetalTextureCache::GetInstance().Shutdown();
    
    // Clean up resources
    m_Delegate = nullptr;
    m_View = nullptr;
    
    TraceLog(LOG_INFO, "[SHUTDOWN] PlatformLayer shutdown complete");
}

// App lifecycle events
void PlatformLayer::OnAppWillResignActive() {
    // Handle app becoming inactive
}

void PlatformLayer::OnAppDidBecomeActive() {
    // Handle app becoming active
}

// File system helpers
std::string PlatformLayer::GetResourcePath(const std::string& relativePath) {
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

std::string PlatformLayer::GetSavePath(const std::string& filename) {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDirectory = [paths objectAtIndex:0];
    NSString* fullPath = [documentsDirectory stringByAppendingPathComponent:[NSString stringWithUTF8String:filename.c_str()]];
    return std::string([fullPath UTF8String]);
}

// Input handling
bool PlatformLayer::IsTouchSupported() const {
    return true; // iOS always supports touch
}

bool PlatformLayer::IsMobilePlatform() const {
    return true;
}

// Touch state is now managed by TouchControls (single source of truth)

void PlatformLayer::UpdateTouchState() {
    TraceLog(LOG_INFO, "[TOUCH] PlatformLayer::UpdateTouchState ENTRY");
    
    if (!m_View) {
        TraceLog(LOG_WARNING, "[TOUCH] PlatformLayer::UpdateTouchState: No view available");
        return;
    }
    
#ifdef PLATFORM_MOBILE
    // Query GameView for touch states
    UIView* view = (__bridge UIView*)m_View;
    if ([view respondsToSelector:@selector(isPrimaryTouchDown)] &&
        [view respondsToSelector:@selector(getPrimaryTouchLocation)] &&
        [view respondsToSelector:@selector(getActiveTouchCount)]) {
        
        // Functionality for touch input moved to GameView
        m_PrimaryInputDown = NO;
        m_PrimaryInputPressed = NO;
        m_PrimaryInputReleased = !m_PrimaryInputDown && m_PrevPrimaryInputDown;
        m_PrevPrimaryInputDown = m_PrimaryInputDown;
        
        NSInteger touchCount = [(id)view getActiveTouchCount];
        m_TouchPoints.clear();
        if (touchCount > 0) {
            CGPoint location = [(id)view getPrimaryTouchLocation];
            m_TouchPoints.push_back(Vector2{(float)location.x, (float)location.y});
        }
        
        // Update TouchControls with the latest state from GameView
        m_TouchControls.UpdateGestureDetection();
    } else {
        TraceLog(LOG_WARNING, "[TOUCH] PlatformLayer::UpdateTouchState: View does not respond to touch state selectors");
        m_PrimaryInputDown = false;
        m_PrimaryInputPressed = false;
        m_PrimaryInputReleased = false;
        m_TouchPoints.clear();
    }
#endif
    
    TraceLog(LOG_INFO, "[TOUCH] PlatformLayer::UpdateTouchState: final states - down=%s, pressed=%s, released=%s, touchCount=%zu",
             m_PrimaryInputDown ? "true" : "false",
             m_PrimaryInputPressed ? "true" : "false",
             m_PrimaryInputReleased ? "true" : "false",
             m_TouchPoints.size());
    
    TraceLog(LOG_INFO, "[TOUCH] PlatformLayer::UpdateTouchState EXIT");
}

// New function to clear touch states after render phase
void PlatformLayer::ClearTouchStatesAfterRender() {
    TraceLog(LOG_INFO, "[TOUCH] PlatformLayer::ClearTouchStatesAfterRender ENTRY");
    
    // Touch states are now managed internally by TouchControls
    // No need to call external clear function
    
    TraceLog(LOG_INFO, "[TOUCH] PlatformLayer::ClearTouchStatesAfterRender EXIT");
}

// Static function to update touch state from external sources
void PlatformLayer::SetTouchState(bool pressed, float x, float y) {
    TraceLog(LOG_INFO, "[TOUCH] PlatformLayer::SetTouchState ENTRY: pressed=%s, pos=(%.1f,%.1f)",
             pressed ? "true" : "false", x, y);
    
#ifdef PLATFORM_MOBILE
    PlatformLayer& instance = PlatformLayer::GetInstance();
    
    // Update TouchControls instance
    instance.m_TouchControls.SetTouchState(pressed, x, y);
    
    // GameView handles primary touch state updates internally, so we don't update PlatformLayer's variables directly
    // Instead, we'll query GameView in UpdateTouchState
#endif
    
    TraceLog(LOG_INFO, "[TOUCH] PlatformLayer::SetTouchState EXIT");
}

// Static function to clear all touch states
void PlatformLayer::ClearAllTouchStates() {
    TraceLog(LOG_INFO, "[TOUCH] PlatformLayer::ClearAllTouchStates ENTRY");
    
#ifdef PLATFORM_MOBILE
    PlatformLayer& instance = PlatformLayer::GetInstance();
    
    // Clear TouchControls instance
    instance.m_TouchControls.ClearAllTouchStates();
    
    // GameView handles touch state clearing internally
#endif
    
    TraceLog(LOG_INFO, "[TOUCH] PlatformLayer::ClearAllTouchStates EXIT");
}

int PlatformLayer::GetTouchCount() const {
    return static_cast<int>(m_TouchPoints.size());
}

Vector2 PlatformLayer::GetTouchPosition(int index) const {
    if (index >= 0 && index < m_TouchPoints.size()) {
        Vector2 touchPoint = m_TouchPoints[index];
        // Convert from points to pixels by multiplying by screen scale
        float scale = UIScreen.mainScreen.scale;
        Vector2 pixelPos = Vector2{touchPoint.x * scale, touchPoint.y * scale};
        return pixelPos;
    }
    
    return Vector2{0, 0};
}

std::vector<Vector2> PlatformLayer::GetTouchPoints() const {
    return m_TouchPoints;
}

Vector2 PlatformLayer::GetPrimaryInputPosition() const {
    return m_TouchPoints.size() > 0 ? m_TouchPoints[0] : Vector2{0, 0};
}

bool PlatformLayer::IsPrimaryInputDown() const {
    return m_PrimaryInputDown;
}

bool PlatformLayer::IsPrimaryInputPressed() const {
    return m_PrimaryInputPressed;
}

bool PlatformLayer::IsPrimaryInputReleased() const {
    return m_PrimaryInputReleased;
}

bool PlatformLayer::IsSecondaryInputDown() const {
    return m_SecondaryInputDown;
}

bool PlatformLayer::IsSecondaryInputPressed() const {
    return m_SecondaryInputPressed;
}

bool PlatformLayer::IsSecondaryInputReleased() const {
    return m_SecondaryInputReleased;
}

// Screen/Window management
float PlatformLayer::GetScreenDensity() const {
    return UIScreen.mainScreen.scale;
}

void PlatformLayer::SetOrientation(bool landscape) {
    // iOS orientation handling if needed
}

bool PlatformLayer::SupportsFullscreen() const {
    return true; // iOS apps are always fullscreen
}

int PlatformLayer::GetScreenWidth() const {
    return static_cast<int>(UIScreen.mainScreen.bounds.size.width * UIScreen.mainScreen.scale);
}

int PlatformLayer::GetScreenHeight() const {
    return static_cast<int>(UIScreen.mainScreen.bounds.size.height * UIScreen.mainScreen.scale);
}

double PlatformLayer::GetCurrentTime() const {
    return CACurrentMediaTime();
}

// Platform-specific features
void PlatformLayer::ShowVirtualKeyboard(bool show) {
#ifdef PLATFORM_MOBILE
    virtualKeyboardShown = show;
    if (show) {
        UIWindow* window = UIApplication.sharedApplication.windows.firstObject;
        UIViewController* rootVC = window.rootViewController;
        UITextField* dummyTextField = [[UITextField alloc] init];
        [rootVC.view addSubview:dummyTextField];
        [dummyTextField becomeFirstResponder];
        [dummyTextField removeFromSuperview];
    } else {
        UIWindow* window = UIApplication.sharedApplication.windows.firstObject;
        [window endEditing:YES];
    }
#endif
}

bool PlatformLayer::IsVirtualKeyboardShown() const {
#ifdef PLATFORM_MOBILE
    return virtualKeyboardShown;
#else
    return false;
#endif
}

void PlatformLayer::Vibrate(int milliseconds) {
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
}

// Safe area handling
Rectangle PlatformLayer::GetSafeArea() const {
    Rectangle safeArea = {0, 0, 0, 0};
    if (@available(iOS 11.0, *)) {
        UIWindow* window = UIApplication.sharedApplication.windows.firstObject;
        safeArea.x = window.safeAreaInsets.left;
        safeArea.y = window.safeAreaInsets.top;
        safeArea.width = window.bounds.size.width - (window.safeAreaInsets.left + window.safeAreaInsets.right);
        safeArea.height = window.bounds.size.height - (window.safeAreaInsets.top + window.safeAreaInsets.bottom);
    } else {
        UIWindow* window = UIApplication.sharedApplication.windows.firstObject;
        safeArea.x = 0;
        safeArea.y = 20; // Status bar height
        safeArea.width = window.bounds.size.width;
        safeArea.height = window.bounds.size.height - 20;
    }
    return safeArea;
}

// Performance hints
bool PlatformLayer::PreferLowPowerMode() const {
    if (@available(iOS 9.0, *)) {
        return NSProcessInfo.processInfo.isLowPowerModeEnabled;
    }
    return false;
}

int PlatformLayer::GetRecommendedTextureSize() const {
    return 2048; // Reasonable default for modern iOS devices
}

// UI and System
void PlatformLayer::SetWindowTitle(const std::string& title) {
    // Not applicable on iOS
}

void PlatformLayer::SetWindowSize(int width, int height) {
    // Not applicable on iOS
}

bool PlatformLayer::IsWindowFullscreen() const {
    return true; // iOS apps are always fullscreen
}

void PlatformLayer::ToggleFullscreen() {
    // Not applicable on iOS
}

float PlatformLayer::GetScreenScale() const {
    return UIScreen.mainScreen.scale;
}

#ifdef PLATFORM_IOS
// Metal rendering interface
MetalRenderer* PlatformLayer::GetMetalRenderer() const {
    // We're not using MetalRenderer anymore, we're using PlatformLayerDelegate
    // Return nullptr to indicate this method is deprecated
    return nullptr;
}

// Texture and image handling
void* PlatformLayer::LoadTexture(const char* fileName, int* width, int* height) {
    TraceLog(LOG_INFO, "[DEBUG] LoadTexture called: fileName=%s", fileName);
    
    // Handle asset catalog resources
    std::string filePath(fileName);
    if (filePath.substr(0, 8) == "asset://") {
        ResourcePathParts parts = ResourceManager::ParseResourcePath(filePath);
        NSString* name = [NSString stringWithUTF8String:parts.baseName.c_str()];
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Loading asset catalog texture: %@", name);
        UIImage* uiImage = [UIImage imageNamed:name];
        
        if (!uiImage) {
            TraceLog(LOG_ERROR, "[ERROR] LoadTexture: Failed to load asset catalog texture: %s", fileName);
            return nullptr;
        }
        
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Successfully loaded UIImage for %@", name);
        
        CGImageRef cgImage = uiImage.CGImage;
        size_t cgWidth = CGImageGetWidth(cgImage);
        size_t cgHeight = CGImageGetHeight(cgImage);
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: CGImage dimensions: %zux%zu", cgWidth, cgHeight);
        
        // Validate CGImage dimensions
        if (cgWidth == 0 || cgHeight == 0) {
            TraceLog(LOG_ERROR, "[ERROR] LoadTexture: CGImage has invalid dimensions!");
            return nullptr;
        }
        
        // Validate width/height pointers
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: About to validate pointers - width: %p, height: %p", width, height);
        if (!width) {
            TraceLog(LOG_ERROR, "[ERROR] LoadTexture: Width pointer is null!");
            return nullptr;
        }
        if (!height) {
            TraceLog(LOG_ERROR, "[ERROR] LoadTexture: Height pointer is null!");
            return nullptr;
        }
        
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Pointers validated, about to assign dimensions");
        *width = (int)cgWidth;
        *height = (int)cgHeight;
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Final image dimensions: %dx%d", *width, *height);
        
        // Create Metal texture
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Creating Metal texture descriptor");
        MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Metal texture descriptor created: %p", textureDescriptor);
        textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        textureDescriptor.width = *width;
        textureDescriptor.height = *height;
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Metal texture descriptor configured");
        // Get Metal device from GameView instead of local storage
        UIView* view = (__bridge UIView*)m_View;
        if (![view isKindOfClass:[GameView class]]) {
            TraceLog(LOG_ERROR, "[ERROR] LoadTexture: View is not a GameView");
            return nullptr;
        }
        
        GameView* gameView = (GameView*)view;
        id<MTLDevice> device = [gameView getMetalDevice];
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Creating Metal texture with device: %p", device);
        NSError *error = nil;
        id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
        LogMetalError(error, @"Creating Metal texture");
        if (!texture) {
                    TraceLog(LOG_ERROR, "[ERROR] LoadTexture: Failed to create Metal texture!");
        return nullptr;
    }
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Metal texture created: %p (retain count: %lu)", texture, (unsigned long)CFGetRetainCount((__bridge CFTypeRef)texture));
        
        // Load image data into texture
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Creating color space");
        MTLRegion region = {{0, 0, 0}, {(NSUInteger)*width, (NSUInteger)*height, 1}};
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Color space created: %p", colorSpace);
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Creating bitmap context");
        CGContextRef context = CGBitmapContextCreate(nil, *width, *height, 8, 0, colorSpace, kCGImageAlphaPremultipliedLast);
        if (!context) {
            TraceLog(LOG_ERROR, "[ERROR] LoadTexture: Failed to create bitmap context!");
            CGColorSpaceRelease(colorSpace);
            return nullptr;
        }
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Bitmap context created: %p", context);
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Drawing image into context");
        CGContextDrawImage(context, CGRectMake(0, 0, *width, *height), cgImage);
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Image drawn into context");
        void* imageData = CGBitmapContextGetData(context);
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Image data pointer: %p", imageData);
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Replacing Metal texture region");
        [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * *width];
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Metal texture region replaced");
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        
        TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Texture loading completed successfully");
        return (__bridge_retained void*)texture;
    }
    
    // Handle regular file paths
    NSString* path = [NSString stringWithUTF8String:fileName];
    NSString* fullPath = [[NSBundle mainBundle] pathForResource:path ofType:nil];
    if (!fullPath) {
        fullPath = [NSString stringWithUTF8String:fileName];
    }
    UIImage* uiImage = [UIImage imageWithContentsOfFile:fullPath];
    if (!uiImage) {
        TraceLog(LOG_ERROR, "[ERROR] LoadTexture: Failed to load image: %s", fileName);
        return nullptr;
    }
    
    CGImageRef cgImage = uiImage.CGImage;
    *width = (int)CGImageGetWidth(cgImage);
    *height = (int)CGImageGetHeight(cgImage);
    
    // Create Metal texture
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = *width;
    textureDescriptor.height = *height;
    
    // Get Metal device from GameView instead of local storage
    UIView* view = (__bridge UIView*)m_View;
    if (![view isKindOfClass:[GameView class]]) {
        TraceLog(LOG_ERROR, "[ERROR] LoadTexture: View is not a GameView");
        return nullptr;
    }
    
    GameView* gameView = (GameView*)view;
    id<MTLDevice> device = [gameView getMetalDevice];
    id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
    if (!texture) {
        TraceLog(LOG_ERROR, "[ERROR] LoadTexture: Failed to create Metal texture!");
        return nullptr;
    }
    TraceLog(LOG_INFO, "[DEBUG] LoadTexture: Regular file texture created: %p (retain count: %lu)", texture, (unsigned long)CFGetRetainCount((__bridge CFTypeRef)texture));
    
    // Load image data into texture
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)*width, (NSUInteger)*height, 1}};
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(nil, *width, *height, 8, 0, colorSpace, kCGImageAlphaPremultipliedLast);
    if (!context) {
        TraceLog(LOG_ERROR, "[ERROR] LoadTexture: Failed to create bitmap context!");
        CGColorSpaceRelease(colorSpace);
        return nullptr;
    }
    CGContextDrawImage(context, CGRectMake(0, 0, *width, *height), cgImage);
    void* imageData = CGBitmapContextGetData(context);
    [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * *width];
    
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    
    return (__bridge_retained void*)texture;
}

void PlatformLayer::UnloadTexture(void* texture) {
    if (texture) {
        id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture;
        TraceLog(LOG_INFO, "[DEBUG] UnloadTexture: Releasing texture: %p", metalTexture);
        
        // Release the bridged texture
        CFBridgingRelease(texture);
        TraceLog(LOG_INFO, "[DEBUG] UnloadTexture: Bridge released for texture: %p", metalTexture);
    }
}

void* PlatformLayer::LoadRenderTexture(int width, int height) {
    TraceLog(LOG_INFO, "[DEBUG] LoadRenderTexture: Starting with width=%d, height=%d", width, height);
    
    // Get Metal device from GameView instead of local storage
    UIView* view = (__bridge UIView*)m_View;
    if (![view isKindOfClass:[GameView class]]) {
        TraceLog(LOG_ERROR, "[ERROR] LoadRenderTexture: View is not a GameView");
        return nullptr;
    }
    
    GameView* gameView = (GameView*)view;
    id<MTLDevice> device = [gameView getMetalDevice];
    TraceLog(LOG_INFO, "[DEBUG] LoadRenderTexture: Metal device: %p", device);
    
    if (!device) {
        TraceLog(LOG_ERROR, "[ERROR] LoadRenderTexture: No Metal device available!");
        return nullptr;
    }
    
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    
    TraceLog(LOG_INFO, "[DEBUG] LoadRenderTexture: Creating Metal texture with descriptor...");
    id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
    
    if (!texture) {
        TraceLog(LOG_ERROR, "[ERROR] LoadRenderTexture: Failed to create Metal render texture!");
        return nullptr;
    }
    
    TraceLog(LOG_INFO, "[DEBUG] LoadRenderTexture: Successfully created Metal texture: %p (w=%lu, h=%lu)", 
          texture, (unsigned long)texture.width, (unsigned long)texture.height);
    
    return (__bridge_retained void*)texture;
}

void PlatformLayer::BeginDrawing(void* renderTexture) {
    // GameView handles rendering setup internally
    UIView* gameView = (__bridge UIView*)m_View;
    if (!gameView) {
        TraceLog(LOG_ERROR, "[ERROR] BeginDrawing: Could not get GameView from stored view!");
        return;
    }

#if defined(PLATFORM_MOBILE)
    // --- MOBILE RENDERING PATH ---
    // Use UICoordinateSystem to get the actual pixel dimensions for the projection matrix.
    Rectangle pixelScreenRect = UICoordinateSystem::GetPixelScreenRect();
    
    // GameView handles drawable size and scale internally
    // Notify GameView to render
    if ([gameView respondsToSelector:@selector(render)]) {
        [(id)gameView render];
    } else {
        TraceLog(LOG_ERROR, "[ERROR] BeginDrawing: GameView does not respond to render selector!");
    }
#else
    // --- DESKTOP RENDERING PATH ---
    // For desktop, we still use the fixed 320x180 resolution
    // GameView handles rendering internally
    if ([gameView respondsToSelector:@selector(render)]) {
        [(id)gameView render];
    } else {
        TraceLog(LOG_ERROR, "[ERROR] BeginDrawing: GameView does not respond to render selector!");
    }
#endif
}

void PlatformLayer::EndDrawing(void* renderTexture) {
    // End the current render pass if needed
    // For simplicity, we rely on the MTKView delegate to handle rendering commits
}

void PlatformLayer::DrawRectangle(int posX, int posY, int width, int height, unsigned int color) {
    TraceLog(LOG_INFO, "[DEBUG] PlatformLayer::DrawRectangle called: posX=%d, posY=%d, width=%d, height=%d, color=0x%08X", posX, posY, width, height, color);
    
    // Ensure we're on the main thread
    if (![NSThread isMainThread]) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawRectangle called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            this->DrawRectangle(posX, posY, width, height, color);
        });
        return;
    }
    
    // Use global MetalRenderer if available
    if (g_metalRenderer) {
        Color raylibColor = {
            (unsigned char)((color >> 24) & 0xFF),
            (unsigned char)((color >> 16) & 0xFF),
            (unsigned char)((color >> 8) & 0xFF),
            (unsigned char)(color & 0xFF)
        };
        g_metalRenderer->DrawRectangle(posX, posY, width, height, raylibColor);
    } else {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawRectangle: Global MetalRenderer not available");
    }
}

void PlatformLayer::DrawTexture(void* texture, float x, float y, float width, float height, Color tint) {
    TraceLog(LOG_INFO, "[DEBUG] PlatformLayer::DrawTexture called: texture=%p, x=%.2f, y=%.2f, width=%.2f, height=%.2f", texture, x, y, width, height);
    // Ensure we're on the main thread
    if (![NSThread isMainThread]) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawTexture called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            this->DrawTexture(texture, x, y, width, height, tint);
        });
        return;
    }
    // Validate texture
    if (!texture) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawTexture: Invalid texture pointer");
        return;
    }
    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture;
    if (!metalTexture) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawTexture: Failed to bridge texture pointer");
        return;
    }
    // Check if texture is still valid
    if (metalTexture.width == 0 || metalTexture.height == 0) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawTexture: Texture has invalid dimensions (w=%lu, h=%lu)", (unsigned long)metalTexture.width, (unsigned long)metalTexture.height);
        return;
    }
    TraceLog(LOG_INFO, "[DEBUG] PlatformLayer::DrawTexture: Valid texture found (ptr=%p, w=%lu, h=%lu)", metalTexture, (unsigned long)metalTexture.width, (unsigned long)metalTexture.height);
    
    // Use global MetalRenderer if available
    if (g_metalRenderer) {
        Rectangle source = {0, 0, (float)metalTexture.width, (float)metalTexture.height};
        Rectangle dest = {x, y, width, height};
        g_metalRenderer->DrawTexture(metalTexture, source, dest, tint);
    } else {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawTexture: Global MetalRenderer not available");
    }
}

void PlatformLayer::EnqueueDrawCommand(void* vertexBuffer, void* texture, size_t vertexCount) {
    // This function is now deprecated in favor of direct drawing
    // Keeping for compatibility but it's no longer used
}

// Metal device management methods removed - now handled by GameView

void* PlatformLayer::GetDelegate() const {
    return m_Delegate;
}

void* PlatformLayer::LoadTextureFromImage(void* imageData, int width, int height, int format) {
    if (!imageData || width <= 0 || height <= 0) {
        TraceLog(LOG_ERROR, "[ERROR] LoadTextureFromImage: Invalid parameters - imageData=%p, width=%d, height=%d", imageData, width, height);
        return nullptr;
    }
    
    // Create Metal texture
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    
    // Get Metal device from GameView instead of local storage
    UIView* view = (__bridge UIView*)m_View;
    if (![view isKindOfClass:[GameView class]]) {
        TraceLog(LOG_ERROR, "[ERROR] LoadTextureFromImage: View is not a GameView");
        return nullptr;
    }
    
    GameView* gameView = (GameView*)view;
    id<MTLDevice> device = [gameView getMetalDevice];
    id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
    if (!texture) {
        TraceLog(LOG_ERROR, "[ERROR] LoadTextureFromImage: Failed to create Metal texture!");
        return nullptr;
    }
    
    // Upload the image data
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
    [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
    
    TraceLog(LOG_INFO, "[DEBUG] LoadTextureFromImage: Created texture from image data - %dx%d, format=%d", width, height, format);
    
    return (__bridge_retained void*)texture;
}

void PlatformLayer::DrawText(const char* text, float x, float y, float fontSize, Color color, void* font) {
    TraceLog(LOG_INFO, "[DEBUG] DrawText called: text='%s', pos=(%.1f, %.1f), fontSize=%.1f, color=(%d,%d,%d,%d)", 
          text, x, y, fontSize, color.r, color.g, color.b, color.a);
    
    // Use global MetalRenderer if available
    if (g_metalRenderer) {
        if (font) {
            Font* fontPtr = (Font*)font;
            g_metalRenderer->DrawText(text, x, y, fontSize, color, fontPtr);
        } else {
            g_metalRenderer->DrawText(text, x, y, fontSize, color);
        }
    } else {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawText: Global MetalRenderer not available");
    }
}

float PlatformLayer::GetLastFrameTime() const {
    if (g_metalRenderer) {
        return g_metalRenderer->GetDebugStats().frameTime;
    }
    return 0.0f;
}

int PlatformLayer::GetLastFPS() const {
    if (g_metalRenderer) {
        float frameTime = g_metalRenderer->GetDebugStats().frameTime;
        if (frameTime > 0.0f) {
            return (int)(1.0f / frameTime + 0.5f);
        }
    }
    return 0;
}

void PlatformLayer::DrawLineEx(float x1, float y1, float x2, float y2, float thickness, Color color) {
    TraceLog(LOG_INFO, "[DEBUG] PlatformLayer::DrawLineEx called: (%.1f,%.1f) to (%.1f,%.1f), thickness=%.1f, color=(%d,%d,%d,%d)", 
          x1, y1, x2, y2, thickness, color.r, color.g, color.b, color.a);
    
    // Ensure we're on the main thread
    if (![NSThread isMainThread]) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawLineEx called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            this->DrawLineEx(x1, y1, x2, y2, thickness, color);
        });
        return;
    }
    
    // Use global MetalRenderer if available
    if (g_metalRenderer) {
        g_metalRenderer->DrawLineEx(x1, y1, x2, y2, thickness, color);
    } else {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawLineEx: Global MetalRenderer not available");
    }
}

void PlatformLayer::DrawRectangleRoundedLines(float x, float y, float width, float height, float roundness, int segments, float lineThick, Color color) {
    TraceLog(LOG_INFO, "[DEBUG] PlatformLayer::DrawRectangleRoundedLines called: rect=(%.1f,%.1f,%.1f,%.1f), roundness=%.1f, lineThick=%.1f, color=(%d,%d,%d,%d)", 
          x, y, width, height, roundness, lineThick, color.r, color.g, color.b, color.a);
    
    // Ensure we're on the main thread
    if (![NSThread isMainThread]) {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawRectangleRoundedLines called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            this->DrawRectangleRoundedLines(x, y, width, height, roundness, segments, lineThick, color);
        });
        return;
    }
    
    // Use global MetalRenderer if available
    if (g_metalRenderer) {
        g_metalRenderer->DrawRectangleRoundedLines(x, y, width, height, roundness, segments, lineThick, color);
    } else {
        TraceLog(LOG_ERROR, "[ERROR] PlatformLayer::DrawRectangleRoundedLines: Global MetalRenderer not available");
    }
}

#endif // PLATFORM_IOS
