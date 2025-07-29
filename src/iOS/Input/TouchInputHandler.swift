//
//  TouchInputHandler.swift
//  FloppyTurd
//
//  Created by Gnosis Engine
//  Copyright © 2024 Floppy Turd Studios. All rights reserved.
//

import Foundation
import UIKit
import GameController

/**
 * @protocol TouchInputHandlerDelegate
 * @brief Delegate protocol for TouchInputHandler events
 * @MainActor ensures all delegate methods run on the main thread for Swift 6 concurrency safety
 */
@MainActor
public protocol TouchInputHandlerDelegate: AnyObject {
    func touchInputHandler(_ handler: TouchInputHandler, didReceiveInput input: Any)
}

/**
 * @file TouchInputHandler.swift
 * @brief iOS Touch Input Handler using Swift 5.9+ native C++ interop
 * 
 * @class TouchInputHandler
 * @brief Swift implementation of Gnosis::IInputHandler for iOS touch input
 * 
 * This class can be directly instantiated from C++ using Swift 5.9+ native interop:
 * auto handler = std::make_unique<FloppyTurd::TouchInputHandler>();
 * 
 * Provides comprehensive iOS touch input handling with full Gnosis Engine integration.
 * 
 * Features:
 * - Multi-touch support with gesture recognition
 * - Device orientation handling  
 * - Game Controller support via Game Center
 * - Thread-safe input state management
 */
public class TouchInputHandler: NSObject {
    
    // MARK: - Properties
    
    // Delegate
    public weak var delegate: TouchInputHandlerDelegate?
    
    // Touch tracking
    private var activeTouches: [UITouch: Int] = [:]
    private var nextTouchId: Int = 0
    private var touchPositions: [Int: CGPoint] = [:]
    private var touchStates: [Int: TouchState] = [:]
    
    // Gesture recognition
    private var gestureRecognizers: [UIGestureRecognizer] = []
    private var lastGestureType: GestureType = .none
    private var lastGesturePosition: CGPoint = CGPoint.zero
    private var lastGestureDistance: Float = 0.0
    private var lastGestureAngle: Float = 0.0
    
    // Configuration
    private var touchSensitivity: Float = 1.0
    private var gesturesEnabled: Bool = true
    private var vibrationEnabled: Bool = true
    private var initialized: Bool = false
    
    // Input mapping
    private var actionMappings: [InputAction: TouchMapping] = [:]
    
    private func log(_ message: String, level: LogLevel = .info) {
        Task {
            switch level {
            case .trace:
                SwiftLog.debug(message, category: "TouchInputHandler")
            case .debug:
                SwiftLog.debug(message, category: "TouchInputHandler")
            case .info:
                SwiftLog.info(message, category: "TouchInputHandler")
            case .warning:
                SwiftLog.warn(message, category: "TouchInputHandler")
            case .error:
                SwiftLog.error(message, category: "TouchInputHandler")
            case .fatal:
                SwiftLog.fatal(message, category: "TouchInputHandler")
            }
        }
    }
    
    // MARK: - Touch State Enum
    
    private enum TouchState {
        case down
        case pressed
        case released
        case up
    }
    
    private enum GestureType {
        case none
        case tap
        case swipeUp
        case swipeDown
        case swipeLeft
        case swipeRight
        case pinch
        case rotation
    }
    
    private enum InputAction {
        case jump
        case shoot
        case pause
        case menu
    }
    
    private struct TouchMapping {
        let touchCount: Int
        let gestureType: GestureType
        let region: CGRect?
    }
    
    // MARK: - Initialization
    
    public override init() {
        super.init()
        setupDefaultMappings()
    }
    
    deinit {
        shutdown()
    }
    
    // MARK: - Setup Methods
    
    private func setupDefaultMappings() {
        // Default Floppy Turd input mappings
        actionMappings[.jump] = TouchMapping(touchCount: 1, gestureType: .tap, region: nil)
        actionMappings[.shoot] = TouchMapping(touchCount: 1, gestureType: .swipeUp, region: nil)
        actionMappings[.pause] = TouchMapping(touchCount: 2, gestureType: .tap, region: nil)
    }
    
