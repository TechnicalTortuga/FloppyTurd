//
//  ThreadingSystem.swift
//  FloppyTurd
//
//  Native C++/Swift interop threading system for processing render commands
//  Part of the Floppy Turd threading architecture for smooth C++/Swift interop
//  Uses Swift 5.9+ C++ interoperability features
//

import Foundation
import GameCoreEngine
import GameCoreGame
import GameCorePlatform
import QuartzCore
import UIKit

// Use CommandType from PlatformDelegates.h via C++ interop

/// @brief Command processor for handling C++ commands on the main thread
///
/// This class is designed to be used exclusively on the main thread.
/// All methods are @MainActor isolated for thread safety and simplified implementation.
///
/// Processes commands from C++ in a thread-safe manner via the threading proxy system.
@MainActor
class CommandProcessor {

    // MARK: - Properties

    /// Metal renderer for executing render commands
    private var metalRenderer: MetalRenderer?

    /// Audio manager for executing audio commands
    private var audioManager: AVAudioHandler?

    /// Game view controller for handling orientation changes
    private weak var gameViewController: GameViewController?

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

    /// Set the game view controller for orientation command execution
    func setGameViewController(_ controller: GameViewController) {
        gameViewController = controller
        log("[CommandProcessor] Game view controller set - orientation commands enabled")
    }

    /// Process all pending commands from the queue
    /// @note This method is @MainActor isolated and must be called from the main thread
    func processCommands() {
        // Get commands from the C++ threading system
        let renderCommands = GameCorePlatform.GameCore.getAndClearRenderCommandsFromProxy()
        let audioCommands = GameCorePlatform.GameCore.getAndClearAudioCommandsFromProxy()
        let logCommands = GameCorePlatform.GameCore.getAndClearLogCommandsFromProxy()
        let assetCommands = GameCorePlatform.GameCore.getAndClearAssetCommandsFromProxy()
        let hapticCommands = GameCorePlatform.GameCore.getAndClearHapticCommandsFromProxy()
        let saveCommands = GameCorePlatform.GameCore.getAndClearSaveCommandsFromProxy()
        let gameCenterCommands = GameCorePlatform.GameCore.getAndClearGameCenterCommandsFromProxy()
        let adCommands = GameCorePlatform.GameCore.getAndClearAdCommandsFromProxy()

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

        for hapticCommand in hapticCommands {
            executeHapticCommand(hapticCommand)
        }

        for saveCommand in saveCommands {
            executeSaveCommand(saveCommand)
        }

        for gameCenterCommand in gameCenterCommands {
            executeGameCenterCommand(gameCenterCommand)
        }

        for adCommand in adCommands {
            executeAdCommand(adCommand)
        }
    }

    /// Execute a single render command using Metal renderer
    private func executeRenderCommand(_ command: GameCorePlatform.GameCore.RenderCommand) {
        guard let renderer = metalRenderer else {
            log(
                "[CommandProcessor] WARNING: No Metal renderer available for command execution",
                level: .warning)
            return
        }

        // Convert C++ enum to Swift enum
        guard
            let commandType = GameCorePlatform.GameCore.CommandType(
                rawValue: UInt32(command.type.rawValue))
        else {
            log(
                "[CommandProcessor] WARNING: Unknown command type: \(command.type.rawValue)",
                level: .warning)
            return
        }

        var data = command.data  // Flattened data structure

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
                renderer.drawSprite(
                    textureHandle: data.textureHandle,
                    x: data.x,
                    y: data.y,
                    rotation: data.rotation)
            }

        case .CMD_DRAW_SPRITE_SCALED:
            if data.textureHandle != 0 {
                renderer.drawSpriteScaled(
                    textureHandle: data.textureHandle,
                    x: data.x,
                    y: data.y,
                    scaleX: data.scaleX,
                    scaleY: data.scaleY,
                    rotation: data.rotation)
            }

        case .CMD_DRAW_SPRITE_SCALED_CENTERED:
            if data.textureHandle != 0 {
                renderer.drawSpriteScaledCentered(
                    textureHandle: data.textureHandle,
                    x: data.x,
                    y: data.y,
                    scaleX: data.scaleX,
                    scaleY: data.scaleY,
                    rotation: data.rotation)
            }

