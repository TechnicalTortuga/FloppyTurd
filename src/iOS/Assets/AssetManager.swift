//
//  AssetManager.swift
//  FloppyTurd
//
//  Modern iOS Asset Management System
//  Designed for Swift 5.9+ native C++ interop
//  Created by Gnosis Engine
//

import Foundation
import UIKit
@preconcurrency import AVFoundation
@preconcurrency import Metal
import MetalKit
import os.log

import GameCoreEngine
import GameCorePlatform
import GameCoreGame

/**
 * Modern iOS Asset Management System
 * 
 * Key Features:
 * - Unified asset loading for iOS
 * - Async/await resource loading
 * - Asset caching and memory management
 * - Platform-specific asset paths
 * - Future Raylib compatibility layer
 * - Thread-safe operations
 */

@MainActor
public class AssetManager {
    
    // MARK: - Singleton Instance
    
    public static let shared = AssetManager()
    
    // MARK: - Asset Types
    
    public enum AssetType: String, CaseIterable {
        case textures = "textures"
        case audio = "audio"
        case fonts = "fonts"
        case shaders = "shaders"
        case data = "data"
    }
    
    // MARK: - Asset Paths
    
    private struct AssetPaths {
        static let basePath = ""
        static let textures = "graphics"
        static let audio = "audio"
        static let music = "audio/music"
        static let sfx = "audio/sfx"
        static let fonts = "fonts"
        static let shaders = "shaders"
        static let data = "data"
    }
    
    // MARK: - Properties
    
    private let logger = Logger(subsystem: "com.floppyturd.game", category: "AssetManager")
    
    // Asset caches
    private var textureCache: [String: MTLTexture] = [:]
    private var audioCache: [String: AVAudioFile] = [:]
    private var fontCache: [String: Any] = [:]
    private var dataCache: [String: Data] = [:]
    
    // Loading queues
    private let assetQueue = DispatchQueue(label: "com.floppyturd.assets", qos: .userInitiated)
    private let textureQueue = DispatchQueue(label: "com.floppyturd.textures", qos: .userInitiated)
    
    // Device capabilities
    private var device: MTLDevice?
    
    // Cache management
    private let maxCacheSize: Int = 100 * 1024 * 1024 // 100MB limit
    private var cacheAccessTimes: [String: Date] = [:]
    
    // MARK: - Initialization
    
    private init() {
        logger.info("AssetManager initialized")
        setupDevice()
        setupMemoryWarningObserver()
        setupBackgroundObserver()
    }
    
    private func setupDevice() {
        device = MTLCreateSystemDefaultDevice()
        if device != nil {
            logger.info("Metal device initialized for texture loading")
        } else {
            logger.error("Failed to initialize Metal device")
        }
    }
    
    private func setupMemoryWarningObserver() {
        NotificationCenter.default.addObserver(
            forName: UIApplication.didReceiveMemoryWarningNotification,
            object: nil,
            queue: .main
        ) { [weak self] _ in
            Task { @MainActor in
                self?.handleMemoryWarning()
            }
        }
    }
    
    private func setupBackgroundObserver() {
        NotificationCenter.default.addObserver(
            forName: UIApplication.didEnterBackgroundNotification,
            object: nil,
            queue: .main
        ) { [weak self] _ in
            Task { @MainActor in
                self?.handleAppBackground()
            }
        }
    }
    
    private func handleMemoryWarning() {
        logger.warning("Memory warning received - clearing non-essential assets")
        
        // Clear 50% of least recently used assets
        evictLeastRecentlyUsedAssets(percentage: 0.5)
    }
    
    private func handleAppBackground() {
        logger.info("App entering background - clearing texture cache")
        
        // Clear texture cache to free GPU memory
        clearCache(for: .textures)
    }
    
    // MARK: - Asset Loading
    