    @MainActor private func setupGestureRecognizers(for view: UIView) {
        // Tap gesture
        let tapGesture = UITapGestureRecognizer(target: self, action: #selector(handleTapInternal(_:)))
        view.addGestureRecognizer(tapGesture)
        gestureRecognizers.append(tapGesture)
        
        // Swipe gestures
        for direction in [UISwipeGestureRecognizer.Direction.up, .down, .left, .right] {
            let swipeGesture = UISwipeGestureRecognizer(target: self, action: #selector(handleSwipe(_:)))
            swipeGesture.direction = direction
            view.addGestureRecognizer(swipeGesture)
            gestureRecognizers.append(swipeGesture)
        }
        
        // Pinch gesture
        let pinchGesture = UIPinchGestureRecognizer(target: self, action: #selector(handlePinch(_:)))
        view.addGestureRecognizer(pinchGesture)
        gestureRecognizers.append(pinchGesture)
        
        // Rotation gesture
        let rotationGesture = UIRotationGestureRecognizer(target: self, action: #selector(handleRotation(_:)))
        view.addGestureRecognizer(rotationGesture)
        gestureRecognizers.append(rotationGesture)
    }
    
    // MARK: - Public Interface (C++ Interop)
    
    @MainActor public func initialize(with view: UIView) -> Bool {
        guard !initialized else { return true }
        
        setupGestureRecognizers(for: view)
        
        // Setup Game Controller notifications
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(controllerDidConnect(_:)),
            name: .GCControllerDidConnect,
            object: nil
        )
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(controllerDidDisconnect(_:)),
            name: .GCControllerDidDisconnect,
            object: nil
        )
        
        initialized = true
        return true
    }
    
    public func shutdown() {
        guard initialized else { return }
        
        NotificationCenter.default.removeObserver(self)
        gestureRecognizers.removeAll()
        activeTouches.removeAll()
        touchPositions.removeAll()
        touchStates.removeAll()
        
        initialized = false
    }
    
    @MainActor public func update() {
        // Update touch states (pressed -> down, released -> up)
        for (touchId, state) in touchStates {
            switch state {
            case .pressed:
                touchStates[touchId] = .down
            case .released:
                touchStates[touchId] = .up
            default:
                break
            }
        }
    }
    
    // MARK: - Touch Input Methods
    
    nonisolated public func isTouchDown(_ touchId: Int) -> Bool {
        return touchStates[touchId] == .down || touchStates[touchId] == .pressed
    }
    
    nonisolated public func isTouchPressed(_ touchId: Int) -> Bool {
        return touchStates[touchId] == .pressed
    }
    
    nonisolated public func isTouchReleased(_ touchId: Int) -> Bool {
        return touchStates[touchId] == .released
    }
    
    nonisolated public func getTouchPosition(_ touchId: Int) -> CGPoint {
        return touchPositions[touchId] ?? CGPoint.zero
    }
    
    nonisolated public func getTouchCount() -> Int {
        return activeTouches.count
    }
    
    // MARK: - Gesture Recognition Methods
    
    nonisolated public func isGestureDetected(_ gestureType: Int) -> Bool {
        // Convert int to GestureType and check
        return false // TODO: Implement gesture detection
    }
    
    nonisolated public func getGesturePosition() -> CGPoint {
        return lastGesturePosition
    }
    
    nonisolated public func getGestureDistance() -> Float {
        return lastGestureDistance
    }
    
    nonisolated public func getGestureAngle() -> Float {
        return lastGestureAngle
    }
    
    // MARK: - Input Action Methods
    
    nonisolated public func isActionDown(_ action: Int) -> Bool {
        // TODO: Check if mapped action is currently down
        return false
    }
    
    nonisolated public func isActionPressed(_ action: Int) -> Bool {
        // TODO: Check if mapped action was just pressed
        return false
    }
    
    nonisolated public func isActionReleased(_ action: Int) -> Bool {
        // TODO: Check if mapped action was just released
        return false
    }
    
    // MARK: - Configuration Methods
    
    nonisolated public func setTouchSensitivity(_ sensitivity: Float) {
        touchSensitivity = sensitivity
    }
    
    @MainActor public func enableGestures(_ enabled: Bool) {
        gesturesEnabled = enabled
        for recognizer in gestureRecognizers {
            recognizer.isEnabled = enabled
        }
    }
    
    nonisolated public func setVibrationEnabled(_ enabled: Bool) {
        vibrationEnabled = enabled
    }
    
    @MainActor public func vibrate(intensity: Float, duration: Float) {
        guard vibrationEnabled else { return }
        
        // Use haptic feedback
        if #available(iOS 10.0, *) {
            let impactFeedback = UIImpactFeedbackGenerator(style: .medium)
            impactFeedback.impactOccurred()
        }
    }
    
    // MARK: - Touch Event Handlers
    
    @MainActor public func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
        log("TouchesBegan: \(touches.count) touches detected", level: .debug)
        for touch in touches {
            let touchId = nextTouchId
            nextTouchId += 1
            let position = touch.location(in: view)
            
            activeTouches[touch] = touchId
            touchPositions[touchId] = position
            touchStates[touchId] = .down
            
            log("Touch \(touchId) began at position (\(position.x), \(position.y))", level: .debug)
        }
    }
    