        case .CMD_DRAW_SPRITE_SCALED_PIVOTED:
            if data.textureHandle != 0 {
                renderer.drawSpriteScaledPivoted(
                    textureHandle: data.textureHandle,
                    x: data.x,
                    y: data.y,
                    scaleX: data.scaleX,
                    scaleY: data.scaleY,
                    rotation: data.rotation,
                    pivotX: data.pivotX,
                    pivotY: data.pivotY)
            }

        case .CMD_DRAW_SPRITE_SCALED_WITH_SOURCE:
            if data.textureHandle != 0 {
                renderer.drawSpriteScaledWithSource(
                    textureHandle: data.textureHandle,
                    x: data.x,
                    y: data.y,
                    scaleX: data.scaleX,
                    scaleY: data.scaleY,
                    rotation: data.rotation,
                    sourceX: data.sourceX,
                    sourceY: data.sourceY,
                    sourceWidth: data.sourceWidth,
                    sourceHeight: data.sourceHeight)
            }

        case .CMD_DRAW_SPRITE_SCALED_WITH_SOURCE_CENTERED:
            if data.textureHandle != 0 {
                renderer.drawSpriteScaledWithSourceCentered(
                    textureHandle: data.textureHandle,
                    x: data.x,
                    y: data.y,
                    scaleX: data.scaleX,
                    scaleY: data.scaleY,
                    rotation: data.rotation,
                    sourceX: data.sourceX,
                    sourceY: data.sourceY,
                    sourceWidth: data.sourceWidth,
                    sourceHeight: data.sourceHeight)
            }

        case .CMD_DRAW_SPRITE_SCALED_WITH_SOURCE_PIVOTED:
            if data.textureHandle != 0 {
                renderer.drawSpriteScaledWithSourcePivoted(
                    textureHandle: data.textureHandle,
                    x: data.x,
                    y: data.y,
                    scaleX: data.scaleX,
                    scaleY: data.scaleY,
                    rotation: data.rotation,
                    pivotX: data.pivotX,
                    pivotY: data.pivotY,
                    sourceX: data.sourceX,
                    sourceY: data.sourceY,
                    sourceWidth: data.sourceWidth,
                    sourceHeight: data.sourceHeight)
            }

        case .CMD_DRAW_SPRITE_BATCH:
            // std::vector<SpriteBatchData> auto-bridges to Swift as RandomAccessCollection
            let batchData = data.batchData
            if !batchData.isEmpty {
                renderer.drawSpriteBatch(batchData)
            }

        case .CMD_DRAW_TEXT:
            let text = String(data.text)
            if !text.isEmpty {
                renderer.drawText(
                    text: text,
                    x: data.x,
                    y: data.y,
                    fontSize: data.fontSize,
                    r: data.r, g: data.g, b: data.b, a: data.a)
            }

        case .CMD_DRAW_TEXT_CENTERED:
            let text = String(data.text)
            if !text.isEmpty {
                renderer.drawTextCentered(
                    text,
                    x: data.x,
                    y: data.y,
                    fontSize: data.fontSize,
                    r: data.r, g: data.g, b: data.b, a: data.a)
            }

        case .CMD_DRAW_TEXT_OUTLINED:
            let text = String(data.text)
            if !text.isEmpty {
                renderer.drawTextOutlined(
                    text,
                    x: data.x,
                    y: data.y,
                    fontSize: data.fontSize,
                    textR: data.r, textG: data.g, textB: data.b, textA: data.a,
                    outlineR: data.outlineR, outlineG: data.outlineG, outlineB: data.outlineB,
                    outlineA: data.outlineA,
                    outlineWidth: data.outlineWidth)
            }

        case .CMD_DRAW_TEXT_CENTERED_OUTLINED:
            let text = String(data.text)
            if !text.isEmpty {
                renderer.drawTextCenteredOutlined(
                    text,
                    x: data.x,
                    y: data.y,
                    fontSize: data.fontSize,
                    textR: data.r, textG: data.g, textB: data.b, textA: data.a,
                    outlineR: data.outlineR, outlineG: data.outlineG, outlineB: data.outlineB,
                    outlineA: data.outlineA,
                    outlineWidth: data.outlineWidth)
            }

