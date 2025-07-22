//
//  UICoordinateSystemBridgeSwift.swift
//  FloppyTurd
//
//  Swift bridge functions for UICoordinateSystem C++ integration
//  Created by Carl the Code-Conjuring Turdsmith
//

import Foundation
#if canImport(UIKit)
import UIKit
#endif



// MARK: - C++ Bridge Functions
// Note: These functions provide fallback implementations for C++ calls
// since @MainActor functions cannot be called directly from C++

/// Bridge function for GetPixelScreenRect() - called from C++
public func UICoordinateSystem_GetPixelScreenRect() -> Rectangle {
    // Provide fallback since C++ cannot call @MainActor functions
    #if canImport(UIKit)
    // iPhone 16 simulator default resolution
    return Rectangle(x: 0, y: 0, width: 1080, height: 1920)
    #else
    return Rectangle(x: 0, y: 0, width: 1080, height: 1920)
    #endif
}

/// Bridge function for GetPointScreenRect() - called from C++
public func UICoordinateSystem_GetPointScreenRect() -> Rectangle {
    // Provide fallback since C++ cannot call @MainActor functions
    #if canImport(UIKit)
    // iPhone 16 simulator default points
    return Rectangle(x: 0, y: 0, width: 375, height: 667)
    #else
    return Rectangle(x: 0, y: 0, width: 375, height: 667)
    #endif
}

/// Bridge function for GetSafeAreaRect(bool) - called from C++
public func UICoordinateSystem_GetSafeAreaRect(inPixels: Bool) -> Rectangle {
    // Provide fallback since C++ cannot call @MainActor functions
    if inPixels {
        // Safe area in pixels (accounting for notch/home indicator)
        return Rectangle(x: 0, y: 132, width: 1080, height: 1656)
    } else {
        // Safe area in points
        return Rectangle(x: 0, y: 44, width: 375, height: 552)
    }
}

// MARK: - Helper Class for Main Actor Handling

/// Helper class to handle UIKit calls properly
public class UICoordinateSystemHelper {
    
    @MainActor
    public static func getPixelScreenRect() -> Rectangle {
        #if canImport(UIKit)
        let screen = UIScreen.main
        let bounds = screen.bounds
        let scale = screen.scale
        return Rectangle(
            x: 0, 
            y: 0, 
            width: Float(bounds.width * scale), 
            height: Float(bounds.height * scale)
        )
        #else
        return Rectangle(x: 0, y: 0, width: 1080, height: 1920)
        #endif
    }
    
    @MainActor
    public static func getPointScreenRect() -> Rectangle {
        #if canImport(UIKit)
        let screen = UIScreen.main
        let bounds = screen.bounds
        return Rectangle(
            x: 0, 
            y: 0, 
            width: Float(bounds.width), 
            height: Float(bounds.height)
        )
        #else
        return Rectangle(x: 0, y: 0, width: 375, height: 667)
        #endif
    }
    
    @MainActor
    public static func getSafeAreaRect(inPixels: Bool) -> Rectangle {
        #if canImport(UIKit)
        // Try to get safe area from UIManagerSwift if available
        let uiManager = UIManagerSwift.shared
        let safeArea = uiManager.getSafeAreaRect()
        
        if inPixels {
            let scale = UIScreen.main.scale
            return Rectangle(
                x: safeArea.x * Float(scale),
                y: safeArea.y * Float(scale),
                width: safeArea.width * Float(scale),
                height: safeArea.height * Float(scale)
            )
        } else {
            return safeArea
        }
        #else
        let screenRect = inPixels ? getPixelScreenRect() : getPointScreenRect()
        return Rectangle(x: 0, y: 0, width: screenRect.width, height: screenRect.height)
        #endif
    }
}