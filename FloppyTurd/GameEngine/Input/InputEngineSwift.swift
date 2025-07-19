//
//  InputEngineSwift.swift
//  Professional Game Engine - Input Layer
//
//  Created for iOS Input Management
//  Pure Swift implementation with C++ interop exports
//

import Foundation
#if canImport(UIKit)
import UIKit
#endif

/// iOS Input Engine - Pure Swift implementation
/// Handles UIKit touch events and provides C++ interop functions
@MainActor
public class InputEngine {
    
    // MARK: - Properties
    private static var isInitialized: Bool = false
    
    // Touch state tracking
    private static var activeTouches: [UITouch: TouchData] = [:]
    private static var touchBeganCallbacks: [(TouchData) -> Void] = []
    private static var touchMovedCallbacks: [(TouchData) -> Void] = []
    private static var touchEndedCallbacks: [(TouchData) -> Void] = []
    
    // Input state cache
    private static var screenSize: CGSize = .zero
    private static var safeAreaInsets: UIEdgeInsets = .zero
    
    // Current input state cache for C++ queries - nonisolated(unsafe) for thread-safe C++ interop access
    nonisolated(unsafe) static var currentTouchPositions: [Vector2] = []
    nonisolated(unsafe) static var isMousePressed: Bool = false
    nonisolated(unsafe) static var mousePosition: Vector2 = Vector2(x: 0, y: 0)
    
    // MARK: - Touch Data Structure
    
    /// Touch data structure that mirrors C++ TouchData
    public struct TouchData {
        public let id: Int
        public let position: Vector2
        public let normalizedPosition: Vector2
        public let pressure: Float
        public let timestamp: Double
        public let phase: TouchPhase
        
        public enum TouchPhase: Int32 {
            case began = 0
            case moved = 1
            case ended = 2
            case cancelled = 3
        }
        
        public init(id: Int, position: Vector2, normalizedPosition: Vector2, pressure: Float, timestamp: Double, phase: TouchPhase) {
            self.id = id
            self.position = position
            self.normalizedPosition = normalizedPosition
            self.pressure = pressure
            self.timestamp = timestamp
            self.phase = phase
        }
    }
    
    // MARK: - Initialization
    
    /// Initialize the input engine for iOS
    public static func initialize(screenSize: CGSize, safeAreaInsets: UIEdgeInsets) {
        print("[InputEngine] Initializing iOS input system")
        
        self.screenSize = screenSize
        self.safeAreaInsets = safeAreaInsets
        
        // Reset state
        activeTouches.removeAll()
        currentTouchPositions.removeAll()
        isMousePressed = false
        mousePosition = Vector2(x: 0, y: 0)
        
        isInitialized = true
        print("[InputEngine] iOS input engine initialization complete")
    }
    
    // MARK: - Touch Event Processing
    
    /// Process touch began events - pure Swift implementation
    public static func processTouchBegan(_ touches: Set<UITouch>, in view: UIView) {
        guard isInitialized else { return }
        
        for touch in touches {
            let touchData = createTouchData(from: touch, in: view, phase: .began)
            activeTouches[touch] = touchData
            
            // Update current state for C++ queries
            updateCurrentState()
            
            // Set mouse pressed state (treat first touch as mouse)
            if activeTouches.count == 1 {
                isMousePressed = true
                mousePosition = touchData.position
            }
            
            // Notify Swift callbacks
            for callback in touchBeganCallbacks {
                callback(touchData)
            }
            
            print("[InputEngine] Touch began: id=\(touchData.id), pos=(\(touchData.position.x), \(touchData.position.y))")
        }
    }
    
    /// Process touch moved events - pure Swift implementation
    public static func processTouchMoved(_ touches: Set<UITouch>, in view: UIView) {
        guard isInitialized else { return }
        
        for touch in touches {
            let touchData = createTouchData(from: touch, in: view, phase: .moved)
            activeTouches[touch] = touchData
            
            // Update current state for C++ queries
            updateCurrentState()
            
            // Update mouse position (use first touch as mouse)
            if activeTouches.values.first?.id == touchData.id {
                mousePosition = touchData.position
            }
            
            // Notify Swift callbacks
            for callback in touchMovedCallbacks {
                callback(touchData)
            }
        }
    }
    
