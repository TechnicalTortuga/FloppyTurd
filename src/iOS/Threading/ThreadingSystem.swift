//
//  ThreadingSystem.swift
//  FloppyTurd
//
//  Native C++/Swift interop threading system for processing render commands
//  Part of the Floppy Turd threading architecture for smooth C++/Swift interop
//  Uses Swift 5.9+ C++ interoperability features
//

import Foundation
import QuartzCore
// Import C++ module for native interop
import FloppyTurdEngine

/// Processes render commands from C++ on the main thread using CADisplayLink
/// This ensures thread-safe execution of Metal rendering operations
@MainActor
class CommandProcessor {
    private var displayLink: CADisplayLink?
    private var metalRenderer: MetalRenderer?
    private var isProcessing = false
    
    /// Initialize the command processor and C++ threading system
    init() {
        // Initialize the C++ threading system using native interop
        FloppyTurd.initializeThreadingSystem()
        print("[CommandProcessor] Initializing - ready to polish those turds on the main thread!")
    }
    
    deinit {
        stopProcessing()
        // Shutdown the C++ threading system
        FloppyTurd.shutdownThreadingSystem()
        print("[CommandProcessor] Deinitialized - turd processing complete!")
    }
    
    /// Set the Metal renderer for command execution
    func setMetalRenderer(_ renderer: MetalRenderer) {
        self.metalRenderer = renderer
        print("[CommandProcessor] Metal renderer set - turds ready for rendering!")
    }
    