    /// Load texture asynchronously
    public func loadTexture(name: String, extension: String = "png") async throws -> MTLTexture {
        let cacheKey = "\(name).\(`extension`)"
        
        // Check cache first
        if let cached = textureCache[cacheKey] {
            cacheAccessTimes[cacheKey] = Date()
            logger.debug("Texture loaded from cache: \(cacheKey)")
            return cached
        }
        
        // Load from asset catalog using UIImage
        guard let image = UIImage(named: name) else {
            throw AssetError.fileNotFound("\(name) in asset catalog")
        }
        
        let texture = try await loadTextureFromUIImage(image)
        textureCache[cacheKey] = texture
        cacheAccessTimes[cacheKey] = Date()
        logger.info("Texture loaded: \(cacheKey)")
        
        // Check cache size and evict if necessary
        checkCacheSizeAndEvict()
        
        return texture
    }
    
    /// Load audio file asynchronously
    public func loadAudio(name: String, extension: String = "mp3") async throws -> AVAudioFile {
        let cacheKey = "\(name).\(`extension`)"
        
        // Check cache first
        if let cached = audioCache[cacheKey] {
            cacheAccessTimes[cacheKey] = Date()
            logger.debug("Audio loaded from cache: \(cacheKey)")
            return cached
        }
        
        // For asset catalog datasets, we need to use NSDataAsset
        // The audio files are stored as .dataset files in the asset catalog
        guard let dataAsset = NSDataAsset(name: name) else {
            throw AssetError.fileNotFound("\(name) in asset catalog")
        }
        
        // Create a temporary file from the data asset
        let tempURL = FileManager.default.temporaryDirectory.appendingPathComponent("\(name).\(`extension`)")
        
        do {
            try dataAsset.data.write(to: tempURL)
            let audioFile = try AVAudioFile(forReading: tempURL)
            audioCache[cacheKey] = audioFile
            cacheAccessTimes[cacheKey] = Date()
            logger.info("Audio loaded from asset catalog dataset: \(cacheKey)")
            
            // Check cache size and evict if necessary
            checkCacheSizeAndEvict()
            
            return audioFile
        } catch {
            throw AssetError.fileNotFound("Failed to load audio from asset catalog: \(error)")
        }
    }
    
    /// Load font data
    public func loadFont(name: String, extension: String = "ttf") async throws -> Data {
        let cacheKey = "\(name).\(`extension`)"
        
        // Check cache first
        if let cached = dataCache[cacheKey] {
            logger.debug("Font loaded from cache: \(cacheKey)")
            return cached
        }
        
        guard let url = Bundle.main.url(forResource: name, withExtension: `extension`, subdirectory: AssetPaths.fonts) else {
            throw AssetError.fileNotFound("\(name).\(`extension`) in \(AssetPaths.fonts)")
        }
        
        let data = try Data(contentsOf: url)
        dataCache[cacheKey] = data
        logger.info("Font loaded: \(cacheKey)")
        return data
    }
    
    /// Load data file
    public func loadData(name: String, extension: String = "json") async throws -> Data {
        let cacheKey = "\(name).\(`extension`)"
        
        // Check cache first
        if let cached = dataCache[cacheKey] {
            logger.debug("Data loaded from cache: \(cacheKey)")
            return cached
        }
        
        guard let url = Bundle.main.url(forResource: name, withExtension: `extension`, subdirectory: AssetPaths.data) else {
            throw AssetError.fileNotFound("\(name).\(`extension`) in \(AssetPaths.data)")
        }
        
        let data = try Data(contentsOf: url)
        dataCache[cacheKey] = data
        logger.info("Data loaded: \(cacheKey)")
        return data
    }
    
    // MARK: - Asset Preloading
    