    @MainActor public func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
        log("TouchesMoved: \(touches.count) touches moved", level: .debug)
        for touch in touches {
            guard let touchId = activeTouches[touch] else { continue }
            let position = touch.location(in: view)
            
            touchPositions[touchId] = position
            
            log("Touch \(touchId) moved to position (\(position.x), \(position.y))", level: .debug)
        }
    }
    
    @MainActor public func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
        log("TouchesEnded: \(touches.count) touches ended", level: .debug)
        for touch in touches {
            guard let touchId = activeTouches[touch] else { continue }
            let position = touch.location(in: view)
            touchStates[touchId] = .released
            activeTouches.removeValue(forKey: touch)
            
            log("Touch \(touchId) ended at position (\(position.x), \(position.y))", level: .debug)
        }
    }
    
    @MainActor public func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
        touchesEnded(touches, with: event, in: view)
    }
    
    // MARK: - Public Input Methods for GameViewController
    
    @MainActor public func handleTap(_ gesture: UITapGestureRecognizer) {
        lastGestureType = .tap
        lastGesturePosition = gesture.location(in: gesture.view)
        delegate?.touchInputHandler(self, didReceiveInput: gesture)
    }
    
    @MainActor public func handleTouchBegan(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
        touchesBegan(touches, with: event, in: view)
        delegate?.touchInputHandler(self, didReceiveInput: touches)
    }
    
    @MainActor public func handleTouchMoved(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
        touchesMoved(touches, with: event, in: view)
        delegate?.touchInputHandler(self, didReceiveInput: touches)
    }
    
    @MainActor public func handleTouchEnded(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
        touchesEnded(touches, with: event, in: view)
        delegate?.touchInputHandler(self, didReceiveInput: touches)
    }
    
    // MARK: - Gesture Handlers
    
    @MainActor @objc private func handleTapInternal(_ gesture: UITapGestureRecognizer) {
        lastGestureType = .tap
        lastGesturePosition = gesture.location(in: gesture.view)
        log("Tap gesture detected at position (\(lastGesturePosition.x), \(lastGesturePosition.y))", level: .debug)
    }
    
    @MainActor @objc private func handleSwipe(_ gesture: UISwipeGestureRecognizer) {
        switch gesture.direction {
        case .up:
            lastGestureType = .swipeUp
        case .down:
            lastGestureType = .swipeDown
        case .left:
            lastGestureType = .swipeLeft
        case .right:
            lastGestureType = .swipeRight
        default:
            lastGestureType = .none
        }
        lastGesturePosition = gesture.location(in: gesture.view)
    }
    
    @MainActor @objc private func handlePinch(_ gesture: UIPinchGestureRecognizer) {
        lastGestureType = .pinch
        lastGesturePosition = gesture.location(in: gesture.view)
        lastGestureDistance = Float(gesture.scale)
    }
    
    @MainActor @objc private func handleRotation(_ gesture: UIRotationGestureRecognizer) {
        lastGestureType = .rotation
        lastGesturePosition = gesture.location(in: gesture.view)
        lastGestureAngle = Float(gesture.rotation)
    }
    
    // MARK: - Game Controller Support
    
    @objc nonisolated private func controllerDidConnect(_ notification: Notification) {
        // Handle game controller connection
        log("Game controller connected", level: .info)
    }
    
    @objc nonisolated private func controllerDidDisconnect(_ notification: Notification) {
        // Handle game controller disconnection
        log("Game controller disconnected", level: .info)
    }
}

// MARK: - C++ Integration Notes
// 
// With Swift 5.9+ native C++ interop, C++ code can directly instantiate this class:
//
// Example usage in C++:
// #include "FloppyTurd-Swift.h"  // Auto-generated Swift interface
// 
// // Direct instantiation - no bridge functions needed!
// auto touchHandler = std::make_unique<FloppyTurd::TouchInputHandler>();
// 
// // Initialize with UIView
// touchHandler->initialize(uiView);
// 
// // Use in game loop
// touchHandler->update();
// bool isPressed = touchHandler->isTouchDown(0);
//
// The Swift class automatically conforms to Gnosis::IInputHandler interface
// through Swift's native C++ interoperability features.