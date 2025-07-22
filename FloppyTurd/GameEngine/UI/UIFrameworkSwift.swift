//
//  UIFrameworkSwift.swift
//  Professional Game Engine - UI Layer
//
//  Created by Swift Architecture Migration
//  Pure Swift UI rendering with MetalRendererSwift integration
//

import Foundation
#if canImport(UIKit)
import UIKit
#endif

/// Professional UI framework providing native Swift UI integration
/// Game-agnostic UI components with Metal rendering calls
@MainActor
public class UIFramework {
    
    // MARK: - Properties
    private static var isInitialized: Bool = false
    private static var metalRenderer: MetalRendererSwift?
    private static var textRenderer: MetalTextRendererSwift?
    
    // UI state management
    private static var activeElements: [String: UIElement] = [:]
    private static var elementHierarchy: [UIElement] = []
    private static var focusedElement: UIElement?
    
    // Layout properties
private static var screenSize: Vector2 = Vector2(x: 0, y: 0)
    private static var safeAreaInsets: UIEdgeInsets = .zero
    private static var pixelDensity: Float = 1.0
    
    // MARK: - UI Element Protocol
    
    /// Base protocol for all UI elements in the game engine
    public protocol UIElement: AnyObject {
        var id: String { get }
        var position: Vector2 { get set }
        var size: Vector2 { get set }
        var isVisible: Bool { get set }
        var isInteractive: Bool { get set }
        var parentElement: UIElement? { get set }
        var childElements: [UIElement] { get set }

        @MainActor func render()
        func update(deltaTime: Float)
        func handleTouch(_ touchData: InputEngine.TouchData) -> Bool
        func getBounds() -> Rectangle
    }
    
    // MARK: - Button Component
    
    /// Professional button component with C++ rendering
    public class Button: UIElement {
        public let id: String
        public var position: Vector2
        public var size: Vector2
        public var isVisible: Bool = true
        public var isInteractive: Bool = true
        public var parentElement: UIElement?
        public var childElements: [UIElement] = []
        
        // Button-specific properties
        public var text: String
        public var normalColor: RaylibColor
        public var hoverColor: RaylibColor
        public var pressedColor: RaylibColor
        public var textColor: RaylibColor
        public var fontSize: Float
        public var isPressed: Bool = false
        public var isHovered: Bool = false
        
        // Callbacks
        public var onPressed: (() -> Void)?
        public var onReleased: (() -> Void)?
        public var onHover: (() -> Void)?
        
        public init(id: String, 
                   position: Vector2, 
                   size: Vector2, 
                   text: String,
                   normalColor: RaylibColor = RaylibColor(r: 51, g: 153, b: 255, a: 255),
                   textColor: RaylibColor = RaylibColor.white) {
            self.id = id
            self.position = position
            self.size = size
            self.text = text
            self.normalColor = normalColor
            self.hoverColor = RaylibColor(r: min(255, UInt8(Double(normalColor.r) * 1.2)), 
                                  g: min(255, UInt8(Double(normalColor.g) * 1.2)), 
                                  b: min(255, UInt8(Double(normalColor.b) * 1.2)), 
                                  a: normalColor.a)
            self.pressedColor = RaylibColor(r: UInt8(Double(normalColor.r) * 0.8), 
                                    g: UInt8(Double(normalColor.g) * 0.8), 
                                    b: UInt8(Double(normalColor.b) * 0.8), 
                                    a: normalColor.a)
            self.textColor = textColor
            self.fontSize = 20.0
        }
        
        public func render() {
            guard isVisible else { return }
            
            // Determine current color based on state
            let currentColor: RaylibColor
            if isPressed {
                currentColor = pressedColor
            } else if isHovered {
                currentColor = hoverColor
            } else {
                currentColor = normalColor
            }
            
            // Use MetalRendererSwift for drawing
            if let renderer = UIFramework.metalRenderer {
                renderer.drawRectangle(x: position.x, y: position.y, width: size.x, height: size.y, color: currentColor)
                
                // Draw border using lines
                let borderColor = RaylibColor(r: 0, g: 0, b: 0, a: 76)
                renderer.drawLine(startX: position.x, startY: position.y, endX: position.x + size.x, endY: position.y, color: borderColor)
                renderer.drawLine(startX: position.x + size.x, startY: position.y, endX: position.x + size.x, endY: position.y + size.y, color: borderColor)
                renderer.drawLine(startX: position.x + size.x, startY: position.y + size.y, endX: position.x, endY: position.y + size.y, color: borderColor)
                renderer.drawLine(startX: position.x, startY: position.y + size.y, endX: position.x, endY: position.y, color: borderColor)
            }
            
            // Render button text with MetalTextRenderer
            let textPosition = Vector2(
                x: position.x + (size.x - Float(text.count) * fontSize * 0.6) / 2,
                y: position.y + (size.y - fontSize) / 2
            )
            if let textRenderer = UIFramework.textRenderer {
                textRenderer.drawText(text, x: textPosition.x, y: textPosition.y, fontSize: fontSize, color: textColor)
            }
        }
        
        
        public func update(deltaTime: Float) {
            // Update button animation or state if needed
            // For now, buttons are static
        }
        