    /// Preload essential assets at startup
    public func preloadEssentialAssets() async {
        logger.info("Preloading essential assets...")
        
        let essentialAssets = [
            ("FloppyTurdMenu", "mp3", AssetType.audio),
            ("button_click", "wav", AssetType.audio),
            ("fart1", "mp3", AssetType.audio),
            ("fart2", "mp3", AssetType.audio),
            ("fart3", "mp3", AssetType.audio),
            ("fart4", "mp3", AssetType.audio),
            ("fart5", "mp3", AssetType.audio),
            ("fart6", "mp3", AssetType.audio),
            ("fart7", "mp3", AssetType.audio),
            ("fart8", "mp3", AssetType.audio),
            ("fart9", "mp3", AssetType.audio),
            ("fart10", "mp3", AssetType.audio),
            ("fart11", "mp3", AssetType.audio),
            ("player_sprite", "png", AssetType.textures),
            ("background", "png", AssetType.textures),
            ("Whacky_Joe", "ttf", AssetType.fonts)
        ]
        
        // Load essential assets sequentially to avoid concurrency issues
        for (name, ext, type) in essentialAssets {
            do {
                switch type {
                case .audio:
                    _ = try await self.loadAudio(name: name, extension: ext)
                case .textures:
                    _ = try await self.loadTexture(name: name, extension: ext)
                case .fonts:
                    _ = try await self.loadFont(name: name, extension: ext)
                case .data:
                    _ = try await self.loadData(name: name, extension: ext)
                default:
                    SwiftLog.warn("Unknown asset type for preloading: \(type)", category: "AssetManager")
                }
            } catch {
                SwiftLog.error("Failed to preload essential asset: \(name).\(ext) - \(error)", category: "AssetManager")
            }
        }
    }
    
    // MARK: - Cache Checking
    
    public func isTextureCached(name: String) -> Bool {
        let cacheKey = name.hasSuffix(".png") ? name : "\(name).png"
        return textureCache[cacheKey] != nil
    }

    public func isAudioCached(name: String) -> Bool {
        let cacheKey = name.hasSuffix(".mp3") ? name : "\(name).mp3"
        return audioCache[cacheKey] != nil
    }

    public func isFontCached(name: String) -> Bool {
        let cacheKey = name.hasSuffix(".ttf") ? name : "\(name).ttf"
        return fontCache[cacheKey] != nil || dataCache[cacheKey] != nil
    }

    public func isDataCached(name: String) -> Bool {
        let cacheKey = name.hasSuffix(".json") ? name : "\(name).json"
        return dataCache[cacheKey] != nil
    }
    
    /// Get cached audio file
    public func getCachedAudio(name: String, extension: String) -> AVAudioFile? {
        let cacheKey = "\(name).\(`extension`)"
        return audioCache[cacheKey]
    }
    
    /// Clear cache for specific asset type or all assets
    public func clearCache(for type: AssetManager.AssetType? = nil) {
        if let type = type {
            switch type {
            case .textures:
                textureCache.removeAll()
            case .audio:
                audioCache.removeAll()
            case .fonts:
                fontCache.removeAll()
            case .data:
                dataCache.removeAll()
            default:
                break
            }
            logger.info("Cleared cache for: \(type.rawValue)")
        } else {
            textureCache.removeAll()
            audioCache.removeAll()
            fontCache.removeAll()
            dataCache.removeAll()
            logger.info("Cleared all asset caches")
        }
    }
    
    /// Get asset info
    public func getAssetInfo(name: String, type: AssetManager.AssetType) -> AssetInfo? {
        let path: String
        switch type {
        case .textures:
            path = "\(AssetPaths.textures)/\(name)"
        case .audio:
            path = "\(AssetPaths.audio)/\(name)"
        case .fonts:
            path = "\(AssetPaths.fonts)/\(name)"
        case .shaders:
            path = "\(AssetPaths.shaders)/\(name)"
        case .data:
            path = "\(AssetPaths.data)/\(name)"
        }
        
        guard let url = Bundle.main.url(forResource: name, withExtension: nil, subdirectory: path) else {
            return nil
        }
        
        var fileSize: UInt64 = 0
        do {
            let attributes = try FileManager.default.attributesOfItem(atPath: url.path)
            fileSize = attributes[.size] as? UInt64 ?? 0
        } catch {
            logger.error("Failed to get file size for: \(name)")
        }
        
        return AssetInfo(
            name: name,
            type: type,
            path: url.path,
            size: fileSize,
            lastModified: Date()
        )
    }
    
    // MARK: - Memory Management
    
