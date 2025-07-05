#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <AVFoundation/AVFoundation.h>
#import "PlatformLayer.h"
#import "RaylibCompat.h"
#import "MetalRenderer.h"

// Singleton instance
static PlatformLayer* s_Instance = nullptr;

@interface PlatformLayerDelegate : NSObject <MTKViewDelegate>
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) MTKView* view;
@property (nonatomic, assign) BOOL isInitialized;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLLibrary> library;
@property (nonatomic, strong) NSMutableArray* drawCommands;
@end

@implementation PlatformLayerDelegate

- (instancetype)initWithView:(MTKView*)view {
    self = [super init];
    if (self) {
        _view = view;
        _view.delegate = self;
        _device = MTLCreateSystemDefaultDevice();
        _view.device = _device;
        _commandQueue = [_device newCommandQueue];
        _isInitialized = NO;
        _library = [_device newDefaultLibrary];
        _drawCommands = [NSMutableArray array];
    }
    return self;
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle resize if needed
}

- (void)drawInMTKView:(nonnull MTKView *)view {
    if (!self.isInitialized) {
        // Initialization logic if needed
        self.isInitialized = YES;
        // Setup Metal pipeline state if not already done
        [self setupPipelineState];
    }
    
    // Create command buffer for rendering
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;
    
    if (renderPassDescriptor != nil) {
        // Configure render pass descriptor
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0); // Black background
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [renderEncoder setRenderPipelineState:self.pipelineState];
        
        // Set viewport
        MTLViewport viewport = {0.0, 0.0, (double)view.drawableSize.width, (double)view.drawableSize.height, 0.0, 1.0};
        [renderEncoder setViewport:viewport];
        
        // Render all queued drawing commands
        [self renderQueuedDrawCommands:renderEncoder];
        
        [renderEncoder endEncoding];
        [commandBuffer presentDrawable:view.currentDrawable];
    }
    [commandBuffer commit];
}

- (void)renderQueuedDrawCommands:(id<MTLRenderCommandEncoder>)renderEncoder {
    // Render a simple triangle as a test if no other commands are queued
    if (self.drawCommands.count == 0) {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f
        };
        id<MTLBuffer> vertexBuffer = [self.device newBufferWithBytes:vertices length:sizeof(vertices) options:MTLResourceStorageModeShared];
        [renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    } else {
        // Process each draw command
        for (NSDictionary* command in self.drawCommands) {
            id<MTLBuffer> vertexBuffer = command[@"vertexBuffer"];
            id<MTLTexture> texture = command[@"texture"];
            NSNumber* vertexCount = command[@"vertexCount"];
            
            [renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
            if (texture) {
                [renderEncoder setFragmentTexture:texture atIndex:0];
            }
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:[vertexCount unsignedIntegerValue]];
        }
        // Clear the draw commands after rendering
        [self.drawCommands removeAllObjects];
    }
}

// Setup Metal pipeline state
- (void)setupPipelineState {
    // Create vertex descriptor
    MTLVertexDescriptor* vertexDescriptor = [[MTLVertexDescriptor alloc] init];
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat3;
    vertexDescriptor.attributes[0].offset = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;
    vertexDescriptor.layouts[0].stride = 3 * sizeof(float);
    vertexDescriptor.layouts[0].stepRate = 1;
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    // Create render pipeline descriptor
    MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.label = @"Simple Pipeline";
    pipelineDescriptor.vertexFunction = [self.library newFunctionWithName:@"vertexShader"];
    pipelineDescriptor.fragmentFunction = [self.library newFunctionWithName:@"fragmentShader"];
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.view.colorPixelFormat;
    
    // Create pipeline state
    NSError* error = nil;
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (error) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

@end

PlatformLayer& PlatformLayer::GetInstance() {
    if (!s_Instance) {
        s_Instance = new PlatformLayer();
    }
    return *s_Instance;
}

PlatformLayer::PlatformLayer() : m_Delegate(nullptr), m_View(nullptr), m_PrimaryInputDown(false), m_PrimaryInputPressed(false), m_PrimaryInputReleased(false), m_SecondaryInputDown(false), m_SecondaryInputPressed(false), m_SecondaryInputReleased(false) {
}

PlatformLayer::~PlatformLayer() {
    if (m_Delegate) {
        id delegate = (__bridge_transfer id)m_Delegate;
        delegate = nil;
        m_Delegate = nullptr;
    }
}

void PlatformLayer::Initialize(void* nativeView) {
    MTKView* view = (__bridge MTKView*)nativeView;
    if (!view) return;
    m_View = nativeView;
    m_Delegate = (__bridge_retained void*)[[PlatformLayerDelegate alloc] initWithView:view];
    m_TouchPoints.clear();
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
    NSString* path = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:relativePath.c_str()] ofType:nil];
    if (path) {
        return std::string([path UTF8String]);
    }
    return relativePath; // Fallback
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
    // Create and initialize MetalRenderer if not already cached

    // Return a valid MetalRenderer instance
    // This assumes MetalRenderer is properly initialized in the PlatformLayerDelegate
    MetalRenderer* renderer = new MetalRenderer();
    renderer->Initialize((__bridge MTKView*)m_View);
    return renderer;
}

