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
import GameCoreEngine
import GameCorePlatform
import GameCoreGame

// Use CommandType from PlatformDelegates.h via C++ interop

/**
 * @brief Command processor for handling C++ commands on the main thread
 * 
 * This class is designed to be used exclusively on the main thread.
 * All methods are @MainActor isolated for thread safety and simplified implementation.
 * 
 * Processes commands from C++ in a thread-safe manner via the threading proxy system.
 */
@MainActor
class CommandProcessor {
    
    // MARK: - Properties
    
    /// Metal renderer for executing render commands
    private var metalRenderer: MetalRenderer?
    
    /// Audio manager for executing audio commands
    private var audioManager: AVAudioHandler?
    
    // MARK: - Private Logging
    
    private func log(_ message: String, level: LogLevel = .info) {
        switch level {
        case .trace:
            SwiftLog.debug(message, category: "CommandProcessor")
        case .debug:
            SwiftLog.debug(message, category: "CommandProcessor")
        case .info:
            SwiftLog.info(message, category: "CommandProcessor")
        case .warning:
            SwiftLog.warn(message, category: "CommandProcessor")
        case .error:
            SwiftLog.error(message, category: "CommandProcessor")
        case .fatal:
            SwiftLog.fatal(message, category: "CommandProcessor")
        }
    }
    
    /// Initialize the command processor and C++ threading system
    init() {
        // Initialize the C++ threading system using native interop
        GameCorePlatform.GameCore.initializeThreadingSystem()
        log("[CommandProcessor] Initialized - ready to polish those turds on the main thread!")
    }
    
    deinit {
        // Shutdown the C++ threading system
        GameCorePlatform.GameCore.shutdownThreadingSystem()
        // Note: Removed log call here to avoid @MainActor issues in deinit
    }
    
    /// Set the Metal renderer for command execution
    func setMetalRenderer(_ renderer: MetalRenderer) {
        self.metalRenderer = renderer
        log("[CommandProcessor] Metal renderer set - turds ready for rendering!")
    }
    
    /// Set the audio manager for the command processor
    func setAudioManager(_ manager: AVAudioHandler) {
        audioManager = manager
        log("[CommandProcessor] Audio manager set - instance: \(ObjectIdentifier(manager))")
    }
    
    /// Process all pending commands from the queue
    /// @note This method is @MainActor isolated and must be called from the main thread
    func processCommands() {
        // Get commands from the C++ threading system
        let renderCommands = GameCorePlatform.GameCore.getAndClearRenderCommandsFromProxy()
        let audioCommands = GameCorePlatform.GameCore.getAndClearAudioCommandsFromProxy()
        let logCommands = GameCorePlatform.GameCore.getAndClearLogCommandsFromProxy()
        let assetCommands = GameCorePlatform.GameCore.getAndClearAssetCommandsFromProxy()
        
        // Process each command type
        for renderCommand in renderCommands {
            executeRenderCommand(renderCommand)
        }
        
        for audioCommand in audioCommands {
            executeAudioCommand(audioCommand)
        }
        
        for logCommand in logCommands {
            executeLogCommand(logCommand)
        }
        
        for assetCommand in assetCommands {
            executeAssetCommand(assetCommand)
        }
    }
    
    /// Execute a single render command using Metal renderer
    private func executeRenderCommand(_ command: GameCorePlatform.GameCore.RenderCommand) {
        guard let renderer = metalRenderer else {
            log("[CommandProcessor] WARNING: No Metal renderer available for command execution", level: .warning)
            return
        }
        
        // Convert C++ enum to Swift enum
        guard let commandType = GameCorePlatform.GameCore.CommandType(rawValue: UInt32(command.type.rawValue)) else {
            log("[CommandProcessor] WARNING: Unknown command type: \(command.type.rawValue)", level: .warning)
            return
        }
        
        let data = command.data  // Flattened data structure
        
        switch commandType {
        case .CMD_BEGIN_FRAME:
            renderer.beginFrame()
            
        case .CMD_END_FRAME:
            renderer.endFrame()
            
        case .CMD_PRESENT:
            renderer.present()
            
        case .CMD_CLEAR_SCREEN:
            renderer.clearScreen()
            
        case .CMD_DRAW_SPRITE:
            if data.textureHandle != 0 {
                renderer.drawSprite(textureHandle: data.textureHandle,
                                  x: data.x,
                                  y: data.y,
                                  rotation: data.rotation)
            }
            
        case .CMD_DRAW_SPRITE_SCALED:
            if data.textureHandle != 0 {
                renderer.drawSpriteScaled(textureHandle: data.textureHandle,
                                        x: data.x,
                                        y: data.y,
                                        scaleX: data.scaleX,
                                        scaleY: data.scaleY,
                                        rotation: data.rotation)
            }
            
        case .CMD_DRAW_TEXT:
            if let text = data.text {
                renderer.drawText(text: String(cString: text),
                                 x: data.x,
                                 y: data.y,
                                 fontSize: data.fontSize,
                                 r: data.r, g: data.g, b: data.b, a: data.a)
            }
            
        case .CMD_DRAW_RECTANGLE:
            renderer.drawRectangle(x: data.x,
                                 y: data.y,
                                 width: data.width,
                                 height: data.height,
                                 r: data.r, g: data.g, b: data.b, a: data.a)
            
        case .CMD_DRAW_CIRCLE:
            renderer.drawCircle(x: data.x,
                              y: data.y,
                              radius: data.radius,
                              r: data.r, g: data.g, b: data.b, a: data.a)
            
        case .CMD_GET_SCREEN_SIZE:
            let size = renderer.getScreenSize()
            if let widthPtr = data.screenWidth {
                widthPtr.pointee = size.width
            }
            if let heightPtr = data.screenHeight {
                heightPtr.pointee = size.height
            }
            
        default:
            log("[CommandProcessor] Unknown render command type: \(commandType)", level: .warning)
        }
    }
    
