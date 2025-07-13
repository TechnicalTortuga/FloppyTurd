#ifndef WINDOW_IOS_H
#define WINDOW_IOS_H

#include "PlatformAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// iOS WINDOW MANAGEMENT FUNCTIONS
// ============================================================================

// Initialize the iOS window system
void WindowIOS_Initialize();

// Cleanup the iOS window system
void WindowIOS_Shutdown();

// Set the root view controller for orientation management
void WindowIOS_SetRootViewController(void* viewController);

// ============================================================================
// iOS-SPECIFIC WINDOW FUNCTIONS
// ============================================================================

// Get safe area insets (for notches, home indicators, etc.)
Rectangle WindowIOS_GetSafeArea();

// Get screen density (scale factor)
float WindowIOS_GetScreenDensity();

// Check if device is in landscape orientation
bool WindowIOS_IsLandscape();

// Check if device is in portrait orientation
bool WindowIOS_IsPortrait();

// Set preferred orientation (landscape or portrait)
void WindowIOS_SetPreferredOrientation(bool landscape);

// Check if we should use larger touch targets (always true on mobile)
bool WindowIOS_ShouldUseLargerTouchTargets();

// Get recommended font size based on screen density
int WindowIOS_GetRecommendedFontSize();

// Get screen center point
Vector2 WindowIOS_GetScreenCenter();

// Get render scale for UI elements
Vector2 WindowIOS_GetRenderScale();

// ============================================================================
// PLATFORM API INTEGRATION FUNCTIONS
// ============================================================================

// These functions are called by PlatformAPI to route to iOS-specific implementations
Rectangle PlatformIOS_GetSafeArea();
float PlatformIOS_GetScreenDensity();
bool PlatformIOS_IsLandscape();
bool PlatformIOS_IsPortrait();
void PlatformIOS_SetPreferredOrientation(bool landscape);
bool PlatformIOS_ShouldUseLargerTouchTargets();
int PlatformIOS_GetRecommendedFontSize();
Vector2 PlatformIOS_GetScreenCenter();
Vector2 PlatformIOS_GetRenderScale();

#ifdef __cplusplus
}
#endif

#endif // WINDOW_IOS_H 