    /// Get current memory usage
    public func getMemoryUsage() -> AssetMemoryUsage {
        let textureMemory = textureCache.values.reduce(into: 0) { result, texture in
            result += (texture.width * texture.height * 4)
        }
        let audioMemory = audioCache.values.reduce(into: 0) { result, audioFile in
            let frameCount = Int(audioFile.length)
            let bytesPerFrame = Int(audioFile.processingFormat.streamDescription.pointee.mBytesPerFrame)
            result += (frameCount * bytesPerFrame)
        }
        let dataMemory = dataCache.values.reduce(0) { $0 + $1.count }
        
        return AssetMemoryUsage(
            textureMemory: textureMemory,
            audioMemory: audioMemory,
            dataMemory: dataMemory,
            totalMemory: textureMemory + audioMemory + dataMemory
        )
    }
    
    // MARK: - Cache Management
    
    private func checkCacheSizeAndEvict() {
        let currentMemory = getMemoryUsage().totalMemory
        
        if currentMemory > maxCacheSize {
            logger.warning("Cache size exceeded (\(currentMemory) bytes) - evicting assets")
            evictLeastRecentlyUsedAssets(percentage: 0.3)
        }
    }
    
    private func evictLeastRecentlyUsedAssets(percentage: Double) {
        let targetEvictionCount = Int(Double(cacheAccessTimes.count) * percentage)
        
        // Sort by access time (oldest first)
        let sortedByAccess = cacheAccessTimes.sorted { $0.value < $1.value }
        let assetsToEvict = Array(sortedByAccess.prefix(targetEvictionCount))
        
        for (cacheKey, _) in assetsToEvict {
            // Remove from appropriate cache
            if textureCache[cacheKey] != nil {
                textureCache.removeValue(forKey: cacheKey)
                logger.debug("Evicted texture: \(cacheKey)")
            } else if audioCache[cacheKey] != nil {
                audioCache.removeValue(forKey: cacheKey)
                logger.debug("Evicted audio: \(cacheKey)")
            } else if dataCache[cacheKey] != nil {
                dataCache.removeValue(forKey: cacheKey)
                logger.debug("Evicted data: \(cacheKey)")
            }
            
            cacheAccessTimes.removeValue(forKey: cacheKey)
        }
        
        logger.info("Evicted \(assetsToEvict.count) assets from cache")
    }
    
    // MARK: - Private Methods
    
    private func loadTextureFromURL(_ url: URL) async throws -> MTLTexture {
        guard let device = device else {
            throw AssetError.metalNotAvailable
        }
        
        let textureLoader = MTKTextureLoader(device: device)
        
        return try await withCheckedThrowingContinuation { continuation in
            textureLoader.newTexture(URL: url, options: [
                MTKTextureLoader.Option.textureUsage: NSNumber(value: MTLTextureUsage.shaderRead.rawValue),
                MTKTextureLoader.Option.textureStorageMode: NSNumber(value: MTLStorageMode.`private`.rawValue)
            ]) { texture, error in
                if let error = error {
                    continuation.resume(throwing: error)
                } else if let texture = texture {
                    continuation.resume(returning: texture)
                } else {
                    continuation.resume(throwing: AssetError.unknownError)
                }
            }
        }
    }
    