    /// Execute a single audio command using AVAudioHandler
    private func executeAudioCommand(_ command: GameCorePlatform.GameCore.AudioCommand) {
        // Convert C++ enum to Swift enum
        guard let commandType = GameCorePlatform.GameCore.CommandType(rawValue: UInt32(command.type.rawValue)) else {
            log("[CommandProcessor] WARNING: Unknown audio command type: \(command.type.rawValue)", level: .warning)
            return
        }
        
        let data = command.data
        
        switch commandType {
        case .CMD_PLAY_MUSIC:
            if let audioFilePtr = data.audioFileName {
                let audioFileName = String(cString: audioFilePtr)
                log("[CommandProcessor] Received playMusic command for: \(audioFileName)")
                
                if let audioManager = audioManager {
                    log("[CommandProcessor] AudioManager found - instance: \(ObjectIdentifier(audioManager)) - calling playMusic")
                    audioManager.playMusic(audioFileName)
                    log("[CommandProcessor] Playing music: \(audioFileName)")
                } else {
                    log("[CommandProcessor] ERROR: AudioManager is nil! Cannot play music: \(audioFileName)", level: .error)
                }
            } else {
                log("[CommandProcessor] Cannot play music: no filename provided", level: .warning)
            }
            
        case .CMD_STOP_MUSIC:
            audioManager?.stopMusic()
            log("[CommandProcessor] Stopping music")
            
        case .CMD_PLAY_SOUND:
            if let audioFilePtr = data.audioFileName {
                let audioFileName = String(cString: audioFilePtr)
                if data.volume > 0 {
                    audioManager?.playSoundWithVolume(audioFileName, volume: data.volume)
                    log("[CommandProcessor] Playing sound: \(audioFileName) with volume \(data.volume)")
                } else {
                    audioManager?.playSound(audioFileName)
                    log("[CommandProcessor] Playing sound: \(audioFileName)")
                }
            } else {
                log("[CommandProcessor] Cannot play sound: no filename provided", level: .warning)
            }
            
        case .CMD_STOP_SOUND:
            audioManager?.stopSound()
            log("[CommandProcessor] Stopping all sounds")
            
        case .CMD_SET_MUSIC_VOLUME:
            audioManager?.setMusicVolume(volume: data.volume)
            log("[CommandProcessor] Setting music volume to: \(data.volume)")
            
        case .CMD_SET_SOUND_VOLUME:
            audioManager?.setSoundVolume(volume: data.volume)
            log("[CommandProcessor] Setting sound volume to: \(data.volume)")
            
        default:
            log("[CommandProcessor] Unknown audio command type: \(command.type)", level: .warning)
        }
    }
    
