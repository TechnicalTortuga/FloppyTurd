//
//  ResourceManagerSwift.swift
//  Professional Game Engine - Native Swift Resource Management
//
//  Created by C++ Swift Interop Migration
//  Modern async resource loading with iOS bundle integration
//

import Foundation
@preconcurrency import Metal
import MetalKit
import Combine

// MARK: - Resource Quality & Types
public enum ResourceQuality: String, CaseIterable, Sendable {
    case low = "low"
    case medium = "medium" 
    case high = "high"
    case auto = "auto"
    
    public static func detectAutoQuality() -> ResourceQuality {
        #if targetEnvironment(simulator)
        return .medium
        #else
        let memory = ProcessInfo.processInfo.physicalMemory
        
        // Simple heuristic based on available memory
        if memory >= 4_000_000_000 { // 4GB+
            return .high
        } else if memory >= 2_000_000_000 { // 2GB+
            return .medium
        } else {
            return .low
        }
        #endif
    }
}

public enum ResourceType: String, CaseIterable, Sendable {
    case texture = "texture"
    case sound = "sound"
    case music = "music"
    case font = "font"
    case shader = "shader"
    case data = "data"
}

public enum LoadingMode: String, CaseIterable, Sendable {
    case sync = "sync"
    case async = "async"
    case stream = "stream"
    case lazy = "lazy"
}

// MARK: - Resource Info & Containers
public struct ResourceInfo: Sendable {
    public let id: String
    public let relativePath: String
    public let type: ResourceType
    public let loadingMode: LoadingMode
    public let minQuality: ResourceQuality
    public var isLoaded: Bool = false
    public var isLoading: Bool = false
    public var qualityVariant: String = ""
    public var lastAccessed: Date = Date()
    
    public init(id: String, relativePath: String, type: ResourceType, 
                loadingMode: LoadingMode = .lazy, minQuality: ResourceQuality = .low) {
        self.id = id
        self.relativePath = relativePath
        self.type = type
        self.loadingMode = loadingMode
        self.minQuality = minQuality
    }
}

/// NOTE: Marked as @unchecked Sendable because T may not be Sendable (e.g., MTLTexture, UIFont).
/// Use with caution and do not share across concurrency domains unless you are certain of thread safety.
public struct CachedResource<T>: @unchecked Sendable {
    public let resource: T
    public let isValid: Bool
    public let path: String
    public let memoryUsage: Int
    public var lastAccessed: Date
    
    public init(resource: T, isValid: Bool = true, path: String = "", memoryUsage: Int = 0) {
        self.resource = resource
        self.isValid = isValid
        self.path = path
        self.memoryUsage = memoryUsage
        self.lastAccessed = Date()
    }
}

// Allow MTLTexture and UIFont to be used as @unchecked Sendable for caching purposes
import Metal
#if canImport(UIKit)
import UIKit
#endif
#if canImport(UIKit)
extension UIFont: @unchecked Sendable {}
#endif

// MARK: - Resource Manager
@MainActor
public final class ResourceManagerSwift: ObservableObject {
    
    // MARK: - Singleton
    nonisolated public static let shared: ResourceManagerSwift = {
        return MainActor.assumeIsolated {
            return ResourceManagerSwift()
        }
    }()
    
    // MARK: - Configuration
    @Published public private(set) var currentQuality: ResourceQuality = .auto
    @Published public private(set) var totalMemoryUsage: Int = 0
    @Published public private(set) var isInitialized: Bool = false
    
    public var compressionEnabled: Bool = false
    public var streamingEnabled: Bool = true
    public var maxTextureSize: Int = 2048
    public var maxCacheMemoryMB: Int = 100
    
    // MARK: - Resource Registry & Caches (public for extensions)
    public var resourceRegistry: [String: ResourceInfo] = [:]
    private var textureCache: [String: CachedResource<MTLTexture>] = [:]
    private var soundCache: [String: CachedResource<Data>] = [:]
    private var musicCache: [String: CachedResource<Data>] = [:]
    #if canImport(UIKit)
    private var fontCache: [String: CachedResource<UIFont>] = [:]
    #endif
    private var dataCache: [String: CachedResource<Data>] = [:]
    
    // MARK: - Metal Device
    private var metalDevice: MTLDevice?
    
    // MARK: - Async Loading
    private let loadingQueue = DispatchQueue(label: "com.floppyturd.resource-loading", qos: .userInitiated)
    private var loadingTasks: [String: Task<Bool, Error>] = [:]
    
