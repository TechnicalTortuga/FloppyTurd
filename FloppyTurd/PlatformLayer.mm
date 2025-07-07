#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <AVFoundation/AVFoundation.h>
#import "PlatformLayer.h"
#import "RaylibCompat.h"
#import "MetalRenderer.h"
#import "PlatformLayerDelegate.h"
#import "UIManager.h"

// Singleton instance
static PlatformLayer* s_Instance = nullptr;

// Metal rendering state
static id<MTLCommandQueue> s_CommandQueue = nil;
static id<MTLRenderCommandEncoder> s_CurrentRenderEncoder = nil;
static id<MTLBuffer> s_VertexBuffer = nil;
static id<MTLRenderPipelineState> s_PipelineState = nil;
static id<MTLTexture> s_CurrentTexture = nil;

// Helper to convert Color to unsigned int (RGBA)
static inline unsigned int ColorToUInt(Color c) {
    return ((unsigned int)c.r << 24) | ((unsigned int)c.g << 16) | ((unsigned int)c.b << 8) | ((unsigned int)c.a);
}

// Error logging for Metal operations
static void LogMetalError(NSError *error, NSString *operation) {
    if (error) {
        NSLog(@"[ERROR] Metal operation failed: %@ - Error: %@", operation, error.localizedDescription);
    }
}

PlatformLayer& PlatformLayer::GetInstance() {
    if (!s_Instance) {
        s_Instance = new PlatformLayer();
    }
    return *s_Instance;
}

PlatformLayer::PlatformLayer() : m_View(nullptr), m_PrimaryInputDown(false), m_PrimaryInputPressed(false), m_PrimaryInputReleased(false), m_SecondaryInputDown(false), m_SecondaryInputPressed(false), m_SecondaryInputReleased(false) {
    NSLog(@"[DEBUG] PlatformLayer constructor starting");
    // Initialize Metal device
    m_MetalDevice = (__bridge_retained void*)MTLCreateSystemDefaultDevice();
    NSLog(@"[DEBUG] PlatformLayer: Metal device created: %p", m_MetalDevice);
    
    // Create command queue
    if (m_MetalDevice) {
        id<MTLDevice> device = (__bridge id<MTLDevice>)m_MetalDevice;
        s_CommandQueue = [device newCommandQueue];
        if (!s_CommandQueue) {
            NSLog(@"[ERROR] PlatformLayer: Failed to create command queue!");
        }
        NSLog(@"[DEBUG] PlatformLayer: Command queue created: %p", s_CommandQueue);
    } else {
        NSLog(@"[ERROR] PlatformLayer: Failed to create Metal device!");
    }
    NSLog(@"[DEBUG] PlatformLayer constructor completed");
}

PlatformLayer::~PlatformLayer() {
    // Clean up Metal resources - ARC will handle the releases automatically
    s_CommandQueue = nil;
    s_VertexBuffer = nil;
    s_PipelineState = nil;
}

void PlatformLayer::Initialize(void* nativeView) {
    NSLog(@"[INIT] ========================================");
    NSLog(@"[INIT] PlatformLayer::Initialize(void* nativeView) STARTING");
    NSLog(@"[INIT] nativeView=%p", nativeView);
    NSLog(@"[INIT] ========================================");
    
    MTKView* view = (__bridge MTKView*)nativeView;
    if (!view) {
        NSLog(@"[ERROR] PlatformLayer::Initialize: nativeView is null or invalid MTKView");
        return;
    }
    
    NSLog(@"[INIT] Got MTKView: %p", view);
    m_View = nativeView;
    
    // Set up the MTKView
    NSLog(@"[INIT] Setting up MTKView properties");
    view.device = MTLCreateSystemDefaultDevice();
    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    view.clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0);
    NSLog(@"[INIT] MTKView device: %p", view.device);
    
    // Create the delegate and set it as the MTKView's delegate for rendering
    NSLog(@"[INIT] Creating PlatformLayerDelegate");
    id delegate = [[PlatformLayerDelegate alloc] initWithView:view];
    m_Delegate = (__bridge_retained void*)delegate;
    
    // Set the delegate as the MTKView's delegate for automatic rendering
    view.delegate = delegate;
    
    // Enable automatic drawing
    view.paused = NO;
    
    m_TouchPoints.clear();
    NSLog(@"[INIT] ========================================");
    NSLog(@"[INIT] PlatformLayer::Initialize(void* nativeView) COMPLETED");
    NSLog(@"[INIT] m_View=%p, m_Delegate=%p", m_View, m_Delegate);
    NSLog(@"[INIT] ========================================");
}