    /// Execute a single logging command - Forward C++ logs to Swift logging system
    private func executeLogCommand(_ command: GameCorePlatform.GameCore.LogCommand) {
        // Convert C++ enum to Swift enum
        guard let commandType = GameCorePlatform.GameCore.CommandType(rawValue: UInt32(command.type.rawValue)) else {
            log("[CommandProcessor] WARNING: Unknown render command type: \(command.type.rawValue)", level: .warning)
            return
        }
        
        let data = command.data
        
        switch commandType {
        case .CMD_LOG_TRACE:
            let message = String(data.logMessage)
            let category = String(data.logCategory)
            SwiftLog.debug(message, category: category)
            
        case .CMD_LOG_DEBUG:
            let message = String(data.logMessage)
            let category = String(data.logCategory)
            SwiftLog.debug(message, category: category)
            
        case .CMD_LOG_INFO:
            let message = String(data.logMessage)
            let category = String(data.logCategory)
            SwiftLog.info(message, category: category)
            
        case .CMD_LOG_WARN:
            let message = String(data.logMessage)
            let category = String(data.logCategory)
            SwiftLog.warn(message, category: category)
            
        case .CMD_LOG_ERROR:
            let message = String(data.logMessage)
            let category = String(data.logCategory)
            SwiftLog.error(message, category: category)
            
        case .CMD_LOG_FATAL:
            let message = String(data.logMessage)
            let category = String(data.logCategory)
            SwiftLog.fatal(message, category: category)
            
        default:
            log("[CommandProcessor] Unknown log command type: \(command.type)", level: .warning)
        }
    }
    
    /// Execute a single asset command using AssetManager
    private func executeAssetCommand(_ command: GameCorePlatform.GameCore.AssetCommand) {
        // Convert C++ enum to Swift enum
        guard let commandType = GameCorePlatform.GameCore.CommandType(rawValue: UInt32(command.type.rawValue)) else {
            log("[CommandProcessor] WARNING: Unknown asset command type: \(command.type.rawValue)", level: .warning)
            return
        }
        
        let data = command.data
        let pathString = String(data.assetPath)
        
        // Parse asset name and extension from path
        let components = pathString.components(separatedBy: ".")
        let name = components.first ?? pathString
        let fileExtension = components.count > 1 ? components.last! : ""
        
        // Execute the appropriate loading function based on command type
        switch commandType {
        case .CMD_LOAD_TEXTURE:
            let ext = fileExtension.isEmpty ? "png" : fileExtension
            loadTextureWithMetalRenderer(name: name, extension: ext, callback: data.callback, userData: data.userData)
            
        case .CMD_LOAD_AUDIO:
            let ext = fileExtension.isEmpty ? "mp3" : fileExtension
            AssetManager.shared.loadAudioSync(name: name, extension: ext, callback: data.callback, userData: data.userData)
            
        case .CMD_LOAD_FONT:
            let ext = fileExtension.isEmpty ? "ttf" : fileExtension
            AssetManager.shared.loadFontSync(name: name, extension: ext, callback: data.callback, userData: data.userData)
            
        case .CMD_LOAD_DATA:
            let ext = fileExtension.isEmpty ? "json" : fileExtension
            AssetManager.shared.loadDataSync(name: name, extension: ext, callback: data.callback, userData: data.userData)
            
        default:
            log("[CommandProcessor] Unsupported asset command type: \(commandType)", level: .warning)
        }
    }
    
    /// Load texture and register it with MetalRenderer
    private func loadTextureWithMetalRenderer(name: String, extension: String, callback: UnsafeMutableRawPointer?, userData: UnsafeMutableRawPointer?) {
        Task {
            do {
                // Load texture from AssetManager
                let texture = try await AssetManager.shared.loadTexture(name: name, extension: `extension`)
                
                // Register texture with MetalRenderer
                guard let renderer = metalRenderer else {
                    log("[CommandProcessor] ERROR: No Metal renderer available for texture registration", level: .error)
                    AssetManager.invokeCallback(callback, textureData: nil, error: "No Metal renderer available", userData: userData)
                    return
                }
                
                let handle = renderer.registerTexture(texture)
                
                // Create TextureData structure for C++
                let textureData = UnsafeMutableRawPointer.allocate(
                    byteCount: MemoryLayout<GameCore.TextureData>.stride,
                    alignment: MemoryLayout<GameCore.TextureData>.alignment
                )
                
                let textureDataPtr = textureData.bindMemory(to: GameCore.TextureData.self, capacity: 1)
                textureDataPtr.pointee.platformTexture = UnsafeMutableRawPointer(bitPattern: UInt(handle))
                textureDataPtr.pointee.width = Int32(texture.width)
                textureDataPtr.pointee.height = Int32(texture.height)
                textureDataPtr.pointee.format = 0 // Default format
                textureDataPtr.pointee.channels = 4 // RGBA
                textureDataPtr.pointee.dataSize = Int(texture.width * texture.height * 4)
                
                log("[CommandProcessor] Texture registered with MetalRenderer: \(name) -> handle \(handle)", level: .debug)
                AssetManager.invokeCallback(callback, textureData: textureData, error: nil, userData: userData)
            } catch {
                log("[CommandProcessor] ERROR: Failed to load texture \(name): \(error)", level: .error)
                AssetManager.invokeCallback(callback, textureData: nil, error: error.localizedDescription, userData: userData)
            }
        }
    }
}