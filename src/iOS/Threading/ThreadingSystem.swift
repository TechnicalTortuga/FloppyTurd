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

/// Processes render commands from C++ on the main thread synchronously
/// This ensures thread-safe execution of Metal rendering operations
@MainActor
class CommandProcessor {
    private var metalRenderer: MetalRenderer?
    
    /// Initialize the command processor and C++ threading system
    init() {
        // Initialize the C++ threading system using native interop
        FloppyTurd.initializeThreadingSystem()
        print("[CommandProcessor] Initialized - ready to polish those turds on the main thread!")
    }
    
    deinit {
        // Shutdown the C++ threading system
        FloppyTurd.shutdownThreadingSystem()
        print("[CommandProcessor] Deinitialized - turd processing complete!")
    }
    
    /// Set the Metal renderer for command execution
    func setMetalRenderer(_ renderer: MetalRenderer) {
        self.metalRenderer = renderer
        print("[CommandProcessor] Metal renderer set - turds ready for rendering!")
    }
    
    /// Process commands from the C++ queue synchronously
    func processCommands() {
        // Get commands from the C++ threading system using the wrapper function
        let cppCommands = FloppyTurd.getAndClearCommandsFromProxy()
        
        // Process each command
        for cppCommand in cppCommands {
            executeCommand(cppCommand)
        }
    }
    
    /// Execute a single render command using Metal renderer
    private func executeCommand(_ command: FloppyTurd.RenderCommand) {
        guard let renderer = metalRenderer else {
            print("[CommandProcessor] WARNING: No Metal renderer available for command execution")
            return
        }
        
        // Convert C++ enum to Swift enum
        guard let commandType = CommandType(rawValue: Int32(command.type.rawValue)) else {
            print("[CommandProcessor] WARNING: Unknown command type: \(command.type.rawValue)")
            return
        }
        
        let data = command.data  // Flattened data structure
        
        switch commandType {
        case .beginFrame:
            renderer.beginFrame()
            
        case .endFrame:
            renderer.endFrame()
            
        case .present:
            renderer.present()
            
        case .clearScreen:
            renderer.clearScreen(data.r, data.g, data.b, data.a)
            
        case .drawSprite:
            if let spriteHandle = data.sprite {
                let intPtr = Int(bitPattern: spriteHandle)
                let handle = UInt32(bitPattern: Int32(intPtr))
                renderer.drawSprite(handle, data.x, data.y, data.rotation)
            }
            
        case .drawSpriteScaled:
            if let spriteHandle = data.sprite {
                let intPtr = Int(bitPattern: spriteHandle)
                let handle = UInt32(bitPattern: Int32(intPtr))
                renderer.drawSpriteScaled(handle, data.x, data.y, data.scaleX, data.scaleY, data.rotation)
            }
            
        case .drawText:
            if let textPtr = data.text {
                let swiftText = String(cString: textPtr)
                renderer.drawText(text: swiftText, x: data.x, y: data.y, fontSize: data.fontSize, r: data.r, g: data.g, b: data.b, a: data.a)
            }
            
        case .drawRectangle:
            renderer.drawRectangle(x: data.x, y: data.y, width: data.width, height: data.height, r: data.r, g: data.g, b: data.b, a: data.a)
            
        case .drawCircle:
            renderer.drawCircle(data.x, data.y, data.radius, data.r, data.g, data.b, data.a)
            
        case .getScreenSize:
            if let widthPtr = data.screenWidth, let heightPtr = data.screenHeight {
                let size = renderer.getScreenSize()
                widthPtr.pointee = size.width
                heightPtr.pointee = size.height
            }
        }
    }
}