    private func loadTextureFromUIImage(_ image: UIImage) async throws -> MTLTexture {
        guard let device = device else {
            throw AssetError.metalNotAvailable
        }
        
        guard let cgImage = image.cgImage else {
            throw AssetError.unknownError
        }
        
        // Add debug logging for texture loading
        logger.debug("🔥 Loaded texture: size \(image.size.width)x\(image.size.height), cgImage format? \(cgImage.bitsPerComponent) bits/component")
        logger.debug("🔥 UIImage colorSpace: \(image.cgImage?.colorSpace?.name as String? ?? "unknown")")
        logger.debug("🔥 UIImage alphaInfo: \(image.cgImage?.alphaInfo.rawValue ?? 0)")
        logger.debug("🔥 UIImage bitmapInfo: \(image.cgImage?.bitmapInfo.rawValue ?? 0)")
        
        let textureLoader = MTKTextureLoader(device: device)
        
        let texture = try await textureLoader.newTexture(cgImage: cgImage, options: [
            MTKTextureLoader.Option.textureUsage: NSNumber(value: MTLTextureUsage.shaderRead.rawValue),
            MTKTextureLoader.Option.textureStorageMode: NSNumber(value: MTLStorageMode.shared.rawValue),  // TEMPORARY: Use shared for debugging
            MTKTextureLoader.Option.SRGB: NSNumber(value: true),  // IMPORTANT: Convert sRGB to linear for Metal
            MTKTextureLoader.Option.generateMipmaps: NSNumber(value: false),  // Don't generate mipmaps
            MTKTextureLoader.Option.allocateMipmaps: NSNumber(value: false),  // Don't allocate space for mipmaps
            MTKTextureLoader.Option.origin: MTKTextureLoader.Origin.topLeft.rawValue as NSString  // Ensure correct origin
        ])
        
        // Add debug logging for created texture
        logger.debug("🔥 Created MTLTexture: \(texture.width)x\(texture.height), pixelFormat: \(texture.pixelFormat.rawValue)")
        
        return texture
    }
}

// MARK: - Supporting Types

public struct AssetInfo {
    public let name: String
    public let type: AssetManager.AssetType
    public let path: String
    public let size: UInt64
    public let lastModified: Date
}

public struct AssetMemoryUsage {
    public let textureMemory: Int
    public let audioMemory: Int
    public let dataMemory: Int
    public let totalMemory: Int
}

public enum AssetError: LocalizedError {
    case fileNotFound(String)
    case metalNotAvailable
    case unknownError
    
    public var errorDescription: String? {
        switch self {
        case .fileNotFound(let path):
            return "Asset file not found: \(path)"
        case .metalNotAvailable:
            return "Metal device not available for texture loading"
        case .unknownError:
            return "Unknown asset loading error"
        }
    }
}

// MARK: - C++ Bridge Extensions

extension AssetManager {
    
    /// C++ accessible method for asset loading
    public static func loadAssetForCPP(name: String, type: Int32) -> Bool {
        let assetType = AssetType.allCases[Int(type)]
        
        Task {
            do {
                switch assetType {
                case .audio:
                    _ = try await shared.loadAudio(name: name)
                case .textures:
                    _ = try await shared.loadTexture(name: name)
                case .fonts:
                    _ = try await shared.loadFont(name: name)
                case .data:
                    _ = try await shared.loadData(name: name)
                default:
                    break
                }
            } catch {
                Logger(subsystem: "com.floppyturd.game", category: "AssetManager")
                    .error("Failed to load asset for C++: \(name) - \(error)")
            }
        }
        
        return true
    }
    
    /// C++ accessible method for checking if asset is cached
    public static func isAssetCachedForCPP(name: String, type: Int32) -> Bool {
        switch type {
        case 0: // texture
            return shared.isTextureCached(name: name)
        case 1: // audio
            return shared.isAudioCached(name: name)
        case 2: // font
            return shared.isFontCached(name: name)
        case 3: // data
            return shared.isDataCached(name: name)
        default:
            return false
        }
    }
    
    /// C++ accessible method for checking if asset is cached (C string version)
    public static func isAssetCachedForCPP(name: UnsafePointer<CChar>, type: Int32) -> Bool {
        let assetName = String(cString: name)
        return isAssetCachedForCPP(name: assetName, type: type)
    }
    
    /// C++ accessible method for checking if asset is cached (for direct C++ interop)
    public static func isAssetCachedFromSwift(name: UnsafePointer<CChar>, type: Int32) -> Bool {
        let assetName = String(cString: name)
        switch type {
        case 0: // texture
            return shared.isTextureCached(name: assetName)
        case 1: // audio
            return shared.isAudioCached(name: assetName)
        case 2: // font
            return shared.isFontCached(name: assetName)
        case 3: // data
            return shared.isDataCached(name: assetName)
        default:
            return false
        }
    }
    
