//
//  MetalTextureSwift.swift
//  Professional Game Engine - Native Swift Texture Management
//
//  Created by C++ Swift Interop Migration
//  Direct Swift replacement for MetalTexture.mm with advanced caching
//

import Foundation
import Metal
import MetalKit
import CoreGraphics
import ImageIO

/// Professional Metal texture management in pure Swift
/// Direct replacement for MetalTexture.mm with enhanced caching and SDF support
actor MetalTextureCacheActor {
    private var textureCache: [String: MetalTextureSwift.CachedTexture] = [:]
    private var nextTextureID: UInt32 = 1

    func getCachedTexture(_ filename: String) -> MetalTextureSwift.CachedTexture? {
        return textureCache[filename]
    }

    func cacheTexture(_ filename: String, cachedTexture: MetalTextureSwift.CachedTexture) {
        textureCache[filename] = cachedTexture
    }

    func clearCache() -> Int {
        let count = textureCache.count
        textureCache.removeAll()
        return count
    }

    func getCacheStats() -> (count: Int, totalMemory: Int) {
        let count = textureCache.count
        let totalMemory = textureCache.values.reduce(0) { sum, texture in
            sum + (texture.width * texture.height * MetalTextureSwift.getBytesPerPixelForFormat(texture.format))
        }
        return (count: count, totalMemory: totalMemory)
    }

    func generateTextureID() -> UInt32 {
        defer { nextTextureID += 1 }
        return nextTextureID
    }
}

public class MetalTextureSwift {
    private static let cacheActor = MetalTextureCacheActor()
    nonisolated(unsafe) private static var synchronousTextureID: UInt32 = 10000 // Start higher to avoid conflicts
    
    // MARK: - Texture Cache Management
    
    /// Global texture cache for efficient memory management
    
    // MARK: - Metal Resources
    private var device: MTLDevice?
    private var commandQueue: MTLCommandQueue?
    private var textureLoader: MTKTextureLoader?
    
    // MARK: - Texture Properties
    public private(set) var metalTexture: MTLTexture?
    public private(set) var width: Int = 0
    public private(set) var height: Int = 0
    public private(set) var format: MTLPixelFormat = .bgra8Unorm
    public private(set) var textureID: UInt32 = 0
    public private(set) var isSDF: Bool = false
    public private(set) var filePath: String = ""
    
    // MARK: - Cached Texture Structure
    
    public struct CachedTexture: @unchecked Sendable {
        let metalTexture: MTLTexture
        let width: Int
        let height: Int
        let format: MTLPixelFormat
        let textureID: UInt32
        let isSDF: Bool
        let creationTime: Date
        let accessCount: Int
        let filePath: String
        
        init(metalTexture: MTLTexture, width: Int, height: Int, format: MTLPixelFormat, 
             textureID: UInt32, isSDF: Bool, filePath: String) {
            self.metalTexture = metalTexture
            self.width = width
            self.height = height
            self.format = format
            self.textureID = textureID
            self.isSDF = isSDF
            self.creationTime = Date()
            self.accessCount = 1
            self.filePath = filePath
        }
    }
    
    // MARK: - Texture Format Detection
    
    public enum TextureFormat: Int {
        case uncompressed_grayscale = 1  // For SDF textures
        case uncompressed_gray_alpha = 2
        case uncompressed_r5g6b5 = 3
        case uncompressed_r8g8b8 = 4
        case uncompressed_r5g5b5a1 = 5
        case uncompressed_r4g4b4a4 = 6
        case uncompressed_r8g8b8a8 = 7
        case uncompressed_r32 = 8
        case uncompressed_r32g32b32 = 9
        case uncompressed_r32g32b32a32 = 10
        case compressed_dxt1_rgb = 11
        case compressed_dxt1_rgba = 12
        case compressed_dxt3_rgba = 13
        case compressed_dxt5_rgba = 14
        case compressed_etc1_rgb = 15
        case compressed_etc2_rgb = 16
        case compressed_etc2_eac_rgba = 17
        case compressed_pvrtc_rgb = 18
        case compressed_pvrtc_rgba = 19
        case compressed_astc_4x4_rgba = 20
        case compressed_astc_8x8_rgba = 21
    }
    