void PlatformLayer::Initialize() {
    // Implementation for initialization if needed beyond what's in Initialize(void*)
}

void PlatformLayer::Shutdown() {
    // Cleanup if needed
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
    NSLog(@"[DEBUG] GetResourcePath called with: %s", relativePath.c_str());
    
    // For iOS asset catalogs, we need to handle the path differently
    // Asset catalogs store resources with their full path (e.g., "hats/poophat")
    NSString* path = [NSString stringWithUTF8String:relativePath.c_str()];
    
    // First try to find the resource as a raw file (for non-asset catalog resources)
    NSString* fullPath = [[NSBundle mainBundle] pathForResource:path ofType:nil];
    if (fullPath) {
        NSLog(@"[DEBUG] GetResourcePath: Found as raw file: %@", fullPath);
        return std::string([fullPath UTF8String]);
    }
    
    // If not found as raw file, try to extract the base name for asset catalog lookup
    // Asset catalogs store resources by their full path (e.g., "hats/poophat" not just "poophat")
    NSString* baseName = [path stringByDeletingPathExtension];
    NSString* directory = [path stringByDeletingLastPathComponent];
    
    NSLog(@"[DEBUG] GetResourcePath: relativePath=%s, directory=%@, baseName=%@", relativePath.c_str(), directory, baseName);
    
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
        
        NSLog(@"[DEBUG] GetResourcePath: Directory %@ is in asset catalog list", directory);
        
        // Extract just the filename without the directory prefix
        NSString* fileName = [baseName lastPathComponent];
        // Remove file extension for asset catalog
        NSString* assetName = [fileName stringByDeletingPathExtension];
        
        // Since the asset catalog is compiled and we know these resources exist,
        // just return the asset:// path for all known asset catalog resources
        std::string assetPath = std::string("asset://") + std::string([assetName UTF8String]);
        NSLog(@"[DEBUG] GetResourcePath: Returning asset catalog path: %s", assetPath.c_str());
        return assetPath;
    }
    
    // Special handling for music files - they should be loaded from the bundle
    NSString* fileExtension = [path pathExtension];
    if ([fileExtension isEqualToString:@"mp3"] || [fileExtension isEqualToString:@"ogg"] || [fileExtension isEqualToString:@"wav"]) {
        NSLog(@"[DEBUG] GetResourcePath: Music file detected: %@", path);
        
        // For music files, we need to construct the proper bundle path
        // The asset catalog script processes music files and puts them in the bundle
        NSString* fileName = [baseName lastPathComponent];
        NSString* musicPath = [[NSBundle mainBundle] pathForResource:fileName ofType:fileExtension];
        
        if (musicPath) {
            NSLog(@"[DEBUG] GetResourcePath: Found music file in bundle: %@", musicPath);
            return std::string([musicPath UTF8String]);
        } else {
            NSLog(@"[DEBUG] GetResourcePath: Music file not found in bundle: %@", fileName);
            // Fallback to asset:// path for music files
            NSString* assetName = [fileName stringByDeletingPathExtension];
            std::string assetPath = std::string("asset://") + std::string([assetName UTF8String]);
            NSLog(@"[DEBUG] GetResourcePath: Returning asset catalog path for music: %s", assetPath.c_str());
            return assetPath;
        }
    }
    
    NSLog(@"[DEBUG] GetResourcePath: Fallback to original path: %s", relativePath.c_str());
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

void PlatformLayer::UpdateTouchState() {
    if (!m_View) return;
    
    UIView* view = (__bridge UIView*)m_View;
    // Accessing touches directly might not be the best approach. Consider using delegate methods or gesture recognizers.
    // For now, we'll simulate an empty touch state or implement via delegate if touches are passed.
    // NSSet* touches = view.window.allTouches; // This line caused an error, so we'll adjust the approach.
    
    m_TouchPoints.clear();
    m_PrimaryInputDown = false;
    m_PrimaryInputPressed = false;
    m_PrimaryInputReleased = false;
    m_SecondaryInputDown = false;
    m_SecondaryInputPressed = false;
    m_SecondaryInputReleased = false;
    
    // If touch data comes from delegate or other source, update here.
}

int PlatformLayer::GetTouchCount() const {
    return static_cast<int>(m_TouchPoints.size());
}