// Texture and image handling
void* PlatformLayer::LoadTexture(const char* fileName, int* width, int* height) {
    NSString* path = [NSString stringWithUTF8String:fileName];
    NSString* fullPath = [[NSBundle mainBundle] pathForResource:path ofType:nil];
    if (!fullPath) {
        fullPath = [NSString stringWithUTF8String:fileName];
    }
    UIImage* uiImage = [UIImage imageWithContentsOfFile:fullPath];
    if (!uiImage) {
        NSLog(@"Failed to load image: %s", fileName);
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
        id<MTLTexture> texture = [((__bridge PlatformLayerDelegate*)m_Delegate).device newTextureWithDescriptor:textureDescriptor];
    
    // Load image data into texture
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)*width, (NSUInteger)*height, 1}};
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(nil, *width, *height, 8, 0, colorSpace, kCGImageAlphaPremultipliedLast);
    CGContextDrawImage(context, CGRectMake(0, 0, *width, *height), cgImage);
    void* imageData = CGBitmapContextGetData(context);
    [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * *width];
    
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    
    return (__bridge_retained void*)texture;
}

void PlatformLayer::UnloadTexture(void* texture) {
    if (texture) {
        // Assuming texture is an id<MTLTexture>, release it
        CFRelease(texture);
    }
}

void* PlatformLayer::LoadRenderTexture(int width, int height) {
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        id<MTLTexture> texture = [((__bridge PlatformLayerDelegate*)m_Delegate).device newTextureWithDescriptor:textureDescriptor];
    return (__bridge_retained void*)texture;
}

void PlatformLayer::BeginDrawing(void* renderTexture) {
    // Start a new render pass if needed
    // For simplicity, we rely on the MTKView delegate to handle rendering
    // But in a more complex app, this would set up a specific render target
    if (renderTexture) {
        // If a render texture is provided, set up rendering to that texture
        id<MTLTexture> texture = (__bridge id<MTLTexture>)renderTexture;
        // Configure render pass to target this texture - would require additional setup
    }
}

void PlatformLayer::EndDrawing(void* renderTexture) {
    // End the current render pass if needed
    // For simplicity, we rely on the MTKView delegate to handle rendering commits
}

void PlatformLayer::DrawRectangle(int posX, int posY, int width, int height, unsigned int color) {
    // Extract color components
    float r = ((color >> 24) & 0xFF) / 255.0f;
    float g = ((color >> 16) & 0xFF) / 255.0f;
    float b = ((color >> 8) & 0xFF) / 255.0f;
    float a = (color & 0xFF) / 255.0f;
    
    // Create vertices for the rectangle (normalized device coordinates)
    // Assuming screen coordinates need to be converted to NDC
        float screenWidth = (float)[((__bridge PlatformLayerDelegate*)m_Delegate).view drawableSize].width;
        float screenHeight = (float)[((__bridge PlatformLayerDelegate*)m_Delegate).view drawableSize].height;
    float x1 = (float)posX / screenWidth * 2.0f - 1.0f;
    float y1 = (float)posY / screenHeight * 2.0f - 1.0f;
    float x2 = (float)(posX + width) / screenWidth * 2.0f - 1.0f;
    float y2 = (float)(posY + height) / screenHeight * 2.0f - 1.0f;
    
    // Define vertices for two triangles forming a rectangle
    float vertices[] = {
        x1, y1, 0.0f, r, g, b, a,  // bottom-left
        x2, y1, 0.0f, r, g, b, a,  // bottom-right
        x1, y2, 0.0f, r, g, b, a,  // top-left
        x2, y1, 0.0f, r, g, b, a,  // bottom-right
        x2, y2, 0.0f, r, g, b, a,  // top-right
        x1, y2, 0.0f, r, g, b, a   // top-left
    };
    
    // Create vertex buffer
        id<MTLBuffer> vertexBuffer = [((__bridge PlatformLayerDelegate*)m_Delegate).device newBufferWithBytes:vertices length:sizeof(vertices) options:MTLResourceStorageModeShared];
    
    // Set up for drawing (assuming we have access to current render encoder or command buffer)
    // In a real app, this would need to be managed within the render loop
    // For now, this is a conceptual implementation
    // [currentRenderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
    // [currentRenderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    
    // Note: Actual rendering would need to be integrated into the render loop
    // This implementation shows the logic but may need adjustment based on app architecture
}



