//

//  Pure Swift UI Manager for FloppyTurd Game Engine
//  Replaces legacy C++ UIManager with native Swift implementation
//


import Foundation
#if canImport(UIKit)
import UIKit
#endif


/// Swift UI Manager singleton to replace UIManager_getInstance from C++
@MainActor
public final class UIManagerSwift {
    public static let shared = UIManagerSwift()
    
    // Screen dimensions and safe area
    public private(set) var screenSize: Vector2 = Vector2.zero
    public private(set) var pixelDensity: Float = 1.0
    public private(set) var safeAreaInsets: UIEdgeInsets = UIEdgeInsets.zero
    
    private init() {
        updateScreenInfo()
    }
    
    /// Update screen information from UIKit
    public func updateScreenInfo() {
        #if canImport(UIKit)
        let screen = UIScreen.main
        let bounds = screen.bounds
        let scale = Float(screen.scale)
        screenSize = Vector2(x: Float(bounds.width), y: Float(bounds.height))
        pixelDensity = scale
        // Get safe area insets (modern approach)
        if #available(iOS 13.0, *) {
            if let windowScene = UIApplication.shared.connectedScenes.first as? UIWindowScene,
               let window = windowScene.windows.first {
                let insets = window.safeAreaInsets
                safeAreaInsets = UIEdgeInsets(top: CGFloat(insets.top), left: CGFloat(insets.left), bottom: CGFloat(insets.bottom), right: CGFloat(insets.right))
            }
        }
        #endif
    }
    
    /// Get screen width in points
    public func getScreenWidth() -> Float {
        return screenSize.x
    }

    /// Get screen height in points
    public func getScreenHeight() -> Float {
        return screenSize.y
    }

    /// Get screen size in points
    public func getScreenSize() -> Vector2 {
        return screenSize
    }

    /// Get safe area rectangle in points
    public func getSafeAreaRect() -> Rectangle {
        return Rectangle(
            x: Float(safeAreaInsets.left),
            y: Float(safeAreaInsets.top),
            width: screenSize.x - (Float(safeAreaInsets.left) + Float(safeAreaInsets.right)),
            height: screenSize.y - (Float(safeAreaInsets.top) + Float(safeAreaInsets.bottom))
        )
    }
    
    /// Convert points to pixels
    public func pointsToPixels(_ points: Float) -> Float {
        return points * pixelDensity
    }
    
    /// Convert pixels to points
    public func pixelsToPoints(_ pixels: Float) -> Float {
        return pixels / pixelDensity
    }
}