Vector2 PlatformLayer::GetTouchPosition(int index) const {
    if (index >= 0 && index < m_TouchPoints.size()) {
        return m_TouchPoints[index];
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
    NSLog(@"[DEBUG] LoadTexture called: fileName=%s", fileName);
    
    // Handle asset catalog resources
    std::string filePath(fileName);
    if (filePath.substr(0, 8) == "asset://") {
        std::string assetName = filePath.substr(8); // Remove "asset://" prefix
        NSString* name = [NSString stringWithUTF8String:assetName.c_str()];
        NSLog(@"[DEBUG] LoadTexture: Loading asset catalog texture: %@", name);
        UIImage* uiImage = [UIImage imageNamed:name];
        
        if (!uiImage) {
            NSLog(@"[ERROR] LoadTexture: Failed to load asset catalog texture: %s", fileName);
            return nullptr;
        }
        
        NSLog(@"[DEBUG] LoadTexture: Successfully loaded UIImage for %@", name);
        
        CGImageRef cgImage = uiImage.CGImage;
        size_t cgWidth = CGImageGetWidth(cgImage);
        size_t cgHeight = CGImageGetHeight(cgImage);
        NSLog(@"[DEBUG] LoadTexture: CGImage dimensions: %zux%zu", cgWidth, cgHeight);
        
        // Validate CGImage dimensions
        if (cgWidth == 0 || cgHeight == 0) {
            NSLog(@"[ERROR] LoadTexture: CGImage has invalid dimensions!");
            return nullptr;
        }
        
        // Validate width/height pointers
        NSLog(@"[DEBUG] LoadTexture: About to validate pointers - width: %p, height: %p", width, height);
        if (!width) {
            NSLog(@"[ERROR] LoadTexture: Width pointer is null!");
            return nullptr;
        }
        if (!height) {
            NSLog(@"[ERROR] LoadTexture: Height pointer is null!");
            return nullptr;
        }
        
        NSLog(@"[DEBUG] LoadTexture: Pointers validated, about to assign dimensions");
        *width = (int)cgWidth;
        *height = (int)cgHeight;
        NSLog(@"[DEBUG] LoadTexture: Final image dimensions: %dx%d", *width, *height);
        
        // Create Metal texture
        NSLog(@"[DEBUG] LoadTexture: Creating Metal texture descriptor");
        MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
        NSLog(@"[DEBUG] LoadTexture: Metal texture descriptor created: %p", textureDescriptor);
        textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        textureDescriptor.width = *width;
        textureDescriptor.height = *height;
        NSLog(@"[DEBUG] LoadTexture: Metal texture descriptor configured");
        NSLog(@"[DEBUG] LoadTexture: Creating Metal texture with device: %p", m_MetalDevice);
        NSError *error = nil;
        id<MTLTexture> texture = [(__bridge id<MTLDevice>)m_MetalDevice newTextureWithDescriptor:textureDescriptor];
        LogMetalError(error, @"Creating Metal texture");
        if (!texture) {
            NSLog(@"[ERROR] LoadTexture: Failed to create Metal texture!");
            return nullptr;
        }
        NSLog(@"[DEBUG] LoadTexture: Metal texture created: %p (retain count: %lu)", texture, (unsigned long)CFGetRetainCount((__bridge CFTypeRef)texture));
        
        // Load image data into texture
        NSLog(@"[DEBUG] LoadTexture: Creating color space");
        MTLRegion region = {{0, 0, 0}, {(NSUInteger)*width, (NSUInteger)*height, 1}};
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        NSLog(@"[DEBUG] LoadTexture: Color space created: %p", colorSpace);
        NSLog(@"[DEBUG] LoadTexture: Creating bitmap context");
        CGContextRef context = CGBitmapContextCreate(nil, *width, *height, 8, 0, colorSpace, kCGImageAlphaPremultipliedLast);
        if (!context) {
            NSLog(@"[ERROR] LoadTexture: Failed to create bitmap context!");
            CGColorSpaceRelease(colorSpace);
            return nullptr;
        }
        NSLog(@"[DEBUG] LoadTexture: Bitmap context created: %p", context);
        NSLog(@"[DEBUG] LoadTexture: Drawing image into context");
        CGContextDrawImage(context, CGRectMake(0, 0, *width, *height), cgImage);
        NSLog(@"[DEBUG] LoadTexture: Image drawn into context");
        void* imageData = CGBitmapContextGetData(context);
        NSLog(@"[DEBUG] LoadTexture: Image data pointer: %p", imageData);
        NSLog(@"[DEBUG] LoadTexture: Replacing Metal texture region");
        [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * *width];
        NSLog(@"[DEBUG] LoadTexture: Metal texture region replaced");
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        
        NSLog(@"[DEBUG] LoadTexture: Texture loading completed successfully");
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
        NSLog(@"[ERROR] LoadTexture: Failed to load image: %s", fileName);
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
    id<MTLTexture> texture = [(__bridge id<MTLDevice>)m_MetalDevice newTextureWithDescriptor:textureDescriptor];
    if (!texture) {
        NSLog(@"[ERROR] LoadTexture: Failed to create Metal texture!");
        return nullptr;
    }
    NSLog(@"[DEBUG] LoadTexture: Regular file texture created: %p (retain count: %lu)", texture, (unsigned long)CFGetRetainCount((__bridge CFTypeRef)texture));
    
    // Load image data into texture
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)*width, (NSUInteger)*height, 1}};
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(nil, *width, *height, 8, 0, colorSpace, kCGImageAlphaPremultipliedLast);
    if (!context) {
        NSLog(@"[ERROR] LoadTexture: Failed to create bitmap context!");
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
        NSLog(@"[DEBUG] UnloadTexture: Releasing texture: %p", metalTexture);
        
        // Release the bridged texture
        CFBridgingRelease(texture);
        NSLog(@"[DEBUG] UnloadTexture: Bridge released for texture: %p", metalTexture);
    }
}