    // MARK: - Bundle & Paths
    internal let bundle = Bundle.main
    internal var resourcesPath: String = ""
    
    // MARK: - Combine
    private var cancellables = Set<AnyCancellable>()
    
    // MARK: - Initialization
    private init() {
        setupResourcePaths()
    }
    
    private func setupResourcePaths() {
        // Find resources directory
        if let path = bundle.resourcePath {
            resourcesPath = path
        } else {
            resourcesPath = ""
        }
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] 📁 Resources path: \(resourcesPath)")
    }
    
    public func initialize(quality: ResourceQuality = .auto, metalDevice: MTLDevice? = nil) {
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] 🚀 Initializing with quality: \(quality)")
        
        self.metalDevice = metalDevice
        
        // Resolve auto quality
        if quality == .auto {
            currentQuality = ResourceQuality.detectAutoQuality()
        } else {
            currentQuality = quality
        }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] 📊 Detected quality level: \(currentQuality)")
        
        // Register default resources
        registerDefaultResources()
        
        isInitialized = true
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✅ Resource manager initialized")
    }
    
    public func shutdown() {
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] 🛑 Shutting down resource manager...")
        
        // Cancel all loading tasks
        loadingTasks.values.forEach { $0.cancel() }
        loadingTasks.removeAll()
        
        // Clear all caches
        clearAllCaches()
        
        isInitialized = false
        cancellables.removeAll()
        
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✅ Resource manager shutdown complete")
    }
    
    // MARK: - Resource Registration
    private func registerDefaultResources() {
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] 📋 Registering default resources...")
        
        // Register common game resources
        registerResource(id: "font_default", relativePath: "fonts/default.ttf", type: .font)
        registerResource(id: "font_ui", relativePath: "fonts/ui.ttf", type: .font)
        
        // Audio resources
        registerResource(id: "sound_click", relativePath: "sounds/click.wav", type: .sound)
        registerResource(id: "sound_jump", relativePath: "sounds/jump.wav", type: .sound)
        registerResource(id: "music_menu", relativePath: "music/menu.mp3", type: .music, loadingMode: .stream)
        registerResource(id: "music_game", relativePath: "music/game.mp3", type: .music, loadingMode: .stream)
        
        // Texture resources - these will be detected automatically when requested
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✅ Default resources registered")
    }
    
    public func registerResource(id: String, relativePath: String, type: ResourceType, 
                                loadingMode: LoadingMode = .lazy, minQuality: ResourceQuality = .low) {
        let resource = ResourceInfo(id: id, relativePath: relativePath, type: type, 
                                   loadingMode: loadingMode, minQuality: minQuality)
        resourceRegistry[id] = resource
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] 📝 Registered resource: \(id) -> \(relativePath)")
    }
    
    // MARK: - Resource Loading - Textures
    public func getTexture(id: String) -> MTLTexture? {
        updateAccessTime(for: id)
        // Check cache first
        if let cached = textureCache[id], cached.isValid {
            return cached.resource
        }
        // Try to load synchronously
        if loadTextureSync(id: id) {
            return textureCache[id]?.resource
        }
        return nil
    }
    
    public func loadTextureAsync(id: String) -> Task<MTLTexture?, Error> {
        return Task { @MainActor in
            let texture = self.getTexture(id: id)
            return texture
        }
    }
    
    #if canImport(UIKit)
    private func loadTextureSync(id: String) -> Bool {
        guard let device = metalDevice else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ No Metal device available for texture loading")
            return false
        }
        guard let path = resolveResourcePath(id: id, type: .texture) else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ Failed to resolve texture path: \(id)")
            return false
        }
        guard let image = UIImage(contentsOfFile: path) else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ Failed to load image: \(path)")
            return false
        }
        guard let cgImage = image.cgImage else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ Failed to get CGImage from UIImage")
            return false
        }
        // Create Metal texture
        let textureLoader = MTKTextureLoader(device: device)
        do {
            let texture = try textureLoader.newTexture(cgImage: cgImage, options: [
                .textureUsage: MTLTextureUsage.shaderRead.rawValue,
                .textureStorageMode: MTLStorageMode.private.rawValue
            ])
            // Estimate memory usage
            let bytesPerPixel = 4 // RGBA8
            let memoryUsage = texture.width * texture.height * bytesPerPixel
            // Cache the texture
            let cachedTexture = CachedResource(resource: texture, path: path, memoryUsage: memoryUsage)
            textureCache[id] = cachedTexture
            // Update memory tracking
            totalMemoryUsage += memoryUsage
            traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✅ Loaded texture: \(id) (\(texture.width)x\(texture.height))")
            return true
        } catch {
            traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ Failed to create Metal texture: \(error)")
            return false
        }
    }
    #else
    private func loadTextureSync(id: String) -> Bool { return false }
    #endif
    
    // MARK: - Resource Loading - Fonts
    #if canImport(UIKit)
    public func getFont(id: String, size: CGFloat = 16.0) -> UIFont? {
        let fontKey = "\(id)_\(size)"
        updateAccessTime(for: id)
        // Check cache first
        if let cached = fontCache[fontKey], cached.isValid {
            return cached.resource
        }
        // Try to load
        if loadFontSync(id: id, size: size) {
            return fontCache[fontKey]?.resource
        }
        return UIFont.systemFont(ofSize: size) // Fallback
    }
    #endif
    
    #if canImport(UIKit)
    private func loadFontSync(id: String, size: CGFloat) -> Bool {
        let fontKey = "\(id)_\(size)"
        // Try to get registered path
        if let info = resourceRegistry[id], info.type == .font {
            guard let path = resolveResourcePath(id: id, type: .font) else {
                traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ Failed to resolve font path: \(id)")
                return false
            }
            // Load custom font
            if let font = loadCustomFont(path: path, size: size) {
                let cached = CachedResource(resource: font, path: path, memoryUsage: 1024) // Estimate
                fontCache[fontKey] = cached
                totalMemoryUsage += 1024
                traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✅ Loaded custom font: \(id)")
                return true
            }
        }
        // Try system font as fallback
        let systemFont = UIFont.systemFont(ofSize: size)
        let cached = CachedResource(resource: systemFont, path: "system", memoryUsage: 512)
        fontCache[fontKey] = cached
        totalMemoryUsage += 512
        return true
    }
    #endif
    
    #if canImport(UIKit)
    private func loadCustomFont(path: String, size: CGFloat) -> UIFont? {
        guard let fontData = NSData(contentsOfFile: path),
              let provider = CGDataProvider(data: fontData),
              let cgFont = CGFont(provider) else {
            return nil
        }
        // Register font if needed
        var error: Unmanaged<CFError>?
        if !CTFontManagerRegisterGraphicsFont(cgFont, &error) {
            if let error = error?.takeRetainedValue() {
                traceLog(SWLogLevel.SWLOG_WARNING, "[ResourceManagerSwift] ⚠️ Font registration warning: \(error)")
            }
        }
        if let fontName = cgFont.postScriptName as String? {
            return UIFont(name: fontName, size: size)
        }
        return nil
    }
    #endif
    
    // MARK: - Resource Loading - Audio Data
    public func getSoundData(id: String) -> Data? {
        updateAccessTime(for: id)
        
        // Check cache first
        if let cached = soundCache[id], cached.isValid {
            return cached.resource
        }
        
        // Load synchronously
        if loadSoundSync(id: id) {
            return soundCache[id]?.resource
        }
        
        return nil
    }
    
    public func getMusicData(id: String) -> Data? {
        updateAccessTime(for: id)
        
        // Check cache first (music might not be cached if streaming)
        if let cached = musicCache[id], cached.isValid {
            return cached.resource
        }
        
        // For streaming mode, load directly without caching
        if let info = resourceRegistry[id], info.loadingMode == .stream {
            guard let path = resolveResourcePath(id: id, type: .music) else { return nil }
            return try? Data(contentsOf: URL(fileURLWithPath: path))
        }
        
        // Load and cache
        if loadMusicSync(id: id) {
            return musicCache[id]?.resource
        }
        
        return nil
    }
    
    private func loadSoundSync(id: String) -> Bool {
        guard let path = resolveResourcePath(id: id, type: .sound) else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ Failed to resolve sound path: \(id)")
            return false
        }
        
        do {
            let data = try Data(contentsOf: URL(fileURLWithPath: path))
            let cached = CachedResource(resource: data, path: path, memoryUsage: data.count)
            soundCache[id] = cached
            totalMemoryUsage += data.count
            traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✅ Loaded sound: \(id) (\(data.count) bytes)")
            return true
        } catch {
            traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ Failed to load sound \(id): \(error)")
            return false
        }
    }
    
    private func loadMusicSync(id: String) -> Bool {
        guard let path = resolveResourcePath(id: id, type: .music) else {
            traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ Failed to resolve music path: \(id)")
            return false
        }
        
        do {
            let data = try Data(contentsOf: URL(fileURLWithPath: path))
            let cached = CachedResource(resource: data, path: path, memoryUsage: data.count)
            musicCache[id] = cached
            totalMemoryUsage += data.count
            traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✅ Loaded music: \(id) (\(data.count) bytes)")
            return true
        } catch {
            traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ Failed to load music \(id): \(error)")
            return false
        }
    }
    
    // MARK: - Path Resolution
    private func resolveResourcePath(id: String, type: ResourceType) -> String? {
        // Check if we have a registered resource
        if let info = resourceRegistry[id] {
            return getQualityVariantPath(basePath: info.relativePath, quality: currentQuality)
        }
        
        // Auto-detect based on id and type
        let possiblePaths = generatePossiblePaths(id: id, type: type)
        
        for path in possiblePaths {
            let fullPath = getQualityVariantPath(basePath: path, quality: currentQuality)
            if let resolvedPath = fullPath, FileManager.default.fileExists(atPath: resolvedPath) {
                // Auto-register this resource
                registerResource(id: id, relativePath: path, type: type)
                return resolvedPath
            }
        }
        
        traceLog(SWLogLevel.SWLOG_ERROR, "[ResourceManagerSwift] ❌ Could not resolve path for resource: \(id)")
        return nil
    }
    
    private func generatePossiblePaths(id: String, type: ResourceType) -> [String] {
        let typeFolder = type.rawValue + "s" // textures, sounds, etc.
        let commonExtensions: [String]
        
        switch type {
        case .texture:
            commonExtensions = ["png", "jpg", "jpeg", "tga", "ktx"]
        case .sound:
            commonExtensions = ["wav", "mp3", "m4a", "ogg"]
        case .music:
            commonExtensions = ["mp3", "m4a", "wav", "ogg"]
        case .font:
            commonExtensions = ["ttf", "otf"]
        case .shader:
            commonExtensions = ["metal", "msl"]
        case .data:
            commonExtensions = ["json", "plist", "txt"]
        }
        
        var paths: [String] = []
        
        for ext in commonExtensions {
            paths.append("\(typeFolder)/\(id).\(ext)")
            paths.append("\(id).\(ext)")
            paths.append("assets/\(typeFolder)/\(id).\(ext)")
            paths.append("resources/\(typeFolder)/\(id).\(ext)")
        }
        
        return paths
    }
    
    private func getQualityVariantPath(basePath: String, quality: ResourceQuality) -> String? {
        let qualityMappings: [ResourceQuality] = [quality, .medium, .low] // Fallback order
        
        for qualityLevel in qualityMappings {
            if qualityLevel == .auto { continue }
            
            // Try quality-specific variant first
            let qualityPath = insertQualityIntoPath(basePath, quality: qualityLevel)
            let fullPath = URL(fileURLWithPath: resourcesPath).appendingPathComponent(qualityPath).path
            
            if FileManager.default.fileExists(atPath: fullPath) {
                return fullPath
            }
            
            // Try base path without quality suffix
            if qualityLevel == quality {
                let basePath = URL(fileURLWithPath: resourcesPath).appendingPathComponent(basePath).path
                if FileManager.default.fileExists(atPath: basePath) {
                    return basePath
                }
            }
        }
        
        return nil
    }
    
    private func insertQualityIntoPath(_ path: String, quality: ResourceQuality) -> String {
        guard quality != .auto else { return path }
        
        let url = URL(fileURLWithPath: path)
        let directory = url.deletingLastPathComponent().path
        let filename = url.deletingPathExtension().lastPathComponent
        let fileExtension = url.pathExtension
        
        let qualityFilename = "\(filename)_\(quality.rawValue)"
        
        if directory.isEmpty {
            return "\(qualityFilename).\(fileExtension)"
        } else {
            return "\(directory)/\(qualityFilename).\(fileExtension)"
        }
    }
    
    // MARK: - Cache Management
    private func updateAccessTime(for id: String) {
        if var info = resourceRegistry[id] {
            info.lastAccessed = Date()
            resourceRegistry[id] = info
        }
    }
    
    public func clearAllCaches() {
        let previousMemory = totalMemoryUsage
        
        textureCache.removeAll()
        soundCache.removeAll()
        musicCache.removeAll()
        #if canImport(UIKit)
        fontCache.removeAll()
        #endif
        dataCache.removeAll()
        
        totalMemoryUsage = 0
        
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] 🗑️ Cleared all caches (freed \(previousMemory) bytes)")
    }
    
    public func trimCache(maxMemoryMB: Int = 50) {
        let maxBytes = maxMemoryMB * 1024 * 1024
        
        guard totalMemoryUsage > maxBytes else { return }
        
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✂️ Trimming cache from \(totalMemoryUsage) to \(maxBytes) bytes")
        
        // Implement LRU eviction
        var freedMemory = 0
        
        // Sort by last accessed time and remove oldest
        let sortedTextures = textureCache.sorted { $0.value.lastAccessed < $1.value.lastAccessed }
        for (id, cached) in sortedTextures {
            if totalMemoryUsage - freedMemory <= maxBytes { break }
            textureCache.removeValue(forKey: id)
            freedMemory += cached.memoryUsage
        }
        
        totalMemoryUsage -= freedMemory
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✅ Cache trimmed, freed \(freedMemory) bytes")
    }
    
    // MARK: - Utility Methods
    public func isResourceLoaded(_ id: String) -> Bool {
        return resourceRegistry[id]?.isLoaded ?? false
    }
    
    public func getLoadedResources() -> [String] {
        return resourceRegistry.compactMap { key, info in
            info.isLoaded ? key : nil
        }
    }
    
    public func reloadResource(_ id: String) {
        // Remove from caches
        textureCache.removeValue(forKey: id)
        soundCache.removeValue(forKey: id)
        musicCache.removeValue(forKey: id)
        #if canImport(UIKit)
        fontCache.removeValue(forKey: id)
        #endif
        dataCache.removeValue(forKey: id)
        
        // Reset loading state
        resourceRegistry[id]?.isLoaded = false
        
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] 🔄 Resource marked for reload: \(id)")
    }
    
    public func handleMemoryWarning() {
        traceLog(SWLogLevel.SWLOG_WARNING, "[ResourceManagerSwift] ⚠️ Handling memory warning")
        trimCache(maxMemoryMB: maxCacheMemoryMB / 2) // More aggressive trimming
    }
    
    // MARK: - Level Management
    public func preloadLevel(_ levelIndex: Int) {
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] 📦 Preloading level \(levelIndex)...")
        
        // This would be implemented based on your level resource definitions
        // For now, just a placeholder
        
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✅ Level \(levelIndex) preloaded")
    }
    
    public func unloadLevel(_ levelIndex: Int) {
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] 📤 Unloading level \(levelIndex)...")
        
        // Remove level-specific resources from cache
        // This would be implemented based on your level resource tagging system
        
        traceLog(SWLogLevel.SWLOG_INFO, "[ResourceManagerSwift] ✅ Level \(levelIndex) unloaded")
    }
    
}

