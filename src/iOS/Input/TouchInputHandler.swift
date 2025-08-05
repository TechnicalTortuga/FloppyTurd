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
 * @protocol TouchInputDelegate
 * @brief Delegate protocol for TouchInputHandler events
 */
import UIKit
import GameController
import os.log

@MainActor
public protocol TouchInputDelegate: AnyObject {
    func onTouchPress(normalizedPosition: CGPoint, viewSize: CGSize)
    func onTouchRelease(normalizedPosition: CGPoint, viewSize: CGSize)
}

// MARK: - Frame-based Input System (Generic)

struct TouchInputEvent {
    let position: CGPoint
    let viewSize: CGSize
    let isPress: Bool
    let frameNumber: Int
    let timestamp: TimeInterval
}

private class InputEventBuffer {
    private var events: [TouchInputEvent] = []
    private let minimumFrameSeparation: Int = 2  // Industry standard: 2 frames minimum for reliable input separation
    
    func addEvent(_ event: TouchInputEvent) {
        events.append(event)
    }
    
    func getEventsToProcess(currentFrame: Int) -> [TouchInputEvent] {
        let eventsToProcess = events.filter { event in
            let framesSinceEvent = currentFrame - event.frameNumber
            return framesSinceEvent >= minimumFrameSeparation
        }
        
        // Remove processed events
        events.removeAll { event in
            let framesSinceEvent = currentFrame - event.frameNumber
            return framesSinceEvent >= minimumFrameSeparation
        }
        
        return eventsToProcess.sorted { $0.frameNumber < $1.frameNumber }
    }
    
    func getBufferedEventCount() -> Int {
        return events.count
    }
    
    func clear() {
        events.removeAll()
    }
}

/**
 * @file TouchInputHandler.swift
 * @brief iOS Touch Input Handler using Swift 5.9+ native C++ interop
 * 
 * @class TouchInputHandler
 * @brief Swift implementation of robust touch input handling
 * 
 * Clean, modern implementation with @MainActor for all UI-related operations
 * and separate background processing for any non-UI work.
 */
@MainActor
public class TouchInputHandler: NSObject, UIGestureRecognizerDelegate {
    
    // MARK: - Properties
    
    // Delegate
    public weak var delegate: TouchInputDelegate?
    
    // Frame-based input system (generic - no game-specific code)
    private var currentFrameNumber: Int = 0
    private let inputEventBuffer = InputEventBuffer()
    
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
    
    // Background processor for non-UI work
    private let backgroundProcessor = TouchInputBackgroundProcessor()
    
    // Advanced input features (generic)
    // MARK: - Initialization
    
    public override init() {
        super.init()
        // TouchInputHandler is now completely generic - no default game mappings
    }
    
    deinit {
        if initialized {
            // Clean shutdown - remove observers
            NotificationCenter.default.removeObserver(self)
        }
    }
    
    private func log(_ message: String, level: LogLevel = .info) {
        // Logging is thread-safe, so we can call it directly
        backgroundProcessor.log(message, level: level, category: "TouchInputHandler")
    }
    
    // MARK: - Supporting Types
    
    private enum TouchState {
        case down
        case pressed
        case released
        case up
    }
    
    public enum GestureType: Int {
        case none = 0
        case tap = 1
        case swipeUp = 2
        case swipeDown = 3
        case swipeLeft = 4
        case swipeRight = 5
        case pinch = 6
        case rotation = 7
    }
}

// MARK: - Background Processor for Non-UI Work

/// Handles all non-UI touch processing work off the main thread
private class TouchInputBackgroundProcessor {
    
    func log(_ message: String, level: LogLevel, category: String) {
        Task.detached {
            switch level {
            case .trace:
                SwiftLog.debug(message, category: category)
            case .debug:
                SwiftLog.debug(message, category: category)
            case .info:
                SwiftLog.info(message, category: category)
            case .warning:
                SwiftLog.warn(message, category: category)
            case .error:
                SwiftLog.error(message, category: category)
            case .fatal:
                SwiftLog.fatal(message, category: category)
            }
        }
    }
    