void* PlatformLayer::LoadRenderTexture(int width, int height) {
    NSLog(@"[DEBUG] LoadRenderTexture: Starting with width=%d, height=%d", width, height);
    NSLog(@"[DEBUG] LoadRenderTexture: Metal device: %p", m_MetalDevice);
    
    if (!m_MetalDevice) {
        NSLog(@"[ERROR] LoadRenderTexture: No Metal device available!");
        return nullptr;
    }
    
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    
    NSLog(@"[DEBUG] LoadRenderTexture: Creating Metal texture with descriptor...");
    id<MTLTexture> texture = [(__bridge id<MTLDevice>)m_MetalDevice newTextureWithDescriptor:textureDescriptor];
    
    if (!texture) {
        NSLog(@"[ERROR] LoadRenderTexture: Failed to create Metal render texture!");
        return nullptr;
    }
    
    NSLog(@"[DEBUG] LoadRenderTexture: Successfully created Metal texture: %p (w=%lu, h=%lu)", 
          texture, (unsigned long)texture.width, (unsigned long)texture.height);
    
    return (__bridge_retained void*)texture;
}

void PlatformLayer::BeginDrawing(void* renderTexture) {
    if (!m_Delegate) {
        NSLog(@"[ERROR] BeginDrawing called with no delegate set!");
        return;
    }
    
    // Get the delegate
    PlatformLayerDelegate* delegate = (__bridge PlatformLayerDelegate*)m_Delegate;
    
    // Get the MTKView from the delegate
    MTKView* mtkView = delegate.view;
    if (!mtkView) {
        NSLog(@"[ERROR] BeginDrawing: Could not get MTKView from delegate!");
        return;
    }

#if defined(PLATFORM_MOBILE)
    // --- MOBILE RENDERING PATH ---
    // Use UIManager to get the actual screen dimensions for the projection matrix.
    UIManager& uiManager = UIManager::GetInstance();
    float screenWidth = uiManager.GetSafeArea().width;
    float screenHeight = uiManager.GetSafeArea().height;
    
    // Update the drawable size for the MTKView
    [mtkView setDrawableSize:CGSizeMake(screenWidth, screenHeight)];
    
    // Set the view's content scale factor based on the screen density
    float density = GetScreenDensity();
    [mtkView setContentScaleFactor:density];
    
    // Notify the view that it needs to redraw with the new size
    [mtkView setNeedsDisplay];
#else
    // --- DESKTOP RENDERING PATH ---
    // For desktop, we still use the fixed 320x180 resolution
    [mtkView setDrawableSize:CGSizeMake(320.0f, 180.0f)];
    [mtkView setContentScaleFactor:1.0f];
    [mtkView setNeedsDisplay];
#endif
}

void PlatformLayer::EndDrawing(void* renderTexture) {
    // End the current render pass if needed
    // For simplicity, we rely on the MTKView delegate to handle rendering commits
}