    /// Get asset path for C++
    public static func getAssetPath(name: String, type: Int32) -> String {
        let assetType = AssetType.allCases[Int(type)]
        
        switch assetType {
        case .textures:
            return "\(AssetPaths.textures)/\(name)"
        case .audio:
            return "\(AssetPaths.audio)/\(name)"
        case .fonts:
            return "\(AssetPaths.fonts)/\(name)"
        case .shaders:
            return "\(AssetPaths.shaders)/\(name)"
        case .data:
            return "\(AssetPaths.data)/\(name)"
        }
    }
    
    /// Load asset and return raw bytes for C++
    public static func loadAssetBytes(name: String, type: Int32, extension: String) async -> Data? {
        let assetType = AssetType.allCases[Int(type)]
        
        do {
            switch assetType {
            case .textures:
                // Load texture and extract raw PNG/JPG bytes
                guard let url = Bundle.main.url(forResource: name, withExtension: `extension`, subdirectory: AssetPaths.textures) else {
                    return nil
                }
                return try Data(contentsOf: url)
                
            case .audio:
                // Load audio file and extract raw bytes
                let paths = [AssetPaths.audio, AssetPaths.music, AssetPaths.sfx]
                for path in paths {
                    if let url = Bundle.main.url(forResource: name, withExtension: `extension`, subdirectory: path) {
                        return try Data(contentsOf: url)
                    }
                }
                return nil
                
            case .fonts, .data:
                // Load data files
                let subdirectory = assetType == .fonts ? AssetPaths.fonts : AssetPaths.data
                guard let url = Bundle.main.url(forResource: name, withExtension: `extension`, subdirectory: subdirectory) else {
                    return nil
                }
                return try Data(contentsOf: url)
                
            case .shaders:
                guard let url = Bundle.main.url(forResource: name, withExtension: `extension`, subdirectory: AssetPaths.shaders) else {
                    return nil
                }
                return try Data(contentsOf: url)
            }
        } catch {
            Logger(subsystem: "com.floppyturd.game", category: "AssetManager")
                .error("Failed to load asset bytes: \(name).\(`extension`) - \(error)")
            return nil
        }
    }
    
    /// Convert raw bytes to native iOS objects and cache them
    public static func cacheAssetFromBytes(name: String, type: Int32, extension: String, bytes: Data) -> Bool {
        let assetType = AssetType.allCases[Int(type)]
        let cacheKey = "\(name).\(`extension`)"
        
        do {
            switch assetType {
            case .textures:
                // Convert bytes to MTLTexture
                guard let device = shared.device else { return false }
                let textureLoader = MTKTextureLoader(device: device)
                
                let texture = try textureLoader.newTexture(data: bytes, options: [
                    MTKTextureLoader.Option.textureUsage: NSNumber(value: MTLTextureUsage.shaderRead.rawValue),
                    MTKTextureLoader.Option.textureStorageMode: NSNumber(value: MTLStorageMode.private.rawValue)
                ])
                
                shared.textureCache[cacheKey] = texture
                shared.cacheAccessTimes[cacheKey] = Date()
                return true
                
            case .audio:
                // Write bytes to temp file and create AVAudioFile
                let tempURL = FileManager.default.temporaryDirectory.appendingPathComponent("\(name).\(`extension`)")
                try bytes.write(to: tempURL)
                
                let audioFile = try AVAudioFile(forReading: tempURL)
                shared.audioCache[cacheKey] = audioFile
                shared.cacheAccessTimes[cacheKey] = Date()
                
                // Clean up temp file
                try? FileManager.default.removeItem(at: tempURL)
                return true
                
            case .fonts, .data:
                // Store raw data
                shared.dataCache[cacheKey] = bytes
                shared.cacheAccessTimes[cacheKey] = Date()
                return true
                
            case .shaders:
                // Store shader source as data
                shared.dataCache[cacheKey] = bytes
                shared.cacheAccessTimes[cacheKey] = Date()
                return true
            }
        } catch {
            Logger(subsystem: "com.floppyturd.game", category: "AssetManager")
                .error("Failed to cache asset from bytes: \(name).\(`extension`) - \(error)")
            return false
        }
    }
    