    /// Process touch ended events - pure Swift implementation
    public static func processTouchEnded(_ touches: Set<UITouch>, in view: UIView) {
        guard isInitialized else { return }
        
        for touch in touches {
            let touchData = createTouchData(from: touch, in: view, phase: .ended)
            
            // Update current state for C++ queries
            activeTouches.removeValue(forKey: touch)
            updateCurrentState()
            
            // Update mouse state (if this was the mouse touch)
            if activeTouches.isEmpty {
                isMousePressed = false
            } else if activeTouches.values.first?.id != touchData.id {
                // Update mouse position to remaining touch
                if let firstTouch = activeTouches.values.first {
                    mousePosition = firstTouch.position
                }
            }
            
            // Notify Swift callbacks
            for callback in touchEndedCallbacks {
                callback(touchData)
            }
            
            print("[InputEngine] Touch ended: id=\(touchData.id), pos=(\(touchData.position.x), \(touchData.position.y))")
        }
    }
    
    /// Process touch cancelled events - pure Swift implementation
    public static func processTouchCancelled(_ touches: Set<UITouch>, in view: UIView) {
        guard isInitialized else { return }
        
        for touch in touches {
            let touchData = createTouchData(from: touch, in: view, phase: .cancelled)
            
            // Clean up active touch
            activeTouches.removeValue(forKey: touch)
            updateCurrentState()
            
            // Update mouse state
            if activeTouches.isEmpty {
                isMousePressed = false
            }
            
            print("[InputEngine] Touch cancelled: id=\(touchData.id)")
        }
    }
    
    // MARK: - Internal State Management
    
    /// Update current touch positions for C++ queries
    private static func updateCurrentState() {
        currentTouchPositions = activeTouches.values.map { $0.position }
    }
    
    // MARK: - Touch Data Conversion
    
    /// Convert UITouch to TouchData structure for C++ interop
    /// Efficient conversion with normalized coordinates
    private static func createTouchData(from touch: UITouch, in view: UIView, phase: TouchData.TouchPhase) -> TouchData {
        let location = touch.location(in: view)
        let touchId = touch.hash // Use hash as unique identifier
        
        // Create position vector
        let position = Vector2(x: Float(location.x), y: Float(location.y))
        
        // Create normalized position (0.0 to 1.0)
        let normalizedX = Float(location.x / screenSize.width)
        let normalizedY = Float(location.y / screenSize.height)
        let normalizedPosition = Vector2(x: normalizedX, y: normalizedY)
        
        // Get pressure (if available)
        var pressure: Float = 1.0
        if #available(iOS 9.0, *) {
            pressure = Float(touch.force / touch.maximumPossibleForce)
        }
        
        // Get timestamp
        let timestamp = touch.timestamp
        
        return TouchData(id: touchId,
                        position: position,
                        normalizedPosition: normalizedPosition,
                        pressure: pressure,
                        timestamp: timestamp,
                        phase: phase)
    }
    
    // MARK: - Callback Registration
    
    /// Register callback for touch began events
    public static func onTouchBegan(_ callback: @escaping (TouchData) -> Void) {
        touchBeganCallbacks.append(callback)
    }
    
    /// Register callback for touch moved events
    public static func onTouchMoved(_ callback: @escaping (TouchData) -> Void) {
        touchMovedCallbacks.append(callback)
    }
    
    /// Register callback for touch ended events
    public static func onTouchEnded(_ callback: @escaping (TouchData) -> Void) {
        touchEndedCallbacks.append(callback)
    }
    
    /// Clear all touch callbacks
    public static func clearTouchCallbacks() {
        touchBeganCallbacks.removeAll()
        touchMovedCallbacks.removeAll()
        touchEndedCallbacks.removeAll()
    }
    
    // MARK: - Input State Queries (for C++ interop)
    
    /// Check if touch is active at position
    public static func isTouchActive(at position: Vector2) -> Bool {
        guard isInitialized else { return false }
        
        let tolerance: Float = 20.0
        for touchPos in currentTouchPositions {
            let dx = touchPos.x - position.x
            let dy = touchPos.y - position.y
            if sqrt(dx * dx + dy * dy) <= tolerance {
                return true
            }
        }
        return false
    }
    
    /// Get current active touch count
    public static func getActiveTouchCount() -> Int {
        guard isInitialized else { return 0 }
        return activeTouches.count
    }
    
    /// Get all active touch positions
    public static func getActiveTouchPositions() -> [Vector2] {
        guard isInitialized else { return [] }
        return currentTouchPositions
    }
    
    // MARK: - Gesture Detection
    
    /// Check for tap gesture at position
    public static func isTapGesture(at position: Vector2, tolerance: Float = 10.0) -> Bool {
        guard isInitialized else { return false }
        return isTouchActive(at: position)
    }
    
    /// Get swipe direction (simplified - could be enhanced)
    public static func getSwipeDirection() -> Vector2? {
        guard isInitialized, !activeTouches.isEmpty else { return nil }
        
        // Simple implementation - return zero for now
        // Could be enhanced to track touch start/end positions
        return Vector2(x: 0, y: 0)
    }
    
    // MARK: - Screen Configuration
    
    /// Update screen size and safe area
    public static func updateScreenConfiguration(size: CGSize, safeAreaInsets: UIEdgeInsets) {
        self.screenSize = size
        self.safeAreaInsets = safeAreaInsets
        
        print("[InputEngine] Screen configuration updated: \(size), safe area: \(safeAreaInsets)")
    }
    
    /// Get current screen size
    public static func getScreenSize() -> CGSize {
        return screenSize
    }
    
    /// Get current safe area insets
    public static func getSafeAreaInsets() -> UIEdgeInsets {
        return safeAreaInsets
    }
    
    // MARK: - Cleanup
    
    /// Shutdown input engine - cleanup resources
    public static func shutdown() {
        print("[InputEngine] Shutting down input engine")
        
        // Clear all active touches
        activeTouches.removeAll()
        currentTouchPositions.removeAll()
        
        // Clear all callbacks
        clearTouchCallbacks()
        
        // Reset state
        isInitialized = false
        isMousePressed = false
        mousePosition = Vector2(x: 0, y: 0)
        
        print("[InputEngine] Input engine shutdown complete")
    }
}