        public func handleTouch(_ touchData: InputEngine.TouchData) -> Bool {
            guard isInteractive && isVisible else { return false }
            
            let bounds = getBounds()
            let touchInBounds = bounds.contains(point: touchData.position)
            
            switch touchData.phase {
            case .began:
                if touchInBounds {
                    isPressed = true
                    onPressed?()
                    return true
                }
                
            case .moved:
                isHovered = touchInBounds
                if touchInBounds {
                    onHover?()
                }
                
            case .ended:
                if isPressed && touchInBounds {
                    onReleased?()
                }
                isPressed = false
                isHovered = false
                return touchInBounds
                
            case .cancelled:
                isPressed = false
                isHovered = false
            }
            
            return false
        }
        
        public func getBounds() -> Rectangle {
            return Rectangle(x: position.x, y: position.y, width: size.x, height: size.y)
        }
    }
    
    // MARK: - Label Component
    
    /// Professional text label component with C++ rendering
    public class Label: UIElement {
        public let id: String
        public var position: Vector2
        public var size: Vector2
        public var isVisible: Bool = true
        public var isInteractive: Bool = false
        public var parentElement: UIElement?
        public var childElements: [UIElement] = []
        
        // Label-specific properties
        public var text: String
        public var textColor: RaylibColor
        public var fontSize: Float
        public var alignment: TextAlignment
        
        public enum TextAlignment {
            case left, center, right
        }
        
        public init(id: String, 
                   position: Vector2, 
                   size: Vector2, 
                   text: String,
                   textColor: RaylibColor = RaylibColor.white,
                   fontSize: Float = 16.0,
                   alignment: TextAlignment = .left) {
            self.id = id
            self.position = position
            self.size = size
            self.text = text
            self.textColor = textColor
            self.fontSize = fontSize
            self.alignment = alignment
        }
        
        public func render() {
            guard isVisible else { return }
            
            // Calculate text position based on alignment
            let textWidth = Float(text.count) * fontSize * 0.6 // Approximate text width
            let textPosition: Vector2
            
            switch alignment {
            case .left:
                textPosition = Vector2(x: position.x, y: position.y + (size.y - fontSize) / 2)
            case .center:
                textPosition = Vector2(x: position.x + (size.x - textWidth) / 2, y: position.y + (size.y - fontSize) / 2)
            case .right:
                textPosition = Vector2(x: position.x + size.x - textWidth, y: position.y + (size.y - fontSize) / 2)
            }
            
            // Render text with MetalTextRenderer
            if let textRenderer = UIFramework.textRenderer {
                textRenderer.drawText(text, x: textPosition.x, y: textPosition.y, fontSize: fontSize, color: textColor)
            }
        }
        
        public func update(deltaTime: Float) {
            // Labels are typically static
        }
        
        public func handleTouch(_ touchData: InputEngine.TouchData) -> Bool {
            return false // Labels are not interactive by default
        }
        
        public func getBounds() -> Rectangle {
            return Rectangle(x: position.x, y: position.y, width: size.x, height: size.y)
        }
    }
    
    // MARK: - Panel Component
    
    /// Professional panel component for UI layout
    public class Panel: UIElement {
        public let id: String
        public var position: Vector2
        public var size: Vector2
        public var isVisible: Bool = true
        public var isInteractive: Bool = false
        public var parentElement: UIElement?
        public var childElements: [UIElement] = []
        
        // Panel-specific properties
        public var backgroundColor: RaylibColor
        public var borderColor: RaylibColor
        public var borderWidth: Float
        public var cornerRadius: Float
        