    func processGameControllerConnection() {
        Task.detached {
            // Handle any background game controller processing
        }
    }
    
    func processGameControllerDisconnection() {
        Task.detached {
            // Handle any background game controller cleanup
        }
    }
}

// MARK: - TouchInputHandler Implementation

extension TouchInputHandler {
    
    // MARK: - Setup Methods
    
    private func setupGestureRecognizers(for view: UIView) {
        // Tap gesture
        let tapGesture = UITapGestureRecognizer(target: self, action: #selector(handleTapInternal(_:)))
        tapGesture.delegate = self
        view.addGestureRecognizer(tapGesture)
        gestureRecognizers.append(tapGesture)
        
        // Swipe gestures
        for direction in [UISwipeGestureRecognizer.Direction.up, .down, .left, .right] {
            let swipeGesture = UISwipeGestureRecognizer(target: self, action: #selector(handleSwipe(_:)))
            swipeGesture.direction = direction
            swipeGesture.delegate = self
            view.addGestureRecognizer(swipeGesture)
            gestureRecognizers.append(swipeGesture)
        }
        
        // Pinch gesture
        let pinchGesture = UIPinchGestureRecognizer(target: self, action: #selector(handlePinch(_:)))
        pinchGesture.delegate = self
        view.addGestureRecognizer(pinchGesture)
        gestureRecognizers.append(pinchGesture)
        
        // Rotation gesture
        let rotationGesture = UIRotationGestureRecognizer(target: self, action: #selector(handleRotation(_:)))
        rotationGesture.delegate = self
        view.addGestureRecognizer(rotationGesture)
        gestureRecognizers.append(rotationGesture)
    }
    
    // MARK: - Public Interface (C++ Interop)
    
    public func initialize(with view: UIView) -> Bool {
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
    
    public func handleTap(_ gesture: UITapGestureRecognizer) {
        let location = gesture.location(in: gesture.view)
        let viewSize = gesture.view?.bounds.size ?? .zero
        
        // Buffer the tap as a press+release sequence
        let pressEvent = TouchInputEvent(
            position: location,
            viewSize: viewSize,
            isPress: true,
            frameNumber: currentFrameNumber,
            timestamp: Date().timeIntervalSince1970
        )
        
        let releaseEvent = TouchInputEvent(
            position: location,
            viewSize: viewSize,
            isPress: false,
            frameNumber: currentFrameNumber + 1,  // Release on next frame
            timestamp: Date().timeIntervalSince1970
        )
        
        inputEventBuffer.addEvent(pressEvent)
        inputEventBuffer.addEvent(releaseEvent)
        
        // Also call the internal handler for immediate gesture recognition
        handleTapInternal(gesture)
    }
    
    // MARK: - Frame-based Input Processing
    
    private func processFrameBasedInput() {
        let eventsToProcess = inputEventBuffer.getEventsToProcess(currentFrame: currentFrameNumber)
        
        for event in eventsToProcess {
            // Create normalized position and pass view size to delegate
            let normalizedPosition = CGPoint(
                x: event.position.x / event.viewSize.width,
                y: event.position.y / event.viewSize.height
            )
            
            // Notify delegate with clean, processed coordinates
            if event.isPress {
                delegate?.onTouchPress(normalizedPosition: normalizedPosition, viewSize: event.viewSize)
            } else {
                delegate?.onTouchRelease(normalizedPosition: normalizedPosition, viewSize: event.viewSize)
            }
        }
    }
    
    public func update() {
        // Increment frame counter (industry standard for frame-based input)
        currentFrameNumber += 1
        
        // Process frame-based input buffer
        processFrameBasedInput()
        
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
    
    public func isTouchDown(_ touchId: Int) -> Bool {
        return touchStates[touchId] == .down || touchStates[touchId] == .pressed
    }
    
    public func isTouchPressed(_ touchId: Int) -> Bool {
        return touchStates[touchId] == .pressed
    }
    
    public func isTouchReleased(_ touchId: Int) -> Bool {
        return touchStates[touchId] == .released
    }
    
    public func getTouchPosition(_ touchId: Int) -> CGPoint {
        return touchPositions[touchId] ?? CGPoint.zero
    }
    
    public func getTouchCount() -> Int {
        return activeTouches.count
    }
    
    // MARK: - Gesture Recognition Methods
    
    public func isGestureDetected(_ gestureType: Int) -> Bool {
        // Convert int to GestureType and check
        guard let gesture = GestureType(rawValue: gestureType) else { return false }
        return lastGestureType == gesture
    }
    
    public func getGesturePosition() -> CGPoint {
        return lastGesturePosition
    }
    
    public func getGestureDistance() -> Float {
        return lastGestureDistance
    }
    
    public func getGestureAngle() -> Float {
        return lastGestureAngle
    }
    
    // MARK: - Generic Input Query Methods
    
    public func getActiveTouchCount() -> Int {
        return activeTouches.count
    }
    
    public func getLastGestureType() -> GestureType {
        return lastGestureType
    }
    
    public func getLastGesturePosition() -> CGPoint {
        return lastGesturePosition
    }
    
    // MARK: - Configuration Methods
    
    public func setTouchSensitivity(_ sensitivity: Float) {
        touchSensitivity = sensitivity
    }
    
    public func enableGestures(_ enabled: Bool) {
        gesturesEnabled = enabled
        for recognizer in gestureRecognizers {
            recognizer.isEnabled = enabled
        }
    }
    
    public func setVibrationEnabled(_ enabled: Bool) {
        vibrationEnabled = enabled
    }
    
    public func vibrate(intensity: Float, duration: Float) {
        guard vibrationEnabled else { return }
        
        // Use haptic feedback
        if #available(iOS 10.0, *) {
            let impactFeedback = UIImpactFeedbackGenerator(style: .medium)
            impactFeedback.impactOccurred()
        }
    }
    
    // MARK: - Touch Event Handlers
    
    public func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
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
        
        // Notify delegate of touch press with coordinates
        if let touch = touches.first {
            let position = touch.location(in: view)
            let viewSize = view.bounds.size
            log("🔥 TouchInputHandler: touchesBegan() - buffering touch PRESS at (\(position.x), \(position.y))", level: .debug)
            
            // Buffer touch press event for frame-based processing
            let touchEvent = TouchInputEvent(
                position: position,
                viewSize: viewSize,
                isPress: true,
                frameNumber: currentFrameNumber,
                timestamp: CACurrentMediaTime()
            )
            inputEventBuffer.addEvent(touchEvent)
        }
    }
    
    public func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
        log("TouchesMoved: \(touches.count) touches moved", level: .debug)
        for touch in touches {
            guard let touchId = activeTouches[touch] else { continue }
            let position = touch.location(in: view)
            
            touchPositions[touchId] = position
            
            log("Touch \(touchId) moved to position (\(position.x), \(position.y))", level: .debug)
        }
    }
    
    public func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
        log("TouchesEnded: \(touches.count) touches ended", level: .debug)
        for touch in touches {
            guard let touchId = activeTouches[touch] else { continue }
            let position = touch.location(in: view)
            touchStates[touchId] = .released
            activeTouches.removeValue(forKey: touch)
            
            log("Touch \(touchId) ended at position (\(position.x), \(position.y))", level: .debug)
        }
        
        // Buffer touch release event for frame-based processing
        if let touch = touches.first {
            let position = touch.location(in: view)
            let viewSize = view.bounds.size
            log("🔥 TouchInputHandler: touchesEnded() - buffering touch RELEASE at (\(position.x), \(position.y))", level: .debug)
            
            let touchEvent = TouchInputEvent(
                position: position,
                viewSize: viewSize,
                isPress: false,
                frameNumber: currentFrameNumber,
                timestamp: CACurrentMediaTime()
            )
            inputEventBuffer.addEvent(touchEvent)
        }
    }
    
    public func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?, in view: UIView) {
        touchesEnded(touches, with: event, in: view)
    }
    
    // MARK: - Gesture Handlers
    
    @objc private func handleTapInternal(_ gesture: UITapGestureRecognizer) {
        lastGestureType = .tap
        lastGesturePosition = gesture.location(in: gesture.view)
        log("Tap gesture detected at position (\(lastGesturePosition.x), \(lastGesturePosition.y))", level: .debug)
        log("Tap gesture detected at position (\(lastGesturePosition.x), \(lastGesturePosition.y))", level: .debug)
    }
    
    @objc private func handleSwipe(_ gesture: UISwipeGestureRecognizer) {
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
        
        // Update ThreadingProxy with gesture state
        log("Swipe gesture detected: \(lastGestureType) at position (\(lastGesturePosition.x), \(lastGesturePosition.y))", level: .debug)
    }
    
    @objc private func handlePinch(_ gesture: UIPinchGestureRecognizer) {
        lastGestureType = .pinch
        lastGesturePosition = gesture.location(in: gesture.view)
        lastGestureDistance = Float(gesture.scale)
    }
    
    @objc private func handleRotation(_ gesture: UIRotationGestureRecognizer) {
        lastGestureType = .rotation
        lastGesturePosition = gesture.location(in: gesture.view)
        lastGestureAngle = Float(gesture.rotation)
    }
    
    // MARK: - Game Controller Support
    
    @objc nonisolated private func controllerDidConnect(_ notification: Notification) {
        // Handle game controller connection
        Task { @MainActor in
            log("Game controller connected", level: .info)
            backgroundProcessor.processGameControllerConnection()
        }
    }
    
    @objc nonisolated private func controllerDidDisconnect(_ notification: Notification) {
        // Handle game controller disconnection
        Task { @MainActor in
            log("Game controller disconnected", level: .info)
            backgroundProcessor.processGameControllerDisconnection()
        }
    }
    
    // MARK: - Game Controller Input Methods
    
    public func isControllerConnected() -> Bool {
        return GCController.controllers().count > 0
    }
    
    public func getConnectedControllerCount() -> Int {
        return GCController.controllers().count
    }
    
    public func isControllerButtonDown(_ button: Int) -> Bool {
        guard let controller = GCController.controllers().first else { return false }
        
        // Map button integers to controller buttons
        switch button {
        case 0: // A button / Cross
            return controller.extendedGamepad?.buttonA.isPressed ?? false
        case 1: // B button / Circle  
            return controller.extendedGamepad?.buttonB.isPressed ?? false
        case 2: // X button / Square
            return controller.extendedGamepad?.buttonX.isPressed ?? false
        case 3: // Y button / Triangle
            return controller.extendedGamepad?.buttonY.isPressed ?? false
        default:
            return false
        }
    }
    
    public func getControllerAxisValue(_ axis: Int) -> Float {
        guard let controller = GCController.controllers().first else { return 0.0 }
        
        // Map axis integers to controller axes
        switch axis {
        case 0: // Left stick X
            return controller.extendedGamepad?.leftThumbstick.xAxis.value ?? 0.0
        case 1: // Left stick Y
            return controller.extendedGamepad?.leftThumbstick.yAxis.value ?? 0.0
        case 2: // Right stick X
            return controller.extendedGamepad?.rightThumbstick.xAxis.value ?? 0.0
        case 3: // Right stick Y
            return controller.extendedGamepad?.rightThumbstick.yAxis.value ?? 0.0
        default:
            return 0.0
        }
    }
    
    // MARK: - UIGestureRecognizerDelegate
    
    public func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool {
        // Allow gesture recognizers to work simultaneously with touch events
        return true
    }
    
    public func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch) -> Bool {
        // Allow both gesture recognizers and raw touch events to be processed
        return true
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