        case .CMD_DRAW_RECTANGLE:
            renderer.drawRectangle(
                x: data.x,
                y: data.y,
                width: data.width,
                height: data.height,
                r: data.r, g: data.g, b: data.b, a: data.a)

        case .CMD_DRAW_CIRCLE:
            renderer.drawCircle(
                x: data.x,
                y: data.y,
                radius: data.radius,
                r: data.r, g: data.g, b: data.b, a: data.a)

        case .CMD_DRAW_FILLED_CIRCLE:
            renderer.drawFilledCircle(
                data.x,
                data.y,
                data.radius,
                data.r, data.g, data.b, data.a)

        case .CMD_GET_SCREEN_SIZE:
            let size = renderer.getScreenSize()
            if let widthPtr = data.screenWidth {
                widthPtr.pointee = size.width
            }
            if let heightPtr = data.screenHeight {
                heightPtr.pointee = size.height
            }

        case .CMD_GET_SCREEN_INFO:
            if let screenInfoPtr = data.screenInfo {
                let screenInfo = renderer.getScreenInfo()
                screenInfoPtr.pointee = screenInfo
            }

        case .CMD_GET_TEXTURE_METADATA:
            let textureId = String(data.textureId)
            if let metadata = renderer.getTextureMetadata(textureId: textureId) {
                // Update the owned TextureMetadata in the command data
                data.textureMetadata = metadata
            }

        case .CMD_LOCK_ORIENTATION:
            Task { @MainActor in
                if let gameViewController = gameViewController {
                    gameViewController.lockOrientation()
                    log("[CommandProcessor] Orientation locked successfully")
                } else {
                    log(
                        "[CommandProcessor] ERROR: Cannot lock orientation - GameViewController not available",
                        level: .error)
                }
            }

        case .CMD_UNLOCK_ORIENTATION:
            Task { @MainActor in
                if let gameViewController = gameViewController {
                    gameViewController.unlockOrientation()
                    log("[CommandProcessor] Orientation unlocked successfully")
                } else {
                    log(
                        "[CommandProcessor] ERROR: Cannot unlock orientation - GameViewController not available",
                        level: .error)
                }
            }

        case .CMD_LOCK_TO_PORTRAIT:
            if let gameViewController = gameViewController {
                gameViewController.lockToPortrait()
                log("[CommandProcessor] Orientation locked to portrait successfully")
            } else {
                log(
                    "[CommandProcessor] ERROR: Cannot lock to portrait - GameViewController not available",
                    level: .error)
            }

        case .CMD_LOCK_TO_LANDSCAPE:
            if let gameViewController = gameViewController {
                gameViewController.lockToLandscape()
                log("[CommandProcessor] Orientation locked to landscape successfully")
            } else {
                log(
                    "[CommandProcessor] ERROR: Cannot lock to landscape - GameViewController not available",
                    level: .error)
            }