void* PlatformLayer::LoadTextureFromImage(void* imageData, int width, int height, int format) {
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
        id<MTLTexture> texture = [((__bridge PlatformLayerDelegate*)m_Delegate).device newTextureWithDescriptor:textureDescriptor];
    
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
    [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
    
    return (__bridge_retained void*)texture;
}

// Audio management
void PlatformLayer::InitializeAudio() {
    // Initialize audio session for iOS
    NSError* error = nil;
        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:&error];
    if (error) {
        NSLog(@"Error initializing audio session: %@", error);
    }
    [[AVAudioSession sharedInstance] setActive:YES error:&error];
    if (error) {
        NSLog(@"Error activating audio session: %@", error);
    }
}

void PlatformLayer::ShutdownAudio() {
    // Deactivate audio session
    NSError* error = nil;
    [[AVAudioSession sharedInstance] setActive:NO error:&error];
    if (error) {
        NSLog(@"Error deactivating audio session: %@", error);
    }
}

void* PlatformLayer::LoadSound(const char* fileName) {
    NSString* path = [NSString stringWithUTF8String:fileName];
    NSString* fullPath = [[NSBundle mainBundle] pathForResource:path ofType:nil];
    if (!fullPath) {
        fullPath = [NSString stringWithUTF8String:fileName];
    }
    NSURL* url = [NSURL fileURLWithPath:fullPath];
    NSError* error = nil;
    AVAudioPlayer* player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
    if (error) {
        NSLog(@"Error loading sound: %@", error);
        return nullptr;
    }
    [player prepareToPlay];
    return (__bridge_retained void*)player;
}

void PlatformLayer::UnloadSound(void* sound) {
    if (sound) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound;
        [player stop];
        CFRelease(sound);
    }
}

void PlatformLayer::PlaySound(void* sound) {
    if (sound) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound;
        [player play];
    }
}

void PlatformLayer::SetSoundVolume(void* sound, float volume) {
    if (sound) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)sound;
        player.volume = volume;
    }
}

// Music streaming
void* PlatformLayer::LoadMusic(const char* fileName) {
    NSString* path = [NSString stringWithUTF8String:fileName];
    NSString* fullPath = [[NSBundle mainBundle] pathForResource:path ofType:nil];
    if (!fullPath) {
        fullPath = [NSString stringWithUTF8String:fileName];
    }
    NSURL* url = [NSURL fileURLWithPath:fullPath];
    NSError* error = nil;
    AVAudioPlayer* player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
    if (error) {
        NSLog(@"Error loading music: %@", error);
        return nullptr;
    }
    player.numberOfLoops = -1; // Loop indefinitely
    [player prepareToPlay];
    return (__bridge_retained void*)player;
}

void PlatformLayer::UnloadMusic(void* music) {
    if (music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music;
        [player stop];
        CFRelease(music);
    }
}

void PlatformLayer::PlayMusic(void* music) {
    if (music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music;
        [player play];
    }
}

void PlatformLayer::StopMusic(void* music) {
    if (music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music;
        [player stop];
        [player setCurrentTime:0];
    }
}

void PlatformLayer::UpdateMusic(void* music) {
    // No-op for AVAudioPlayer, as it handles playback internally
}

bool PlatformLayer::IsMusicPlaying(void* music) {
    if (music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music;
        return player.isPlaying;
    }
    return false;
}

void PlatformLayer::SetMusicVolume(void* music, float volume) {
    if (music) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)music;
        player.volume = volume;
    }
}

// Text rendering via MetalTextRenderer
void* PlatformLayer::LoadFont(const char* fileName, int size) {
    NSString* fontName = [NSString stringWithUTF8String:fileName];
    UIFont* font = [UIFont fontWithName:fontName size:size];
    if (!font) {
        font = [UIFont systemFontOfSize:size];
    }
    return (__bridge_retained void*)font;
}

void PlatformLayer::UnloadFont(void* font) {
    if (font) {
        CFRelease(font);
    }
}