// MARK: - C++ Interop Functions (raylib-compatible)

extension InputEngine {
    
    /// Check if mouse button is pressed (treats first touch as mouse)
    @_expose(Cxx)
    nonisolated public static func isMouseButtonPressed(_ button: Int32) -> Bool {
        return button == 0 ? isMousePressed : false // MOUSE_LEFT_BUTTON = 0
    }
    
    /// Get mouse position (uses first touch)
    @_expose(Cxx)
    nonisolated public static func getMousePosition() -> Vector2 {
        return mousePosition
    }
    
    /// Get mouse X position
    @_expose(Cxx)
    nonisolated public static func getMouseX() -> Float {
        return mousePosition.x
    }
    
    /// Get mouse Y position  
    @_expose(Cxx)
    nonisolated public static func getMouseY() -> Float {
        return mousePosition.y
    }
    
    /// Get touch position by index
    @_expose(Cxx)
    nonisolated public static func getTouchPosition(_ index: Int32) -> Vector2 {
        guard index >= 0 && index < currentTouchPositions.count else {
            return Vector2(x: 0, y: 0)
        }
        return currentTouchPositions[Int(index)]
    }
    
    /// Get touch X coordinate
    @_expose(Cxx)
    nonisolated public static func getTouchX() -> Float {
        return currentTouchPositions.first?.x ?? 0.0
    }
    
    /// Get touch Y coordinate
    @_expose(Cxx) 
    nonisolated public static func getTouchY() -> Float {
        return currentTouchPositions.first?.y ?? 0.0
    }
    
    /// Get number of current touches
    @_expose(Cxx)
    nonisolated public static func getTouchCount() -> Int32 {
        return Int32(currentTouchPositions.count)
    }
}

// MARK: - Input Engine Extensions

extension InputEngine {
    
    /// Convert screen coordinates to game world coordinates (1:1 for now)
    public static func screenToWorld(_ screenPosition: Vector2) -> Vector2 {
        guard isInitialized else { return screenPosition }
        
        // Simple 1:1 mapping - can be enhanced for camera transformations
        return screenPosition
    }
    
    /// Convert game world coordinates to screen coordinates (1:1 for now)
    public static func worldToScreen(_ worldPosition: Vector2) -> Vector2 {
        guard isInitialized else { return worldPosition }
        
        // Simple 1:1 mapping - can be enhanced for camera transformations  
        return worldPosition
    }
    
    /// Check if position is within safe area
    public static func isPositionInSafeArea(_ position: Vector2) -> Bool {
        let x = CGFloat(position.x)
        let y = CGFloat(position.y)
        
        return x >= safeAreaInsets.left &&
               x <= screenSize.width - safeAreaInsets.right &&
               y >= safeAreaInsets.top &&
               y <= screenSize.height - safeAreaInsets.bottom
    }
    
    /// Get touch position relative to safe area
    public static func getTouchInSafeArea(_ position: Vector2) -> Vector2 {
        let adjustedX = Float(position.x - Float(safeAreaInsets.left))
        let adjustedY = Float(position.y - Float(safeAreaInsets.top))
        
        return Vector2(x: adjustedX, y: adjustedY)
    }
}
