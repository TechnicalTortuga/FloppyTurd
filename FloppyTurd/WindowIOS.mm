#import "WindowIOS.h"
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

// ============================================================================
// iOS-SPECIFIC WINDOW IMPLEMENTATION
// ============================================================================

@interface WindowIOSDelegate : NSObject
@property (nonatomic, assign) BOOL preferLandscape;
@property (nonatomic, strong) UIViewController* rootViewController;
@end

@implementation WindowIOSDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        _preferLandscape = YES; // Default to landscape for games
    }
    return self;
}

- (BOOL)shouldAutorotate {
    return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    if (self.preferLandscape) {
        return UIInterfaceOrientationMaskLandscape;
    } else {
        return UIInterfaceOrientationMaskPortrait;
    }
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation {
    if (self.preferLandscape) {
        return UIInterfaceOrientationLandscapeRight;
    } else {
        return UIInterfaceOrientationPortrait;
    }
}

@end

// ============================================================================
// C++ IMPLEMENTATION
// ============================================================================

#include "PlatformAPI.h"
#include "GlobalStateManager.h"

// Global instance for iOS window management
static WindowIOSDelegate* s_windowDelegate = nil;
static UIViewController* s_rootViewController = nil;

// Initialize iOS window system
void WindowIOS_Initialize() {
    if (!s_windowDelegate) {
        s_windowDelegate = [[WindowIOSDelegate alloc] init];
    }
}

// Cleanup iOS window system
void WindowIOS_Shutdown() {
    s_windowDelegate = nil;
    s_rootViewController = nil;
}

// Set the root view controller for orientation management
void WindowIOS_SetRootViewController(void* viewController) {
    s_rootViewController = (__bridge UIViewController*)viewController;
    if (s_windowDelegate) {
        s_windowDelegate.rootViewController = s_rootViewController;
    }
}

// Get safe area insets (for notches, home indicators, etc.)
Rectangle WindowIOS_GetSafeArea() {
    if (@available(iOS 11.0, *)) {
        UIWindow* window = [UIApplication sharedApplication].windows.firstObject;
        if (window) {
            UIEdgeInsets safeArea = window.safeAreaInsets;
            return Rectangle{
                static_cast<float>(safeArea.left),
                static_cast<float>(safeArea.top),
                static_cast<float>(window.frame.size.width - safeArea.left - safeArea.right),
                static_cast<float>(window.frame.size.height - safeArea.top - safeArea.bottom)
            };
        }
    }
    
    // Fallback: return full screen area
    UIScreen* screen = [UIScreen mainScreen];
    return Rectangle{
        0.0f,
        0.0f,
        (float)screen.bounds.size.width,
        (float)screen.bounds.size.height
    };
}

// Get screen density (scale factor)
float WindowIOS_GetScreenDensity() {
    UIScreen* screen = [UIScreen mainScreen];
    return (float)screen.scale;
}

// Check if device is in landscape orientation
bool WindowIOS_IsLandscape() {
    UIDeviceOrientation orientation = [UIDevice currentDevice].orientation;
    return (orientation == UIDeviceOrientationLandscapeLeft || 
            orientation == UIDeviceOrientationLandscapeRight);
}

// Check if device is in portrait orientation
bool WindowIOS_IsPortrait() {
    return !WindowIOS_IsLandscape();
}

// Set preferred orientation (landscape or portrait)
void WindowIOS_SetPreferredOrientation(bool landscape) {
    if (s_windowDelegate) {
        s_windowDelegate.preferLandscape = landscape;
        
        // Force orientation change if needed
        if (s_rootViewController) {
            UIInterfaceOrientation newOrientation;
            if (landscape) {
                newOrientation = UIInterfaceOrientationLandscapeRight;
            } else {
                newOrientation = UIInterfaceOrientationPortrait;
            }
            
            // Use supported method for iOS 16+
            if (@available(iOS 16.0, *)) {
                [s_rootViewController setNeedsUpdateOfSupportedInterfaceOrientations];
            } else {
                // For older iOS versions, we need to handle this differently
                // The orientation will change when the user rotates the device
            }
        }
    }
}

// Check if we should use larger touch targets (always true on mobile)
bool WindowIOS_ShouldUseLargerTouchTargets() {
    return true; // Always use larger touch targets on mobile
}

// Get recommended font size based on screen density
int WindowIOS_GetRecommendedFontSize() {
    float density = WindowIOS_GetScreenDensity();
    int baseSize = 16;  // Base font size for desktop
    
    // Scale font size based on screen density for mobile readability
    int scaledSize = (int)(baseSize * density);
    
    // Ensure minimum readable size on mobile
    if (scaledSize < 18) scaledSize = 18;
    
    // Cap maximum size to prevent overly large text
    if (scaledSize > 32) scaledSize = 32;
    
    return scaledSize;
}

// Get screen center point
Vector2 WindowIOS_GetScreenCenter() {
    UIScreen* screen = [UIScreen mainScreen];
    return Vector2{
        (float)screen.bounds.size.width / 2.0f,
        (float)screen.bounds.size.height / 2.0f
    };
}

// Get render scale for UI elements
Vector2 WindowIOS_GetRenderScale() {
    float density = WindowIOS_GetScreenDensity();
    
    // On mobile, scale based on screen density for crisp UI
    return Vector2{ density, density };
}

// ============================================================================
// PLATFORM API INTEGRATION
// ============================================================================

// These functions are called by PlatformAPI to route to iOS-specific implementations
extern "C" {
    Rectangle PlatformIOS_GetSafeArea() {
        return WindowIOS_GetSafeArea();
    }
    
    float PlatformIOS_GetScreenDensity() {
        return WindowIOS_GetScreenDensity();
    }
    
    bool PlatformIOS_IsLandscape() {
        return WindowIOS_IsLandscape();
    }
    
    bool PlatformIOS_IsPortrait() {
        return WindowIOS_IsPortrait();
    }
    
    void PlatformIOS_SetPreferredOrientation(bool landscape) {
        WindowIOS_SetPreferredOrientation(landscape);
    }
    
    bool PlatformIOS_ShouldUseLargerTouchTargets() {
        return WindowIOS_ShouldUseLargerTouchTargets();
    }
    
    int PlatformIOS_GetRecommendedFontSize() {
        return WindowIOS_GetRecommendedFontSize();
    }
    
    Vector2 PlatformIOS_GetScreenCenter() {
        return WindowIOS_GetScreenCenter();
    }
    
    Vector2 PlatformIOS_GetRenderScale() {
        return WindowIOS_GetRenderScale();
    }
} 