    /// Start processing commands using CADisplayLink
    func startProcessing() {
        guard !isProcessing else {
            print("[CommandProcessor] Already processing - turds are flying!")
            return
        }
        
        displayLink = CADisplayLink(target: self, selector: #selector(processCommands))
        displayLink?.add(to: .main, forMode: .common)
        isProcessing = true
        
        print("[CommandProcessor] Started processing - turd commands incoming!")
    }
    
    /// Stop processing commands
    func stopProcessing() {
        guard isProcessing else { return }
        
        displayLink?.invalidate()
        displayLink = nil
        isProcessing = false
        
        print("[CommandProcessor] Stopped processing - turd rendering paused!")
    }
    
    /// Process commands from the C++ queue (called by CADisplayLink)
    @objc private func processCommands() {
        // Get commands from C++ threading proxy
        let commands = getCommandsFromCpp()
        
        guard !commands.isEmpty else { return }
        
        // Execute each command
        for command in commands {
            executeCommand(command)
        }
    }
    
    /// Execute a single render command
    private func executeCommand(_ command: RenderCommand) {
        guard let renderer = metalRenderer else {
            print("[CommandProcessor] Warning: No Metal renderer available for command execution")
            return
        }
        
        switch command.type {
        case .beginFrame:
            renderer.beginFrame()
            
        case .endFrame:
            renderer.endFrame()
            
        case .present:
            renderer.present()
            
        case .clearScreen:
            if let clearData = command.data.clearScreen {
                renderer.clearScreen(clearData.r, clearData.g, clearData.b, clearData.a)
            }
            
        case .drawSprite:
            if let spriteData = command.data.sprite {
                renderer.drawSprite(spriteData.sprite, spriteData.x, spriteData.y, spriteData.rotation)
            }
            
        case .drawSpriteScaled:
            if let spriteData = command.data.spriteScaled {
                renderer.drawSpriteScaled(
                    spriteData.sprite,
                    spriteData.x, spriteData.y,
                    spriteData.scaleX, spriteData.scaleY,
                    spriteData.rotation
                )
            }
            
        case .drawText:
            if let textData = command.data.text {
                renderer.drawText(
                    textData.text,
                    textData.x, textData.y,
                    textData.fontSize,
                    textData.r, textData.g, textData.b, textData.a
                )
            }
            
        case .drawRectangle:
            if let rectData = command.data.rectangle {
                renderer.drawRectangle(
                    rectData.x, rectData.y,
                    rectData.width, rectData.height,
                    rectData.r, rectData.g, rectData.b, rectData.a
                )
            }
            
        case .drawCircle:
            if let circleData = command.data.circle {
                renderer.drawCircle(
                    circleData.x, circleData.y,
                    circleData.radius,
                    circleData.r, circleData.g, circleData.b, circleData.a
                )
            }
            
        case .getScreenSize:
            // Screen size queries are typically handled synchronously
            // This case is here for completeness but may not be used in practice
            break
            
        default:
            print("[CommandProcessor] Unknown command type: \(command.type)")
        }
    }
    
    /// Get commands from C++ threading proxy using native interop
    private func getCommandsFromCpp() -> [RenderCommand] {
        // Get the C++ threading proxy instance
        guard let cppProxy = FloppyTurd.getThreadingProxy() else {
            return []
        }
        
        // Get and clear commands from C++ queue
        let cppCommands = cppProxy.getAndClearCommands()
        
        // Convert C++ commands to Swift commands
        var swiftCommands: [RenderCommand] = []
        
        // Process each command from the C++ queue
        while !cppCommands.empty() {
            let cppCommand = cppCommands.front()
            
            // Convert C++ command to Swift command
            if let swiftCommand = convertCppCommandToSwift(cppCommand) {
                swiftCommands.append(swiftCommand)
            }
            
            cppCommands.pop()
        }
        
        return swiftCommands
    }
    
    /// Convert C++ RenderCommand to Swift RenderCommand
    private func convertCppCommandToSwift(_ cppCommand: FloppyTurd.RenderCommand) -> RenderCommand? {
        guard let commandType = CommandType(rawValue: cppCommand.type.rawValue) else {
            print("[CommandProcessor] Unknown C++ command type: \(cppCommand.type.rawValue)")
            return nil
        }
        
        var commandData = CommandData()
        
        switch commandType {
        case .beginFrame, .endFrame, .present:
            // No additional data needed
            break
            
        case .clearScreen:
            commandData.clearScreen = ClearScreenData(
                r: cppCommand.data.clearScreen.r,
                g: cppCommand.data.clearScreen.g,
                b: cppCommand.data.clearScreen.b,
                a: cppCommand.data.clearScreen.a
            )
            
        case .drawSprite:
            commandData.sprite = SpriteData(
                sprite: cppCommand.data.drawSprite.sprite,
                x: cppCommand.data.drawSprite.x,
                y: cppCommand.data.drawSprite.y,
                rotation: cppCommand.data.drawSprite.rotation
            )
            
        case .drawSpriteScaled:
            commandData.spriteScaled = SpriteScaledData(
                sprite: cppCommand.data.drawSpriteScaled.sprite,
                x: cppCommand.data.drawSpriteScaled.x,
                y: cppCommand.data.drawSpriteScaled.y,
                scaleX: cppCommand.data.drawSpriteScaled.scaleX,
                scaleY: cppCommand.data.drawSpriteScaled.scaleY,
                rotation: cppCommand.data.drawSpriteScaled.rotation
            )
            
        case .drawText:
            commandData.text = TextData(
                text: String(cString: cppCommand.data.drawText.text),
                x: cppCommand.data.drawText.x,
                y: cppCommand.data.drawText.y,
                fontSize: cppCommand.data.drawText.fontSize,
                r: cppCommand.data.drawText.r,
                g: cppCommand.data.drawText.g,
                b: cppCommand.data.drawText.b,
                a: cppCommand.data.drawText.a
            )
            
        case .drawRectangle:
            commandData.rectangle = RectangleData(
                x: cppCommand.data.drawRect.x,
                y: cppCommand.data.drawRect.y,
                width: cppCommand.data.drawRect.width,
                height: cppCommand.data.drawRect.height,
                r: cppCommand.data.drawRect.r,
                g: cppCommand.data.drawRect.g,
                b: cppCommand.data.drawRect.b,
                a: cppCommand.data.drawRect.a
            )
            
        case .drawCircle:
            commandData.circle = CircleData(
                x: cppCommand.data.drawCircle.x,
                y: cppCommand.data.drawCircle.y,
                radius: cppCommand.data.drawCircle.radius,
                r: cppCommand.data.drawCircle.r,
                g: cppCommand.data.drawCircle.g,
                b: cppCommand.data.drawCircle.b,
                a: cppCommand.data.drawCircle.a
            )
            
        case .getScreenSize:
            // Screen size queries are typically handled synchronously
            break
        }
        
        return RenderCommand(type: commandType, data: commandData)
    }
}

// MARK: - RenderCommand Swift Representation

/// Swift representation of C++ RenderCommand for processing
struct RenderCommand {
    let type: CommandType
    let data: CommandData
}

/// Command types matching C++ enum
enum CommandType: Int32 {
    case beginFrame = 0
    case endFrame = 1
    case present = 2
    case clearScreen = 3
    case drawSprite = 4
    case drawSpriteScaled = 5
    case drawText = 6
    case drawRectangle = 7
    case drawCircle = 8
    case getScreenSize = 9
}

/// Command data union matching C++ structure
struct CommandData {
    var clearScreen: ClearScreenData?
    var sprite: SpriteData?
    var spriteScaled: SpriteScaledData?
    var text: TextData?
    var rectangle: RectangleData?
    var circle: CircleData?
}

// MARK: - Command Data Structures

struct ClearScreenData {
    let r, g, b, a: Float
}

struct SpriteData {
    let sprite: UnsafeMutableRawPointer?
    let x, y, rotation: Float
}

struct SpriteScaledData {
    let sprite: UnsafeMutableRawPointer?
    let x, y, scaleX, scaleY, rotation: Float
}

struct TextData {
    let text: String
    let x, y, fontSize: Float
    let r, g, b, a: Float
}

struct RectangleData {
    let x, y, width, height: Float
    let r, g, b, a: Float
}

struct CircleData {
    let x, y, radius: Float
    let r, g, b, a: Float
}