Vector2 PlatformLayer::MeasureText(const char* text, void* font, float fontSize, float spacing) {
    if (!text || !font) return Vector2{0, 0};
    NSString* nsText = [NSString stringWithUTF8String:text];
    UIFont* uiFont = (__bridge UIFont*)font;
    if (fontSize != uiFont.pointSize) {
        uiFont = [uiFont fontWithSize:fontSize];
    }
    NSDictionary* attributes = @{NSFontAttributeName: uiFont};
    CGSize size = [nsText sizeWithAttributes:attributes];
    return Vector2{(float)size.width, (float)size.height};
}

void PlatformLayer::DrawText(const char* text, float x, float y, float fontSize, Color color, void* font) {
    if (!text || !font) return;
    
    NSString* nsText = [NSString stringWithUTF8String:text];
    UIFont* uiFont = (__bridge UIFont*)font;
    if (fontSize != uiFont.pointSize) {
        uiFont = [uiFont fontWithSize:fontSize];
    }
    
    // Convert Color to UIColor
    UIColor* textColor = [UIColor colorWithRed:color.r/255.0 green:color.g/255.0 blue:color.b/255.0 alpha:color.a/255.0];
    
    // Calculate text size
    NSDictionary* attributes = @{NSFontAttributeName: uiFont, NSForegroundColorAttributeName: textColor};
    CGSize textSize = [nsText sizeWithAttributes:attributes];
    
    // Create a bitmap context to render the text
    UIGraphicsBeginImageContextWithOptions(textSize, NO, 0.0);
    [nsText drawAtPoint:CGPointZero withAttributes:attributes];
    UIImage* textImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
    if (!textImage) return;
    
    // Convert UIImage to Metal texture
    CGImageRef cgImage = textImage.CGImage;
    int width = (int)CGImageGetWidth(cgImage);
    int height = (int)CGImageGetHeight(cgImage);
    
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    id<MTLTexture> texture = [((__bridge PlatformLayerDelegate*)m_Delegate).device newTextureWithDescriptor:textureDescriptor];
    
    // Load image data into texture
    MTLRegion region = {{0, 0, 0}, {(NSUInteger)width, (NSUInteger)height, 1}};
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(nil, width, height, 8, 0, colorSpace, kCGImageAlphaPremultipliedLast);
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
    void* imageData = CGBitmapContextGetData(context);
    [texture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:4 * width];
    
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    
    // Now draw the texture at the specified position
    // Convert screen coordinates to normalized device coordinates
    float screenWidth = (float)[((__bridge PlatformLayerDelegate*)m_Delegate).view drawableSize].width;
    float screenHeight = (float)[((__bridge PlatformLayerDelegate*)m_Delegate).view drawableSize].height;
    float x1 = x / screenWidth * 2.0f - 1.0f;
    float y1 = 1.0f - (y + height) / screenHeight * 2.0f; // Flip Y coordinate
    float x2 = (x + width) / screenWidth * 2.0f - 1.0f;
    float y2 = 1.0f - y / screenHeight * 2.0f; // Flip Y coordinate
    
    // Define vertices for the text quad (two triangles)
    float vertices[] = {
        x1, y1, 0.0f, 0.0f, 1.0f,  // bottom-left
        x2, y1, 0.0f, 1.0f, 1.0f,  // bottom-right
        x1, y2, 0.0f, 0.0f, 0.0f,  // top-left
        x2, y1, 0.0f, 1.0f, 1.0f,  // bottom-right
        x2, y2, 0.0f, 1.0f, 0.0f,  // top-right
        x1, y2, 0.0f, 0.0f, 0.0f   // top-left
    };
    
    // Create vertex buffer
    id<MTLBuffer> vertexBuffer = [((__bridge PlatformLayerDelegate*)m_Delegate).device newBufferWithBytes:vertices length:sizeof(vertices) options:MTLResourceStorageModeShared];
    
    // Enqueue the draw command for Metal rendering
    EnqueueDrawCommand((__bridge void*)vertexBuffer, (__bridge void*)texture, 6);
}

void PlatformLayer::EnqueueDrawCommand(void* vertexBuffer, void* texture, size_t vertexCount) {
    NSDictionary* command = @{
        @"vertexBuffer": (__bridge id)vertexBuffer,
        @"texture": texture ? (__bridge id)texture : [NSNull null],
        @"vertexCount": @(vertexCount)
    };
    [((__bridge PlatformLayerDelegate*)m_Delegate).drawCommands addObject:command];
}
#endif // PLATFORM_IOS