        public init(id: String, 
                   position: Vector2, 
                   size: Vector2,
                   backgroundColor: RaylibColor = RaylibColor(r: 25, g: 25, b: 25, a: 204),
                   borderColor: RaylibColor = RaylibColor(r: 76, g: 76, b: 76, a: 255),
                   borderWidth: Float = 2.0,
                   cornerRadius: Float = 8.0) {
            self.id = id
            self.position = position
            self.size = size
            self.backgroundColor = backgroundColor
            self.borderColor = borderColor
            self.borderWidth = borderWidth
            self.cornerRadius = cornerRadius
        }
        
        public func render() {
            guard isVisible else { return }
            
            // Render panel background with MetalRenderer
            if let renderer = UIFramework.metalRenderer {
                renderer.drawRectangle(x: position.x, y: position.y, width: size.x, height: size.y, color: backgroundColor)
                
                // Draw border using lines if borderWidth > 0
                if borderWidth > 0 {
                    renderer.drawLine(startX: position.x, startY: position.y, endX: position.x + size.x, endY: position.y, color: borderColor)
                    renderer.drawLine(startX: position.x + size.x, startY: position.y, endX: position.x + size.x, endY: position.y + size.y, color: borderColor)
                    renderer.drawLine(startX: position.x + size.x, startY: position.y + size.y, endX: position.x, endY: position.y + size.y, color: borderColor)
                    renderer.drawLine(startX: position.x, startY: position.y + size.y, endX: position.x, endY: position.y, color: borderColor)
                }
            }
            
            // Render child elements
            for child in childElements {
                child.render()
            }
        }
        
        public func update(deltaTime: Float) {
            // Update all child elements
            for child in childElements {
                child.update(deltaTime: deltaTime)
            }
        }
        
        public func handleTouch(_ touchData: InputEngine.TouchData) -> Bool {
            guard isVisible else { return false }
            
            // Check child elements first (reverse order for proper z-order)
            for child in childElements.reversed() {
                if child.handleTouch(touchData) {
                    return true
                }
            }
            
            // Check if touch is within panel bounds
            let bounds = getBounds()
            return bounds.contains(point: touchData.position)
        }
        
        public func getBounds() -> Rectangle {
            return Rectangle(x: position.x, y: position.y, width: size.x, height: size.y)
        }
        
        // Panel-specific methods
        public func addChild(_ element: UIElement) {
            element.parentElement = self
            childElements.append(element)
        }
        
        public func removeChild(_ element: UIElement) {
            element.parentElement = nil
            childElements.removeAll { $0.id == element.id }
        }
    }
    
    // MARK: - Static Framework Methods
    
    /// Initialize the UI framework with Swift managers
    /// Pure Swift implementation - no C++ bridge required
    public static func initialize(screenSize: Vector2, safeAreaInsets: UIEdgeInsets, pixelDensity: Float) {
        traceLog(SWLogLevel.SWLOG_INFO, "[UIFramework] Initializing with pure Swift architecture")
        
        UIFramework.screenSize = screenSize
        UIFramework.safeAreaInsets = safeAreaInsets
        UIFramework.pixelDensity = pixelDensity
        
        // Use UIManagerSwift singleton instead of C++ UIManager
        let uiManager = UIManagerSwift.shared
        uiManager.updateScreenInfo()
        
        // Create MetalRendererSwift instance
        metalRenderer = MetalRendererSwift()
        textRenderer = MetalTextRendererSwift()
        
        if metalRenderer != nil {
            traceLog(SWLogLevel.SWLOG_INFO, "[UIFramework] Initialized with MetalRendererSwift")
        } else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[UIFramework] MetalRendererSwift initialization failed")
        }
        