void PlatformLayer::DrawRectangle(int posX, int posY, int width, int height, unsigned int color) {
    NSLog(@"[DEBUG] PlatformLayer::DrawRectangle called: posX=%d, posY=%d, width=%d, height=%d, color=0x%08X", posX, posY, width, height, color);
    
    // Ensure we're on the main thread
    if (![NSThread isMainThread]) {
        NSLog(@"[ERROR] PlatformLayer::DrawRectangle called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            this->DrawRectangle(posX, posY, width, height, color);
        });
        return;
    }
    
    NSLog(@"[DEBUG] PlatformLayer::DrawRectangle: Calling delegate drawRectangleWithPosX");
    if (m_Delegate) {
        PlatformLayerDelegate* delegate = (__bridge PlatformLayerDelegate*)m_Delegate;
        [delegate drawRectangleWithPosX:posX posY:posY width:width height:height color:color];
    }
    NSLog(@"[DEBUG] PlatformLayer::DrawRectangle: Delegate call completed");
}

void PlatformLayer::DrawTexture(void* texture, float x, float y, float width, float height, Color tint) {
    NSLog(@"[DEBUG] PlatformLayer::DrawTexture called: texture=%p, x=%.2f, y=%.2f, width=%.2f, height=%.2f", texture, x, y, width, height);
    // Ensure we're on the main thread
    if (![NSThread isMainThread]) {
        NSLog(@"[ERROR] PlatformLayer::DrawTexture called on non-main thread! Current thread: %@", [NSThread currentThread]);
        dispatch_async(dispatch_get_main_queue(), ^{
            this->DrawTexture(texture, x, y, width, height, tint);
        });
        return;
    }
    // Validate texture
    if (!texture) {
        NSLog(@"[ERROR] PlatformLayer::DrawTexture: Invalid texture pointer");
        return;
    }
    id<MTLTexture> metalTexture = (__bridge id<MTLTexture>)texture;
    if (!metalTexture) {
        NSLog(@"[ERROR] PlatformLayer::DrawTexture: Failed to bridge texture pointer");
        return;
    }
    // Check if texture is still valid
    if (metalTexture.width == 0 || metalTexture.height == 0) {
        NSLog(@"[ERROR] PlatformLayer::DrawTexture: Texture has invalid dimensions (w=%lu, h=%lu)", (unsigned long)metalTexture.width, (unsigned long)metalTexture.height);
        return;
    }
    NSLog(@"[DEBUG] PlatformLayer::DrawTexture: Valid texture found (ptr=%p, w=%lu, h=%lu)", metalTexture, (unsigned long)metalTexture.width, (unsigned long)metalTexture.height);
    if (m_Delegate) {
        PlatformLayerDelegate* delegate = (__bridge PlatformLayerDelegate*)m_Delegate;
        [delegate drawTexture:texture x:(int)x y:(int)y width:(int)width height:(int)height tint:ColorToUInt(tint)];
    }
}

void PlatformLayer::EnqueueDrawCommand(void* vertexBuffer, void* texture, size_t vertexCount) {
    // This function is now deprecated in favor of direct drawing
    // Keeping for compatibility but it's no longer used
}

void* PlatformLayer::GetMetalDevice() const {
    return m_MetalDevice;
}

void* PlatformLayer::GetDelegate() const {
    return m_Delegate;
}

void* PlatformLayer::LoadTextureFromImage(void* imageData, int width, int height, int format) {
    if (!imageData || width <= 0 || height <= 0) {
        NSLog(@"[ERROR] LoadTextureFromImage: Invalid parameters - imageData=%p, width=%d, height=%d", imageData, width, height);
        return nullptr;
    }
    
    // Create Metal texture
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    
    id<MTLTexture> texture = [(__bridge id<MTLDevice>)m_MetalDevice newTextureWithDescriptor:textureDescriptor];
    if (!texture) {
        NSLog(@"[ERROR] LoadTextureFromImage: Failed to create Metal texture!");
        return nullptr;
    }
    
    // Upload the image data
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
    [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
    
    NSLog(@"[DEBUG] LoadTextureFromImage: Created texture from image data - %dx%d, format=%d", width, height, format);
    
    return (__bridge_retained void*)texture;
}

void PlatformLayer::DrawText(const char* text, float x, float y, float fontSize, Color color, void* font) {
    NSLog(@"[DEBUG] DrawText called: text='%s', pos=(%.1f, %.1f), fontSize=%.1f, color=(%d,%d,%d,%d)", 
          text, x, y, fontSize, color.r, color.g, color.b, color.a);
    if (m_Delegate) {
        PlatformLayerDelegate* delegate = (__bridge PlatformLayerDelegate*)m_Delegate;
        [delegate drawText:text x:x y:y fontSize:fontSize color:ColorToUInt(color) font:font];
    }
}

#endif // PLATFORM_IOS