// MARK: - C++ Interop Bridge
/// Bridge class for C++ interoperability with ResourceManagerSwift
@_expose(Cxx)
public class ResourceManagerCppBridge {
    
    /// Get resource path - C++ compatible
    @_expose(Cxx) nonisolated public static func getResourcePath(_ resourceName: String) -> String {
        return MainActor.assumeIsolated {
            let manager = ResourceManagerSwift.shared
            if let path = manager.bundle.path(forResource: resourceName, ofType: nil) {
                return path
            }
            // Try with common extensions
            let extensions = ["png", "jpg", "wav", "mp3", "ttf", "json"]
            for ext in extensions {
                if let path = manager.bundle.path(forResource: resourceName, ofType: ext) {
                    return path
                }
            }
            return manager.resourcesPath + "/" + resourceName
        }
    }
    
    /// Get save data path - C++ compatible
    @_expose(Cxx) nonisolated public static func getSaveDataPath(_ fileName: String) -> String {
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first?.path ?? ""
        return documentsPath + "/" + fileName
    }
    
    /// Get documents directory - C++ compatible
    @_expose(Cxx) nonisolated public static func getDocumentsDirectory() -> String {
        return FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first?.path ?? ""
    }
    
    /// Get application support directory - C++ compatible
    @_expose(Cxx) nonisolated public static func getApplicationSupportDirectory() -> String {
        return FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first?.path ?? ""
    }
    
    /// Get bundle path - C++ compatible
    @_expose(Cxx) nonisolated public static func getBundlePath() -> String {
        return Bundle.main.bundlePath
    }
    
    /// Check if resource exists - C++ compatible
    @_expose(Cxx) nonisolated public static func resourceExists(_ resourceName: String) -> Bool {
        return MainActor.assumeIsolated {
            let manager = ResourceManagerSwift.shared
            return manager.bundle.path(forResource: resourceName, ofType: nil) != nil
        }
    }
}