        if textRenderer != nil {
            traceLog(SWLogLevel.SWLOG_INFO, "[UIFramework] Initialized with MetalTextRendererSwift")
        } else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[UIFramework] MetalTextRendererSwift initialization failed")
        }
        
        isInitialized = true
        traceLog(SWLogLevel.SWLOG_INFO, "[UIFramework] UI framework initialization complete")
    }
    
    // MARK: - Element Management
    
    /// Add UI element to the framework
    public static func addElement(_ element: UIElement) {
        activeElements[element.id] = element
        
        if element.parentElement == nil {
            elementHierarchy.append(element)
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[UIFramework] Added UI element: \(element.id)")
    }
    
    /// Remove UI element from the framework
    public static func removeElement(_ elementId: String) {
        if let element = activeElements.removeValue(forKey: elementId) {
            elementHierarchy.removeAll { $0.id == elementId }
            
            // Remove from parent if it has one
            if let parent = element.parentElement as? Panel {
                parent.removeChild(element)
            }
            
            traceLog(SWLogLevel.SWLOG_INFO, "[UIFramework] Removed UI element: \(elementId)")
        }
    }
    
    /// Get UI element by ID
    public static func getElement(_ elementId: String) -> UIElement? {
        return activeElements[elementId]
    }
    
    /// Clear all UI elements
    public static func clearAllElements() {
        activeElements.removeAll()
        elementHierarchy.removeAll()
        focusedElement = nil
        
        traceLog(SWLogLevel.SWLOG_INFO, "[UIFramework] Cleared all UI elements")
    }
    
    // MARK: - Rendering
    
    /// Render all UI elements - direct C++ calls
    @MainActor
    public static func render() {
        guard isInitialized else { return }
        
        // Render all root elements (they will render their children)
        for element in elementHierarchy {
            element.render()
        }
    }
    
    /// Update all UI elements
    public static func update(deltaTime: Float) {
        guard isInitialized else { return }
        
        // Update all root elements (they will update their children)
        for element in elementHierarchy {
            element.update(deltaTime: deltaTime)
        }
    }
    
    // MARK: - Input Handling
    
    /// Handle touch input for all UI elements
    public static func handleTouch(_ touchData: InputEngine.TouchData) -> Bool {
        guard isInitialized else { return false }
        
        // Process elements in reverse order for proper z-order
        for element in elementHierarchy.reversed() {
            if element.handleTouch(touchData) {
                focusedElement = element
                return true
            }
        }
        
        focusedElement = nil
        return false
    }
    
    // MARK: - Layout Utilities
    
    /// Get safe area rectangle
    public static func getSafeAreaBounds() -> Rectangle {
        return Rectangle(
            x: Float(safeAreaInsets.left),
            y: Float(safeAreaInsets.top),
            width: screenSize.x - Float(safeAreaInsets.left + safeAreaInsets.right),
            height: screenSize.y - Float(safeAreaInsets.top + safeAreaInsets.bottom)
        )
    }
    
    /// Convert points to pixels
    public static func pointsToPixels(_ points: Float) -> Float {
        return points * pixelDensity
    }
    
    /// Convert pixels to points
    public static func pixelsToPoints(_ pixels: Float) -> Float {
        return pixels / pixelDensity
    }
    
    /// Get screen center position
    public static func getScreenCenter() -> Vector2 {
        return Vector2(x: screenSize.x / 2, y: screenSize.y / 2)
    }
    
    // MARK: - Cleanup
    
    /// Shutdown UI framework - cleanup resources
    public static func shutdown() {
        traceLog(SWLogLevel.SWLOG_INFO, "[UIFramework] Shutting down UI framework")
        
        // Clear all elements
        clearAllElements()
        
        // Reset state
        isInitialized = false
        metalRenderer = nil
        textRenderer = nil
        
        traceLog(SWLogLevel.SWLOG_INFO, "[UIFramework] UI framework shutdown complete")
    }
}

// MARK: - UI Framework Extensions

extension UIFramework {
    
    /// Create a button with automatic positioning
    public static func createButton(id: String, 
                                  text: String, 
                                  centerPosition: Vector2,
                                  size: Vector2? = nil,
                                  onPressed: (() -> Void)? = nil) -> Button {
        let buttonSize = size ?? Vector2(x: 200, y: 50)
        let position = Vector2(x: centerPosition.x - buttonSize.x / 2, y: centerPosition.y - buttonSize.y / 2)
        
        let button = Button(id: id, position: position, size: buttonSize, text: text)
        button.onPressed = onPressed
        
        addElement(button)
        return button
    }
    
    /// Create a label with automatic sizing
    public static func createLabel(id: String, 
                                 text: String, 
                                 position: Vector2,
                                 fontSize: Float = 16.0,
                                 alignment: Label.TextAlignment = .left) -> Label {
        let estimatedWidth = Float(text.count) * fontSize * 0.7
        let size = Vector2(x: estimatedWidth, y: fontSize * 1.5)
        
        let label = Label(id: id, position: position, size: size, text: text, fontSize: fontSize, alignment: alignment)
        
        addElement(label)
        return label
    }
    
    /// Create a panel that fills the safe area
    public static func createSafeAreaPanel(id: String) -> Panel {
        let safeArea = getSafeAreaBounds()
        let position = Vector2(x: safeArea.x, y: safeArea.y)
        let size = Vector2(x: safeArea.width, y: safeArea.height)
        
        let panel = Panel(id: id, position: position, size: size)
        
        addElement(panel)
        return panel
    }
}