    // MARK: - Initialization
    
    public init() {
        print("[MetalTextureSwift] Initializing texture manager")
    }
    
    deinit {
        print("[MetalTextureSwift] Destroying texture: \(filePath)")
        releaseTexture()
    }
    
    /// Initialize with Metal device and command queue
    public func initialize(device: MTLDevice, commandQueue: MTLCommandQueue) -> Bool {
        self.device = device
        self.commandQueue = commandQueue
        self.textureLoader = MTKTextureLoader(device: device)
        
        print("[MetalTextureSwift] ✅ Texture manager initialized")
        return true
    }
    
    // MARK: - Texture Loading API
    
    /// Load texture from file with advanced caching and SDF detection
    public func loadFromFile(_ filename: String) async -> Bool {
        print("[MetalTextureSwift] Loading texture: \(filename)")
        // Check cache first
        if let cachedTexture = await Self.cacheActor.getCachedTexture(filename) {
            print("[MetalTextureSwift] ✅ Using cached texture: \(filename)")
            applyCachedTexture(cachedTexture)
            return true
        }
        // Find file in bundle
        guard let filePath = findTextureFile(filename) else {
            print("[MetalTextureSwift] ERROR: Texture file not found: \(filename)")
            return false
        }
        // Detect if this is an SDF texture based on filename or content
        let isSdfTexture = detectSDFTexture(filename: filename, path: filePath)
        // Load texture based on type
        let success = isSdfTexture ? loadSDFTexture(from: filePath) : loadRegularTexture(from: filePath)
        if success {
            self.filePath = filename
            self.isSDF = isSdfTexture
            self.textureID = await Self.cacheActor.generateTextureID()
            // Cache the loaded texture
            let cachedTexture = CachedTexture(
                metalTexture: metalTexture!,
                width: width,
                height: height,
                format: format,
                textureID: textureID,
                isSDF: isSDF,
                filePath: filename
            )
            await Self.cacheActor.cacheTexture(filename, cachedTexture: cachedTexture)
            print("[MetalTextureSwift] ✅ Loaded texture: \(filename), size: \(width)x\(height), format: \(format), SDF: \(isSDF)")
        } else {
            print("[MetalTextureSwift] ERROR: Failed to load texture: \(filename)")
        }
        return success
    }
    
    /// Create texture from raw data (for procedural generation)
    public func createFromData(_ data: Data, width: Int, height: Int, format: TextureFormat) -> Bool {
        guard let device = device else {
            print("[MetalTextureSwift] ERROR: No Metal device available")
            return false
        }
        
        let metalFormat = convertToMetalPixelFormat(format)
        
        let textureDesc = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: metalFormat,
            width: width,
            height: height,
            mipmapped: false
        )
        textureDesc.usage = [.shaderRead]
        textureDesc.storageMode = .shared
        
        guard let texture = device.makeTexture(descriptor: textureDesc) else {
            print("[MetalTextureSwift] ERROR: Failed to create Metal texture")
            return false
        }
        
        // Upload data
        let region = MTLRegion(origin: MTLOrigin(x: 0, y: 0, z: 0),
                              size: MTLSize(width: width, height: height, depth: 1))
        
        let bytesPerRow = getBytesPerPixel(format) * width
        
        data.withUnsafeBytes { bytes in
            texture.replace(region: region, mipmapLevel: 0, withBytes: bytes.baseAddress!, bytesPerRow: bytesPerRow)
        }
        
        // Set properties
        self.metalTexture = texture
        self.width = width
        self.height = height
        self.format = metalFormat
        // Use synchronous texture ID for non-async contexts
        Self.synchronousTextureID += 1
        self.textureID = Self.synchronousTextureID
        self.isSDF = (format == .uncompressed_grayscale)
        
        texture.label = "Procedural Texture \(textureID)"
        