    // MARK: - Command Processing (called from ThreadingSystem)
    
    // Asset command processing moved to ThreadingSystem.swift for architectural consistency
    
    // MARK: - Synchronous Asset Loading Helpers for Command Processing
    
    /// Load texture synchronously for command processing
    public func loadTextureSync(name: String, extension: String, callback: UnsafeMutableRawPointer?, userData: UnsafeMutableRawPointer?) {
        Task {
            do {
                let texture = try await loadTexture(name: name, extension: `extension`)
                
                // Create TextureData structure for C++
                let textureData = UnsafeMutableRawPointer.allocate(
                    byteCount: MemoryLayout<GameCore.TextureData>.stride,
                    alignment: MemoryLayout<GameCore.TextureData>.alignment
                )
                
                let textureDataPtr = textureData.bindMemory(to: GameCore.TextureData.self, capacity: 1)
                textureDataPtr.pointee.platformTexture = Unmanaged.passRetained(texture).toOpaque()
                textureDataPtr.pointee.width = Int32(texture.width)
                textureDataPtr.pointee.height = Int32(texture.height)
                textureDataPtr.pointee.format = 0 // Default format
                textureDataPtr.pointee.channels = 4 // RGBA
                textureDataPtr.pointee.dataSize = Int(texture.width * texture.height * 4)
                
                AssetManager.invokeCallback(callback, textureData: textureData, error: nil, userData: userData)
            } catch {
                AssetManager.invokeCallback(callback, textureData: nil, error: error.localizedDescription, userData: userData)
            }
        }
    }
    
    /// Load audio synchronously for command processing
    public func loadAudioSync(name: String, extension: String, callback: UnsafeMutableRawPointer?, userData: UnsafeMutableRawPointer?) {
        Task {
            do {
                let _ = try await loadAudio(name: name, extension: `extension`)
                // For audio, we don't return data - this would need AudioData structure
                AssetManager.invokeCallback(callback, textureData: nil, error: nil, userData: userData)
            } catch {
                AssetManager.invokeCallback(callback, textureData: nil, error: error.localizedDescription, userData: userData)
            }
        }
    }
    
    /// Load font synchronously for command processing
    public func loadFontSync(name: String, extension: String, callback: UnsafeMutableRawPointer?, userData: UnsafeMutableRawPointer?) {
        Task {
            do {
                _ = try await loadFont(name: name, extension: `extension`)
                AssetManager.invokeCallback(callback, textureData: nil, error: nil, userData: userData)
            } catch {
                AssetManager.invokeCallback(callback, textureData: nil, error: error.localizedDescription, userData: userData)
            }
        }
    }
    
    /// Load data synchronously for command processing
    public func loadDataSync(name: String, extension: String, callback: UnsafeMutableRawPointer?, userData: UnsafeMutableRawPointer?) {
        Task {
            do {
                _ = try await loadData(name: name, extension: `extension`)
                AssetManager.invokeCallback(callback, textureData: nil, error: nil, userData: userData)
            } catch {
                AssetManager.invokeCallback(callback, textureData: nil, error: error.localizedDescription, userData: userData)
            }
        }
    }
    



    public static func invokeCallback(_ callback: UnsafeMutableRawPointer?, textureData: UnsafeMutableRawPointer?, error: String?, userData: UnsafeMutableRawPointer?) {
        guard let callback = callback else { return }
        
        // Cast to the expected C++ callback function signature
        let callbackFunc = unsafeBitCast(callback, to: (@convention(c) (UnsafeMutableRawPointer?, UnsafePointer<CChar>?, UnsafeMutableRawPointer?) -> Void).self)
        
        if let error = error {
            error.withCString { errorPtr in
                callbackFunc(nil, errorPtr, userData)
            }
        } else {
            callbackFunc(textureData, nil, userData)
        }
    }

}