        default:
            log("[CommandProcessor] Unknown render command type: \(commandType)", level: .warning)
        }
    }

    /// Execute a single audio command using AVAudioHandler
    private func executeAudioCommand(_ command: GameCorePlatform.GameCore.AudioCommand) {
        // Convert C++ enum to Swift enum
        guard
            let commandType = GameCorePlatform.GameCore.CommandType(
                rawValue: UInt32(command.type.rawValue))
        else {
            log(
                "[CommandProcessor] WARNING: Unknown audio command type: \(command.type.rawValue)",
                level: .warning)
            return
        }

        let data = command.data

        switch commandType {
        case .CMD_PLAY_MUSIC:
            let audioFileName = String(data.audioFileName)
            if !audioFileName.isEmpty {
                log("[CommandProcessor] Received playMusic command for: \(audioFileName)")

                if let audioManager = audioManager {
                    log(
                        "[CommandProcessor] AudioManager found - instance: \(ObjectIdentifier(audioManager)) - dispatching to main thread"
                    )
                    Task { @MainActor in
                        audioManager.playMusic(audioFileName)
                        log("[CommandProcessor] Playing music: \(audioFileName)")
                    }
                } else {
                    log(
                        "[CommandProcessor] ERROR: AudioManager is nil! Cannot play music: \(audioFileName)",
                        level: .error)
                }
            } else {
                log("[CommandProcessor] Cannot play music: filename is empty", level: .warning)
            }

        case .CMD_STOP_MUSIC:
            if let audioManager = audioManager {
                Task { @MainActor in
                    audioManager.stopMusic()
                    log("[CommandProcessor] Stopping music")
                }
            }

        case .CMD_PLAY_SOUND:
            let audioFileName = String(data.audioFileName)
            if !audioFileName.isEmpty {
                log("[CommandProcessor] Received playSound command for: \(audioFileName)")

                if let audioManager = audioManager {
                    log(
                        "[CommandProcessor] AudioManager found - instance: \(ObjectIdentifier(audioManager)) - dispatching to main thread"
                    )
                    Task { @MainActor in
                        log("[CommandProcessor] Task started on main actor")
                        if data.volume > 0 {
                            log("[CommandProcessor] Setting sound volume to: \(data.volume)")
                            audioManager.setSoundVolume(volume: data.volume)
                            log("[CommandProcessor] Sound volume set successfully")
                        }
                        log(
                            "[CommandProcessor] About to call audioManager.playSound(\(audioFileName))"
                        )
                        log(
                            "[CommandProcessor] audioManager instance: \(ObjectIdentifier(audioManager))"
                        )
                        audioManager.playSound(audioFileName)
                        log("[CommandProcessor] playSound call completed successfully")
                        log(
                            "[CommandProcessor] Playing sound: \(audioFileName) with volume \(data.volume > 0 ? data.volume : audioManager.soundVolume)"
                        )
                    }
                } else {
                    log(
                        "[CommandProcessor] ERROR: AudioManager is nil! Cannot play sound: \(audioFileName)",
                        level: .error)
                }
            } else {
                log("[CommandProcessor] Cannot play sound: filename is empty", level: .warning)
            }

        case .CMD_STOP_SOUND:
            if let audioManager = audioManager {
                let audioFileName = String(data.audioFileName)
                Task { @MainActor in
                    if !audioFileName.isEmpty {
                        log("[CommandProcessor] Stopping specific sound: \(audioFileName)")
                        audioManager.stopSound(audioFileName)
                    } else {
                        log("[CommandProcessor] Stopping all sounds (no filename specified)")
                        audioManager.stopSound()
                    }
                }
            }

        case .CMD_SET_MUSIC_VOLUME:
            if let audioManager = audioManager {
                Task { @MainActor in
                    audioManager.setMusicVolume(volume: data.volume)
                    log("[CommandProcessor] Setting music volume to: \(data.volume)")
                }
            }

        case .CMD_SET_SOUND_VOLUME:
            if let audioManager = audioManager {
                Task { @MainActor in
                    audioManager.setSoundVolume(volume: data.volume)
                    log("[CommandProcessor] Setting sound volume to: \(data.volume)")
                }
            }

        default:
            log("[CommandProcessor] Unknown audio command type: \(command.type)", level: .warning)
        }
    }

    /// Execute a single logging command - Forward C++ logs to Swift logging system
    private func executeLogCommand(_ command: GameCorePlatform.GameCore.LogCommand) {
        // Convert C++ enum to Swift enum
        guard
            let commandType = GameCorePlatform.GameCore.CommandType(
                rawValue: UInt32(command.type.rawValue))
        else {
            log(
                "[CommandProcessor] WARNING: Unknown render command type: \(command.type.rawValue)",
                level: .warning)
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
        guard
            let commandType = GameCorePlatform.GameCore.CommandType(
                rawValue: UInt32(command.type.rawValue))
        else {
            log(
                "[CommandProcessor] WARNING: Unknown asset command type: \(command.type.rawValue)",
                level: .warning)
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
            loadTextureWithMetalRenderer(
                name: name, extension: ext, callback: data.callback, userData: data.userData)

        case .CMD_LOAD_AUDIO:
            let ext = fileExtension.isEmpty ? "mp3" : fileExtension
            AssetManager.shared.loadAudioSync(
                name: name, extension: ext, callback: data.callback, userData: data.userData)

        case .CMD_LOAD_FONT:
            let ext = fileExtension.isEmpty ? "ttf" : fileExtension
            AssetManager.shared.loadFontSync(
                name: name, extension: ext, callback: data.callback, userData: data.userData)

        case .CMD_LOAD_DATA:
            let ext = fileExtension.isEmpty ? "json" : fileExtension
            AssetManager.shared.loadDataSync(
                name: name, extension: ext, callback: data.callback, userData: data.userData)

        case .CMD_PRELOAD_ESSENTIAL_ASSETS:
            // Preload essential assets (textures, audio, etc.)
            Task {
                await AssetManager.shared.preloadEssentialAssets()
            }
            log("[CommandProcessor] Preloading essential assets requested via delegate.")

        case .CMD_IS_CACHED:
            // Check if asset is cached and invoke callback
            let assetNameStr = String(data.cacheAssetName)
            let assetType = data.cacheAssetType
            var cached = false
            switch assetType {
            case 0:  // texture
                cached = AssetManager.shared.isTextureCached(name: assetNameStr)
            case 1:  // audio
                cached = AssetManager.shared.isAudioCached(name: assetNameStr)
            case 2:  // font
                cached = AssetManager.shared.isFontCached(name: assetNameStr)
            case 3:  // data
                cached = AssetManager.shared.isDataCached(name: assetNameStr)
            default:
                cached = false
            }
            if let callbackPtr = data.callback {
                let callback = unsafeBitCast(
                    callbackPtr,
                    to: (@convention(c) (Bool, UnsafePointer<CChar>?, UnsafeMutableRawPointer?) ->
                        Void).self)
                callback(cached, nil, data.userData)
            }
            log(
                "[CommandProcessor] isCached delegate called for asset: \(assetNameStr) type: \(assetType) result: \(cached)"
            )

        default:
            log(
                "[CommandProcessor] Unsupported asset command type: \(commandType)", level: .warning
            )
        }
    }

    /// Load texture and register it with MetalRenderer
    private func loadTextureWithMetalRenderer(
        name: String, extension: String, callback: UnsafeMutableRawPointer?,
        userData: UnsafeMutableRawPointer?
    ) {
        Task {
            do {
                // Load texture from AssetManager (uses existing cache)
                let texture = try await AssetManager.shared.loadTexture(
                    name: name, extension: `extension`)

                // Register texture with MetalRenderer (handles its own deduplication)
                guard let renderer = metalRenderer else {
                    log(
                        "[CommandProcessor] ERROR: No Metal renderer available for texture registration",
                        level: .error)
                    AssetManager.invokeCallback(
                        callback, textureData: nil, error: "No Metal renderer available",
                        userData: userData)
                    return
                }

                let handle = renderer.registerTexture(texture)

                // Create TextureData structure for C++
                let textureData = UnsafeMutableRawPointer.allocate(
                    byteCount: MemoryLayout<GameCore.TextureData>.stride,
                    alignment: MemoryLayout<GameCore.TextureData>.alignment
                )

                let textureDataPtr = textureData.bindMemory(
                    to: GameCore.TextureData.self, capacity: 1)
                textureDataPtr.pointee.platformTexture = UnsafeMutableRawPointer(
                    bitPattern: UInt(handle))
                textureDataPtr.pointee.width = Int32(texture.width)
                textureDataPtr.pointee.height = Int32(texture.height)
                textureDataPtr.pointee.format = 0  // Default format
                textureDataPtr.pointee.channels = 4  // RGBA
                textureDataPtr.pointee.dataSize = Int(texture.width * texture.height * 4)

                log(
                    "[CommandProcessor] Texture registered with MetalRenderer: \(name) -> handle \(handle)",
                    level: .debug)
                AssetManager.invokeCallback(
                    callback, textureData: textureData, error: nil, userData: userData)
            } catch {
                log(
                    "[CommandProcessor] ERROR: Failed to load texture \(name): \(error)",
                    level: .error)
                AssetManager.invokeCallback(
                    callback, textureData: nil, error: error.localizedDescription,
                    userData: userData)
            }
        }
    }

    /// Execute a single haptic command using HapticManager
    private func executeHapticCommand(_ command: GameCorePlatform.GameCore.HapticCommand) {
        let commandType = command.type

        // Dispatch to main thread for haptic feedback (required by iOS)
        Task { @MainActor in
            switch commandType {
            case .CMD_HAPTIC_IMPACT:
                let style = mapHapticStyle(command.data.style)
                let intensity = CGFloat(command.data.intensity)
                HapticManager.shared.triggerImpact(style: style, intensity: intensity)
                log(
                    "[CommandProcessor] Haptic impact triggered: style=\(style) intensity=\(intensity)",
                    level: .trace)

            case .CMD_HAPTIC_SELECTION:
                HapticManager.shared.triggerSelection()
                log("[CommandProcessor] Haptic selection triggered", level: .trace)

            case .CMD_HAPTIC_NOTIFICATION:
                let notifType = mapHapticNotificationType(command.data.notificationType)
                HapticManager.shared.triggerNotification(type: notifType)
                log(
                    "[CommandProcessor] Haptic notification triggered: type=\(notifType)",
                    level: .trace)

            case .CMD_HAPTIC_PATTERN:
                let patternName = String(command.data.patternName)
                HapticManager.shared.triggerPattern(name: patternName)
                log(
                    "[CommandProcessor] Haptic pattern triggered: \(patternName)", level: .trace)

            case .CMD_HAPTIC_PREPARE:
                let style = mapHapticStyle(command.data.style)
                HapticManager.shared.prepare(style: style)
                log("[CommandProcessor] Haptic prepare: style=\(style)", level: .trace)

            default:
                log(
                    "[CommandProcessor] Unsupported haptic command type: \(commandType)",
                    level: .warning)
            }
        }
    }

    /// Map C++ HapticStyle to UIImpactFeedbackGenerator.FeedbackStyle
    private func mapHapticStyle(_ style: GameCorePlatform.GameCore.HapticStyle)
        -> UIImpactFeedbackGenerator.FeedbackStyle
    {
        switch style {
        case .LIGHT:
            return .light
        case .MEDIUM:
            return .medium
        case .HEAVY:
            return .heavy
        case .RIGID:
            if #available(iOS 13.0, *) {
                return .rigid
            } else {
                return .heavy
            }
        case .SOFT:
            if #available(iOS 13.0, *) {
                return .soft
            } else {
                return .light
            }
        @unknown default:
            return .medium
        }
    }

    /// Map C++ HapticNotificationType to UINotificationFeedbackGenerator.FeedbackType
    private func mapHapticNotificationType(_ type: GameCorePlatform.GameCore.HapticNotificationType)
        -> UINotificationFeedbackGenerator.FeedbackType
    {
        switch type {
        case .SUCCESS:
            return .success
        case .WARNING:
            return .warning
        case .ERROR:
            return .error
        @unknown default:
            return .success
        }
    }

    /// Execute a single save/load command using SaveManager
    private func executeSaveCommand(_ command: GameCorePlatform.GameCore.SaveCommand) {
        let commandType = command.type

        // Dispatch to main thread for save operations
        Task { @MainActor in
            switch commandType {
            case .CMD_SAVE_GAME:
                let jsonString = String(command.data.jsonData)
                if !jsonString.isEmpty {
                    let success = SaveManager.processSaveGameCommand(jsonString)
                    if success {
                        self.log("[CommandProcessor] Game data saved successfully", level: .info)
                    } else {
                        self.log("[CommandProcessor] Failed to save game data", level: .error)
                    }
                }

            case .CMD_LOAD_GAME:
                if SaveManager.processLoadGameCommand() != nil {
                    self.log("[CommandProcessor] Game data loaded successfully", level: .info)
                    // TODO: Pass JSON back to C++ via callback mechanism
                } else {
                    self.log(
                        "[CommandProcessor] No save data found or load failed", level: .warning)
                }

            case .CMD_SAVE_SETTINGS:
                // Settings are automatically persisted via UserDefaults when set on GameSettings
                // Just ensure synchronization
                UserDefaults.standard.synchronize()
                self.log(
                    "[CommandProcessor] Settings saved: master=\(GameSettings.masterVolume) music=\(GameSettings.musicVolume) sfx=\(GameSettings.sfxVolume) debug=\(GameSettings.debugMode) haptics=\(GameSettings.hapticsEnabled)",
                    level: .info)

            case .CMD_LOAD_SETTINGS:
                let (
                    masterVol, musicVol, sfxVol, difficulty, debugMode, hapticsEnabled, wasLoaded
                ) =
                    SaveManager.processLoadSettingsCommand()
                self.log(
                    "[CommandProcessor] Settings loaded: master=\(masterVol) music=\(musicVol) sfx=\(sfxVol) difficulty=\(difficulty) debug=\(debugMode) haptics=\(hapticsEnabled) wasLoaded=\(wasLoaded)",
                    level: .info)

            default:
                self.log(
                    "[CommandProcessor] Unsupported save command type: \(commandType)",
                    level: .warning)
            }
        }
    }

    private func executeGameCenterCommand(_ command: GameCorePlatform.GameCore.GameCenterCommand) {
        let commandType = command.type

        // Process commands directly - GameCenterManager is @MainActor
        switch commandType {
        case .CMD_GAME_CENTER_AUTHENTICATE:
            GameCenterManager.shared.authenticate { [weak self] success, error in
                if success {
                    self?.log(
                        "[CommandProcessor] Game Center authenticated successfully",
                        level: .info)
                } else if let error = error {
                    self?.log(
                        "[CommandProcessor] Game Center authentication failed: \(error.localizedDescription)",
                        level: .error)
                }
            }

        case .CMD_GAME_CENTER_SUBMIT_SCORE:
            let leaderboardID = String(command.data.leaderboardID)
            let score = command.data.score

            GameCenterManager.shared.submitScore(score, leaderboardID: leaderboardID) {
                [weak self] success, error in
                if success {
                    self?.log(
                        "[CommandProcessor] Score \(score) submitted to \(leaderboardID)",
                        level: .info)
                } else if let error = error {
                    self?.log(
                        "[CommandProcessor] Score submission failed: \(error.localizedDescription)",
                        level: .error)
                }
            }

        case .CMD_GAME_CENTER_SHOW_LEADERBOARD:
            let leaderboardID = String(command.data.leaderboardID)
            GameCenterManager.shared.showLeaderboard(leaderboardID)
            self.log("[CommandProcessor] Showing leaderboard: \(leaderboardID)", level: .info)

        case .CMD_GAME_CENTER_SHOW_ALL_LEADERBOARDS:
            GameCenterManager.shared.showAllLeaderboards()
            self.log("[CommandProcessor] Showing all leaderboards", level: .info)

        default:
            self.log(
                "[CommandProcessor] Unsupported Game Center command type: \(commandType)",
                level: .warning)
        }
    }

    // MARK: - Ad Command Execution

    private func executeAdCommand(_ command: GameCorePlatform.GameCore.AdCommand) {
        let commandType = command.type

        // Process commands directly - AdManager is @MainActor
        switch commandType {
        case .CMD_AD_PRELOAD:
            AdManager.shared.preloadAd()
            self.log("[CommandProcessor] Ad preload requested", level: .info)

        case .CMD_AD_SHOW:
            AdManager.shared.showAd()
            self.log("[CommandProcessor] Ad show requested", level: .info)

        case .CMD_AD_IS_READY:
            let isReady = AdManager.shared.isAdReady()
            self.log("[CommandProcessor] Ad ready check: \(isReady)", level: .info)

        case .CMD_AD_SET_ENABLED:
            let enabled = command.data.adsEnabled
            AdManager.shared.setAdsEnabled(enabled)
            self.log("[CommandProcessor] Ads enabled set to: \(enabled)", level: .info)

        default:
            self.log(
                "[CommandProcessor] Unsupported Ad command type: \(commandType)",
                level: .warning)
        }
    }
}