        print("[MetalTextureSwift] ✅ Created procedural texture: \(width)x\(height), format: \(format)")
        return true
    }
    
    // MARK: - SDF Texture Loading (Preserving Your SDF System)
    
    /// Load SDF (Signed Distance Field) texture with specialized handling
    private func loadSDFTexture(from path: String) -> Bool {
        print("[MetalTextureSwift] Loading SDF texture from: \(path)")
        
        guard let device = device else { return false }
        
        // Load image data
        guard let imageData = loadImageData(from: path) else {
            print("[MetalTextureSwift] ERROR: Failed to load SDF image data")
            return false
        }
        
        // SDF textures are typically single-channel grayscale
        let textureDesc = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .r8Unorm,  // Single channel for SDF
            width: imageData.width,
            height: imageData.height,
            mipmapped: true  // Enable mipmapping for SDF scaling
        )
        textureDesc.usage = [.shaderRead]
        textureDesc.storageMode = .shared
        
        guard let texture = device.makeTexture(descriptor: textureDesc) else {
            print("[MetalTextureSwift] ERROR: Failed to create SDF Metal texture")
            return false
        }
        
        // Upload grayscale data
        let region = MTLRegion(origin: MTLOrigin(x: 0, y: 0, z: 0),
                              size: MTLSize(width: imageData.width, height: imageData.height, depth: 1))
        
        texture.replace(region: region, mipmapLevel: 0, withBytes: imageData.data, bytesPerRow: imageData.width)
        
        // Generate mipmaps for better SDF scaling
        generateMipmaps(for: texture)
        
        // Set properties
        self.metalTexture = texture
        self.width = imageData.width
        self.height = imageData.height
        self.format = .r8Unorm
        
        texture.label = "SDF Texture: \(URL(fileURLWithPath: path).lastPathComponent)"
        
        print("[MetalTextureSwift] ✅ SDF texture loaded: \(width)x\(height), mipmaps: \(texture.mipmapLevelCount)")
        return true
    }
    
    /// Load regular RGBA texture
    private func loadRegularTexture(from path: String) -> Bool {
        print("[MetalTextureSwift] Loading regular texture from: \(path)")
        
        guard let textureLoader = textureLoader else {
            print("[MetalTextureSwift] ERROR: No texture loader available")
            return false
        }
        
        let url = URL(fileURLWithPath: path)
        
        // Configure loading options for optimal performance
        let options: [MTKTextureLoader.Option: Any] = [
            .textureUsage: MTLTextureUsage.shaderRead.rawValue,
            .textureStorageMode: MTLStorageMode.shared.rawValue,
            .generateMipmaps: false  // We'll generate mipmaps manually if needed
        ]
        
        do {
            let texture = try textureLoader.newTexture(URL: url, options: options)
            
            // Set properties
            self.metalTexture = texture
            self.width = texture.width
            self.height = texture.height
            self.format = texture.pixelFormat
            
            texture.label = "Texture: \(url.lastPathComponent)"
            
            return true
        } catch {
            print("[MetalTextureSwift] ERROR: Failed to load texture: \(error)")
            return false
        }
    }
    
    // MARK: - SDF Detection and Processing
    
    /// Detect if a texture should be treated as SDF based on filename patterns
    private func detectSDFTexture(filename: String, path: String) -> Bool {
        let lowercaseFilename = filename.lowercased()
        
        // Check filename patterns for SDF indicators
        let sdfPatterns = [
            "sdf", "distance", "distancefield", "font", "text", 
            "_sdf", "_distance", "_df", "atlas", "fontatlas"
        ]
        
        for pattern in sdfPatterns {
            if lowercaseFilename.contains(pattern) {
                print("[MetalTextureSwift] Detected SDF texture by filename: \(filename)")
                return true
            }
        }
        
        // Check if it's a single-channel image (common for SDF)
        if let imageData = loadImageData(from: path) {
            if imageData.channels == 1 {
                print("[MetalTextureSwift] Detected SDF texture by single channel: \(filename)")
                return true
            }
        }
        
        return false
    }
    
    // MARK: - Cache Management
    
    // Removed old static cache methods. All cache operations now use MetalTextureCacheActor (cacheActor).
    
    /// Apply cached texture to current instance
    private func applyCachedTexture(_ cached: CachedTexture) {
        self.metalTexture = cached.metalTexture
        self.width = cached.width
        self.height = cached.height
        self.format = cached.format
        self.textureID = cached.textureID
        self.isSDF = cached.isSDF
        self.filePath = cached.filePath
    }
    
    /// Clear texture cache to free memory
    public static func clearCache() async {
        let count = await cacheActor.clearCache()
        print("[MetalTextureSwift] ✅ Cleared texture cache: \(count) textures released")
    }
    
    /// Get cache statistics
    public static func getCacheStats() async -> (count: Int, totalMemory: Int) {
        await cacheActor.getCacheStats()
    }
    
    // MARK: - Utility Functions
    
    /// Find texture file in bundle or resources
    private func findTextureFile(_ filename: String) -> String? {
        let bundle = Bundle.main
        let fileExtensions = ["png", "jpg", "jpeg", "bmp", "tga", "dds", "ktx"]
        
        // Try with original filename first
        if let path = bundle.path(forResource: filename, ofType: nil) {
            return path
        }
        
        // Try without extension
        let filenameWithoutExt = URL(fileURLWithPath: filename).deletingPathExtension().lastPathComponent
        
        for ext in fileExtensions {
            if let path = bundle.path(forResource: filenameWithoutExt, ofType: ext) {
                return path
            }
        }
        
        // Try in resources subdirectory
        for ext in fileExtensions {
            if let path = bundle.path(forResource: filenameWithoutExt, ofType: ext, inDirectory: "resources") {
                return path
            }
        }
        
        return nil
    }
    
    /// Load image data for analysis
    private func loadImageData(from path: String) -> (data: UnsafeMutableRawPointer, width: Int, height: Int, channels: Int)? {
        let url = URL(fileURLWithPath: path)
        
        guard let imageSource = CGImageSourceCreateWithURL(url as CFURL, nil),
              let cgImage = CGImageSourceCreateImageAtIndex(imageSource, 0, nil) else {
            return nil
        }
        
        let width = cgImage.width
        let height = cgImage.height
        let channels = cgImage.bitsPerPixel / cgImage.bitsPerComponent
        
        let colorSpace = CGColorSpaceCreateDeviceGray()
        let bytesPerPixel = 1  // For grayscale
        let bytesPerRow = width * bytesPerPixel
        let bufferSize = height * bytesPerRow
        
        guard let data = malloc(bufferSize) else { return nil }
        
        guard let context = CGContext(
            data: data,
            width: width,
            height: height,
            bitsPerComponent: 8,
            bytesPerRow: bytesPerRow,
            space: colorSpace,
            bitmapInfo: CGImageAlphaInfo.none.rawValue
        ) else {
            free(data)
            return nil
        }
        
        context.draw(cgImage, in: CGRect(x: 0, y: 0, width: width, height: height))
        
        return (data: data, width: width, height: height, channels: channels)
    }
    
    /// Generate mipmaps for texture (important for SDF scaling)
    private func generateMipmaps(for texture: MTLTexture) {
        guard let commandQueue = commandQueue else { return }
        
        let commandBuffer = commandQueue.makeCommandBuffer()
        let blitEncoder = commandBuffer?.makeBlitCommandEncoder()
        
        blitEncoder?.generateMipmaps(for: texture)
        blitEncoder?.endEncoding()
        
        commandBuffer?.commit()
        commandBuffer?.waitUntilCompleted()
        
        print("[MetalTextureSwift] Generated \(texture.mipmapLevelCount) mipmap levels")
    }
    
    /// Convert TextureFormat to MTLPixelFormat
    private func convertToMetalPixelFormat(_ format: TextureFormat) -> MTLPixelFormat {
        switch format {
        case .uncompressed_grayscale:
            return .r8Unorm
        case .uncompressed_gray_alpha:
            return .rg8Unorm
        case .uncompressed_r8g8b8a8:
            return .rgba8Unorm
        case .uncompressed_r8g8b8:
            return .rgba8Unorm  // Metal doesn't have RGB8, use RGBA8
        case .uncompressed_r32:
            return .r32Float
        case .uncompressed_r32g32b32a32:
            return .rgba32Float
        default:
            return .rgba8Unorm  // Safe default
        }
    }
    
    /// Get bytes per pixel for format
    private func getBytesPerPixel(_ format: TextureFormat) -> Int {
        switch format {
        case .uncompressed_grayscale:
            return 1
        case .uncompressed_gray_alpha:
            return 2
        case .uncompressed_r8g8b8:
            return 3
        case .uncompressed_r8g8b8a8:
            return 4
        case .uncompressed_r32:
            return 4
        case .uncompressed_r32g32b32:
            return 12
        case .uncompressed_r32g32b32a32:
            return 16
        default:
            return 4  // Safe default
        }
    }
    
    /// Get bytes per pixel for MTLPixelFormat
    public static func getBytesPerPixelForFormat(_ format: MTLPixelFormat) -> Int {
        switch format {
        case .r8Unorm:
            return 1
        case .rg8Unorm:
            return 2
        case .rgba8Unorm, .bgra8Unorm:
            return 4
        case .r32Float:
            return 4
        case .rgba32Float:
            return 16
        default:
            return 4  // Safe default
        }
    }
    
    /// Generate unique texture ID
    private func generateTextureID() async -> UInt32 {
        return await MetalTextureSwift.cacheActor.generateTextureID()
    }
    
    /// Release texture resources
    private func releaseTexture() {
        metalTexture = nil
        width = 0
        height = 0
        textureID = 0
        isSDF = false
        filePath = ""
    }
    
    // MARK: - Public API
    
    /// Get the Metal texture for rendering
    public func getMetalTexture() -> MTLTexture? {
        return metalTexture
    }
    
    /// Check if texture is loaded
    public var isLoaded: Bool {
        return metalTexture != nil
    }
    
    /// Get texture format as integer (for compatibility)
    public func getFormatAsInt() -> Int {
        if isSDF {
            return TextureFormat.uncompressed_grayscale.rawValue
        }
        
        switch format {
        case .r8Unorm:
            return TextureFormat.uncompressed_grayscale.rawValue
        case .rg8Unorm:
            return TextureFormat.uncompressed_gray_alpha.rawValue
        case .rgba8Unorm, .bgra8Unorm:
            return TextureFormat.uncompressed_r8g8b8a8.rawValue
        case .r32Float:
            return TextureFormat.uncompressed_r32.rawValue
        case .rgba32Float:
            return TextureFormat.uncompressed_r32g32b32a32.rawValue
        default:
            return TextureFormat.uncompressed_r8g8b8a8.rawValue
        }
    }
}

// MARK: - Global Texture Management Functions

/// Global texture creation function (equivalent to C++ API)
public func createTextureFromFile(_ filename: String, device: MTLDevice, commandQueue: MTLCommandQueue) async -> MetalTextureSwift? {
    let texture = MetalTextureSwift()
    let initialized = texture.initialize(device: device, commandQueue: commandQueue)
    let loaded = await texture.loadFromFile(filename)
    if initialized {
        if loaded {
            return texture
        }
    }
    return nil
}

/// Global SDF texture creation function
public func createSDFTextureFromFile(_ filename: String, device: MTLDevice, commandQueue: MTLCommandQueue) async -> MetalTextureSwift? {
    let texture = MetalTextureSwift()
    let initialized = texture.initialize(device: device, commandQueue: commandQueue)
    let loaded = await texture.loadFromFile(filename)
    if initialized {
        if loaded {
            // Verify it was loaded as SDF
            if texture.isSDF {
                return texture
            } else {
                print("[MetalTextureSwift] WARNING: Texture '\(filename)' was not detected as SDF")
            }
        }
    }
    return nil
}
