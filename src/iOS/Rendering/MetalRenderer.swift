//
//  MetalRenderer.swift
//  FloppyTurd
//
//  Created by Gnosis Engine
//  Copyright © 2024 Floppy Turd Studios. All rights reserved.
//

import CoreGraphics
import CoreText
import Foundation
import GameCorePlatform
import Metal
import MetalKit
import UIKit
import simd

// MARK: - RotSprite Data Structures

/// RotSprite processing parameters - matches the Metal shader structure
struct RotSpriteParams {
    var rotationAngle: Float
    var originalSize: SIMD2<UInt32>
    var upscaledSize: SIMD2<UInt32>
    var rotationCenter: SIMD2<Float>  // For output bb
    var inputCenter: SIMD2<Float>  // For input upscaled
    var colorThreshold: Float

    init(
        rotationDegrees: Float, originalWidth: UInt32, originalHeight: UInt32,
        inputCenterX: Float = 0.5, inputCenterY: Float = 0.5,
        outputCenterX: Float = 0.5, outputCenterY: Float = 0.5,
        threshold: Float = 0.1
    ) {
        self.rotationAngle = rotationDegrees * Float.pi / 180.0
        self.originalSize = SIMD2(originalWidth, originalHeight)
        self.upscaledSize = SIMD2(originalWidth * 8, originalHeight * 8)
        self.inputCenter = SIMD2(inputCenterX, inputCenterY)
        self.rotationCenter = SIMD2(outputCenterX, outputCenterY)
        self.colorThreshold = threshold
    }
}

// MARK: - SDF Font Data Structures

struct GlyphInfo {
    let atlasX: Float  // X position in atlas (0-1)
    let atlasY: Float  // Y position in atlas (0-1)
    let atlasWidth: Float  // Width in atlas (0-1)
    let atlasHeight: Float  // Height in atlas (0-1)
    let bearingX: Float  // Left bearing
    let bearingY: Float  // Top bearing
    let advance: Float  // Horizontal advance
    let width: Float  // Glyph width in pixels
    let height: Float  // Glyph height in pixels
}

struct FontMetrics {
    let size: Float  // Font size
    let lineHeight: Float  // Line spacing
    let ascender: Float  // Distance from baseline to top
    let descender: Float  // Distance from baseline to bottom
    let base: Float  // Baseline position from .fnt file
    let atlasWidth: Float  // Atlas texture width in pixels
    let atlasHeight: Float  // Atlas texture height in pixels
}

/**
 * @file MetalRenderer.swift
 * @brief Metal-based 2D renderer for iOS using threading system for command processing
 *
 * This implementation uses the FloppyTurd threading system to process render commands
 * safely on the main thread, eliminating concurrency issues and weak self captures.
 *
 * The renderer is designed to be called exclusively from the CommandProcessor on the
 * main thread, ensuring thread safety without complex dispatch patterns.
 *
 * Features:
 * - Hardware-accelerated 2D rendering via Metal
 * - Efficient texture management and caching
 * - Optimized batch rendering for sprites and primitives
 * - Integration with threading system for safe command processing
 * - Clean, direct method calls without concurrency complexity
 */

/// @class MetalRenderer
/// @brief Swift implementation for iOS Metal rendering via threading system
///
/// This class is designed to be used exclusively by the CommandProcessor on the main thread.
/// All methods are @MainActor isolated for thread safety and simplified implementation.
///
/// Provides high-performance Metal-based 2D rendering with clean, direct method calls.
@MainActor
public class MetalRenderer {

    // MARK: - Properties (private for encapsulation, @MainActor isolated for thread safety)
    private var device: MTLDevice?
    private var commandQueue: MTLCommandQueue?
    private var renderPipelineState: MTLRenderPipelineState?
    private var texturedPipelineState: MTLRenderPipelineState?

    // MARK: - RotSprite Compute Pipeline Infrastructure
    private var rotspriteUpscaleComputePipeline: MTLComputePipelineState?
    private var rotspriteRotateComputePipeline: MTLComputePipelineState?
    private var rotspriteRestoreComputePipeline: MTLComputePipelineState?

    // RotSprite texture buffers (texture pool for intermediate processing)
    private var rotspriteTexturePool: [MTLTexture] = []
    private var rotspriteMaxTextureSize: Int = 1024  // Maximum texture size for pooling

    // RotSprite processed texture cache (cache results for frequently rotated sprites)
    private var rotspriteTextureCache: [String: MTLTexture] = [:]
    private var rotspriteCacheMaxSize: Int = 50  // Maximum cached textures

    // Text is rasterized via CoreText; no GPU text pipelines needed

    private var vertexBuffer: MTLBuffer?
    private var indexBuffer: MTLBuffer?
    private var uniformBuffer: MTLBuffer?
    private var samplerState: MTLSamplerState?
    private var parallaxSamplerState: MTLSamplerState?  // Specialized sampler for parallax backgrounds
    // No SDF/MSDF samplers
    private var library: MTLLibrary?

    private func log(_ message: String, level: LogLevel = .info) {
        // Assuming SwiftLog is synchronous (no await needed; fix for warnings)
        // If SwiftLog is async, add @MainActor or Task { await ... }
        Task {
            switch level {
            case .trace:
                SwiftLog.debug(message, category: "MetalRenderer")
            case .debug:
                SwiftLog.debug(message, category: "MetalRenderer")
            case .info:
                SwiftLog.info(message, category: "MetalRenderer")
            case .warning:
                SwiftLog.warn(message, category: "MetalRenderer")
            case .error:
                SwiftLog.error(message, category: "MetalRenderer")
            case .fatal:
                SwiftLog.fatal(message, category: "MetalRenderer")
            }
        }
    }

    // Rendering state (private, @MainActor isolated)
    private var currentRenderTarget: MTLTexture?
    private var currentCommandBuffer: MTLCommandBuffer?
    private var currentRenderPassDescriptor: MTLRenderPassDescriptor?
    private var currentRenderEncoder: MTLRenderCommandEncoder?  // FIX: Track the single render encoder
    
    // MARK: - Performance Profiling
    private struct TimingStats {
        var totalMs: Double = 0.0
        var maxMs: Double = 0.0
        var callCount: Int = 0
    }
    
    private var profilingEnabled: Bool = true  // Enable by default for debugging
    private var frameCount: Int = 0
    private var timingStats: [String: TimingStats] = [:]
    private var lastFlushTime: CFAbsoluteTime = CFAbsoluteTimeGetCurrent()
    private var sectionStartTime: CFAbsoluteTime = 0
    private var currentDrawable: CAMetalDrawable?
    private var viewportSize: CGSize = CGSize.zero
    private var clearColor: MTLClearColor = MTLClearColor(
        red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)  // Black background

    // MTKView connection (weak, private)
    private weak var metalView: MTKView?

    // Resource management (private)
    private var textures: [UInt32: MTLTexture] = [:]
    private var nextTextureHandle: UInt32 = 1

    // Handle deduplication system
    private var textureToHandle: [ObjectIdentifier: UInt32] = [:]
    private var handleReferenceCount: [UInt32: Int] = [:]

    // Statistics tracking
    private var handleReuseCount: Int = 0
    private var newHandleCount: Int = 0

    // No SDF/MSDF atlases; we rasterize to RGBA textures per string
    private var fontMetrics: FontMetrics?
    private var glyphMap: [Character: GlyphInfo] = [:]
    private var fontAtlasCache: [String: UIImage] = [:]
    // Keep the CoreText font so we can measure with exact typographic bounds
    private var ctFont: CTFont?
    // Device scale for px<->pt conversion (we render in device pixels)
    private var deviceScale: CGFloat = UIScreen.main.scale
    // Cache for rasterized text textures: key -> (texture,sizePx, ascentPx, padPx)
    // Cache rasterized text textures AND their registered renderer handle to avoid per-frame register/unregister churn
    private var textTextureCache:
        [String: (
            texture: MTLTexture, sizePx: CGSize, ascentPx: CGFloat, padPx: CGFloat, handle: UInt32
        )] = [:]

    // MARK: - Initialization (@MainActor ensures main thread execution)

    public init() {
        log("🔧 MetalRenderer init() called - setting up Metal on main thread", level: .info)
        setupMetal()
        log(
            "🔧 MetalRenderer init() completed - device: \(device != nil), commandQueue: \(commandQueue != nil)",
            level: .info)
    }

    deinit {
        // Note: Cannot call @MainActor shutdown() from deinit
        // Resources will be cleaned up when the actor is deallocated
        // or shutdown() should be called explicitly before deallocation
    }

    // MARK: - Metal Setup

    private func setupMetal() {
        log("🔧 setupMetal() starting - initializing Metal device and command queue", level: .info)

        // Get the default Metal device
        guard let device = MTLCreateSystemDefaultDevice() else {
            log("Metal is not supported on this device", level: .error)
            return
        }

        self.device = device
        log("Metal device created successfully: \(device.name)", level: .debug)

        self.commandQueue = device.makeCommandQueue()
        log("Metal command queue created: \(commandQueue != nil)", level: .debug)

        setupRenderPipeline()
        setupBuffers()

        // Load the default font for text rendering
        log("About to load default font 'Whacky_Joe' with fontSize 32.0", level: .info)
        let fontLoaded = loadFont(fontName: "Whacky_Joe", fontSize: 32.0)
        if fontLoaded {
            log("Default font 'Whacky_Joe' loaded successfully", level: .info)
        } else {
            log(
                "Failed to load default font 'Whacky_Joe' - text rendering will use placeholders",
                level: .warning)
        }

        log("🔧 setupMetal() completed - ready for rendering", level: .info)
    }

    private func setupRenderPipeline() {
        guard let device = device else {
            log("Cannot setup render pipeline: device is nil", level: .error)
            return
        }

        // Create the default library
        guard let library = device.makeDefaultLibrary() else {
            log("Failed to create default Metal library", level: .error)
            return
        }

        self.library = library

        // Create vertex and fragment functions
        guard let vertexFunction = library.makeFunction(name: "vertex_main"),
            let fragmentFunction = library.makeFunction(name: "fragment_main")
        else {
            log("Failed to create vertex/fragment functions", level: .error)
            return
        }

        // Create vertex descriptor
        let vertexDescriptor = MTLVertexDescriptor()

        // Position attribute (float2)
        vertexDescriptor.attributes[0].format = .float2
        vertexDescriptor.attributes[0].offset = 0
        vertexDescriptor.attributes[0].bufferIndex = 0

        // Texture coordinate attribute (float2)
        vertexDescriptor.attributes[1].format = .float2
        vertexDescriptor.attributes[1].offset = 8
        vertexDescriptor.attributes[1].bufferIndex = 0

        // Color attribute (float4)
        vertexDescriptor.attributes[2].format = .float4
        vertexDescriptor.attributes[2].offset = 16
        vertexDescriptor.attributes[2].bufferIndex = 0

        // Buffer layout
        vertexDescriptor.layouts[0].stride = 32  // 2 + 2 + 4 floats * 4 bytes
        vertexDescriptor.layouts[0].stepRate = 1
        vertexDescriptor.layouts[0].stepFunction = .perVertex

        // Create render pipeline descriptor
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.vertexFunction = vertexFunction
        pipelineDescriptor.fragmentFunction = fragmentFunction
        pipelineDescriptor.vertexDescriptor = vertexDescriptor
        pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm_srgb

        // Enable blending for transparency support
        pipelineDescriptor.colorAttachments[0].isBlendingEnabled = true
        pipelineDescriptor.colorAttachments[0].rgbBlendOperation = .add
        pipelineDescriptor.colorAttachments[0].alphaBlendOperation = .add
        pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = .sourceAlpha
        pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
        // pipelineDescriptor.colorAttachments[0].rgbBlendOperation = .add
        // pipelineDescriptor.colorAttachments[0].alphaBlendOperation = .add
        // pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        // pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = .sourceAlpha
        // pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        // pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha

        do {
            renderPipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
            log("Render pipeline state created successfully", level: .debug)
        } catch {
            log("Failed to create render pipeline state: \(error)", level: .error)
        }

        // Create textured pipeline
        guard let texturedFragmentFunction = library.makeFunction(name: "textured_fragment_main")
        else {
            log("Failed to create textured fragment function", level: .error)
            return
        }

        let texturedPipelineDescriptor = MTLRenderPipelineDescriptor()
        texturedPipelineDescriptor.vertexFunction = vertexFunction
        texturedPipelineDescriptor.fragmentFunction = texturedFragmentFunction
        texturedPipelineDescriptor.vertexDescriptor = vertexDescriptor
        texturedPipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm_srgb  // Match MTKView pixel format

        // Enable blending for textured rendering (required for sprites with transparency)
        texturedPipelineDescriptor.colorAttachments[0].isBlendingEnabled = true
        texturedPipelineDescriptor.colorAttachments[0].rgbBlendOperation = .add
        texturedPipelineDescriptor.colorAttachments[0].alphaBlendOperation = .add
        texturedPipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        texturedPipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = .sourceAlpha
        texturedPipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor =
            .oneMinusSourceAlpha
        texturedPipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor =
            .oneMinusSourceAlpha

        // Also create a non-blending pipeline for testing

        do {
            texturedPipelineState = try device.makeRenderPipelineState(
                descriptor: texturedPipelineDescriptor)
            log("Textured pipeline state created successfully", level: .debug)
        } catch {
            log("Failed to create textured pipeline state: \(error)", level: .error)
        }

        // No SDF/MSDF pipelines needed for raster text

        // MARK: - RotSprite Compute Pipeline Setup
        setupRotSpriteComputePipelines()

        // Create sampler state for texture sampling (sprites/pixel art)
        let samplerDescriptor = MTLSamplerDescriptor()
        samplerDescriptor.minFilter = .nearest
        samplerDescriptor.magFilter = .nearest
        samplerDescriptor.mipFilter = .notMipmapped
        samplerDescriptor.sAddressMode = .clampToEdge
        samplerDescriptor.tAddressMode = .clampToEdge
        samplerState = device.makeSamplerState(descriptor: samplerDescriptor)

        // Create specialized parallax sampler for sub-pixel perfect scrolling
        let parallaxSamplerDescriptor = MTLSamplerDescriptor()
        parallaxSamplerDescriptor.minFilter = .nearest
        parallaxSamplerDescriptor.magFilter = .nearest
        parallaxSamplerDescriptor.mipFilter = .notMipmapped
        parallaxSamplerDescriptor.sAddressMode = .repeat  // Allow seamless wrapping for parallax
        parallaxSamplerDescriptor.tAddressMode = .clampToEdge
        parallaxSamplerDescriptor.normalizedCoordinates = true  // Use normalized coordinates for precision
        parallaxSamplerState = device.makeSamplerState(descriptor: parallaxSamplerDescriptor)

        log("Sampler states created (nearest for sprites, specialized parallax)", level: .debug)
    }

    // MARK: - RotSprite Compute Pipeline Setup

    /// Set up the RotSprite compute pipelines for high-quality pixel art rotation
    private func setupRotSpriteComputePipelines() {
        guard let device = device, let library = library else {
            log("Cannot setup RotSprite pipelines: device or library is nil", level: .error)
            return
        }

        log("🎨 Setting up RotSprite compute pipelines for pixel-perfect rotation", level: .info)

        // Create compute pipeline for Scale2x upscaling
        do {
            guard let upscaleFunction = library.makeFunction(name: "rotsprite_scale2x_upscale")
            else {
                log("Failed to create RotSprite upscale function", level: .error)
                return
            }
            rotspriteUpscaleComputePipeline = try device.makeComputePipelineState(
                function: upscaleFunction)
            log("RotSprite upscale compute pipeline created successfully", level: .debug)
        } catch {
            log("Failed to create RotSprite upscale compute pipeline: \(error)", level: .error)
        }

        // Create compute pipeline for rotation and downscaling
        do {
            guard let rotateFunction = library.makeFunction(name: "rotsprite_rotate_and_downscale")
            else {
                log("Failed to create RotSprite rotate function", level: .error)
                return
            }
            rotspriteRotateComputePipeline = try device.makeComputePipelineState(
                function: rotateFunction)
            log("RotSprite rotate compute pipeline created successfully", level: .debug)
        } catch {
            log("Failed to create RotSprite rotate compute pipeline: \(error)", level: .error)
        }

        // Create compute pipeline for detail restoration (optional, for Phase 5)
        do {
            guard let restoreFunction = library.makeFunction(name: "rotsprite_restore_details")
            else {
                log("Failed to create RotSprite restore function", level: .error)
                return
            }
            rotspriteRestoreComputePipeline = try device.makeComputePipelineState(
                function: restoreFunction)
            log("RotSprite restore compute pipeline created successfully", level: .debug)
        } catch {
            log("Failed to create RotSprite restore compute pipeline: \(error)", level: .error)
        }

        log("✅ RotSprite compute pipelines setup complete", level: .info)
    }

    private func setupSDFTextPipelines(
        device: MTLDevice, library: MTLLibrary, vertexFunction: MTLFunction,
        vertexDescriptor: MTLVertexDescriptor
    ) {}

    private func setupMSDFTextPipelines(
        device: MTLDevice, library: MTLLibrary, vertexFunction: MTLFunction,
        vertexDescriptor: MTLVertexDescriptor
    ) {}

    private func setupBuffers() {
        guard let device = device else {
            log("Cannot setup buffers: device is nil", level: .error)
            return
        }

        // Create vertex buffer for a quad with position, texCoord, and color
        // Format: [x, y, u, v, r, g, b, a] per vertex
        let vertices: [Float] = [
            // Bottom-left
            0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0,
            // Bottom-right
            1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
            // Top-right
            1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0,
            // Top-left
            0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0,
        ]

        vertexBuffer = device.makeBuffer(
            bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: [])
        vertexBuffer?.label = "Quad Vertices"

        // Create index buffer
        let indices: [UInt16] = [0, 1, 2, 2, 3, 0]
        indexBuffer = device.makeBuffer(
            bytes: indices, length: indices.count * MemoryLayout<UInt16>.stride, options: [])
        indexBuffer?.label = "Quad Indices"

        log("Vertex and index buffers created successfully", level: .debug)
    }

    // MARK: - Public Interface (Threading System Integration)

    public func initialize() -> Bool {
        let isInitialized = device != nil && commandQueue != nil
        log(
            "initialize() check - device: \(device != nil), commandQueue: \(commandQueue != nil), result: \(isInitialized)",
            level: .debug)
        return isInitialized
    }

    public func shutdown() {
        textures.removeAll()
        vertexBuffer = nil
        indexBuffer = nil
        uniformBuffer = nil
        samplerState = nil
        parallaxSamplerState = nil
        renderPipelineState = nil
        texturedPipelineState = nil

        // Clean up RotSprite resources
        rotspriteUpscaleComputePipeline = nil
        rotspriteRotateComputePipeline = nil
        rotspriteRestoreComputePipeline = nil
        rotspriteTexturePool.removeAll()
        rotspriteTextureCache.removeAll()

        library = nil
        commandQueue = nil
        device = nil
        metalView = nil
        log("MetalRenderer shutdown complete", level: .info)
    }

    // MARK: - MTKView Connection

    public func setMetalView(_ view: MTKView) {
        metalView = view
        view.device = device
        view.colorPixelFormat = .bgra8Unorm_srgb  // Use sRGB and match pipeline formats
        view.clearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)  // Black background
        view.framebufferOnly = false
        view.enableSetNeedsDisplay = false
        view.isPaused = false
        view.preferredFramesPerSecond = 60

        viewportSize = view.drawableSize

        // Create uniform buffer for MVP matrix
        if let device = device {
            uniformBuffer = device.makeBuffer(
                length: MemoryLayout<simd_float4x4>.stride, options: [])
            uniformBuffer?.label = "Uniforms"

            // Set up orthographic projection matrix
            updateProjectionMatrix()
        }

        log(
            "MetalRenderer connected to MTKView with size \(viewportSize) (width: \(viewportSize.width), height: \(viewportSize.height))",
            level: .debug)
    }

    private func updateProjectionMatrix() {
        guard let uniformBuffer = uniformBuffer else { return }

        let width = Float(viewportSize.width)
        let height = Float(viewportSize.height)

        // Use helper function to create projection matrix
        let projectionMatrix = MetalMatrixHelpers.viewportProjectionMatrix(
            width: width, height: height)

        let contents = uniformBuffer.contents().bindMemory(to: simd_float4x4.self, capacity: 1)
        contents.pointee = projectionMatrix

        // NOTE: didModifyRange not needed on iOS - cache coherency is automatic

        print("🔧 Updated projection matrix for viewport \(width)x\(height)")
        log("Updated projection matrix for viewport \(width)x\(height)", level: .debug)
    }
    
    // MARK: - Profiling Methods
    
    private func startTiming(_ section: String) {
        guard profilingEnabled else { return }
        sectionStartTime = CFAbsoluteTimeGetCurrent()
    }
    
    private func endTiming(_ section: String) {
        guard profilingEnabled else { return }
        let duration = (CFAbsoluteTimeGetCurrent() - sectionStartTime) * 1000.0  // Convert to ms
        
        var stats = timingStats[section] ?? TimingStats()
        stats.totalMs += duration
        stats.maxMs = max(stats.maxMs, duration)
        stats.callCount += 1
        timingStats[section] = stats
    }
    
    private func flushTimings() {
        guard profilingEnabled else { return }
        
        let now = CFAbsoluteTimeGetCurrent()
        if now - lastFlushTime >= 1.0 {  // Flush every ~1 second
            var output = "[MetalProfiler] frames=\(frameCount)"
            
            for (name, stats) in timingStats.sorted(by: { $0.key < $1.key }) {
                let avgMs = stats.callCount > 0 ? stats.totalMs / Double(stats.callCount) : 0.0
                output += " | \(name) avg=\(String(format: "%.3f", avgMs))ms"
                output += " max=\(String(format: "%.3f", stats.maxMs))ms"
                output += " calls=\(stats.callCount)"
            }
            
            log(output, level: .info)
            
            // Reset stats
            timingStats.removeAll()
            frameCount = 0
            lastFlushTime = now
        }
    }

    public func beginFrame() {
        frameCount += 1
        startTiming("BeginFrame")
        
        guard let commandQueue = commandQueue else { 
            endTiming("BeginFrame")
            return 
        }

        // Guard against cases where the view is not ready to be drawn to. This can happen
        // during app startup, backgrounding, or other view lifecycle events.
        guard metalView?.currentDrawable != nil, metalView?.currentRenderPassDescriptor != nil
        else {
            // Don't create a command buffer if we can't render. The system will
            // simply skip this frame.
            log("Metal view is not ready for drawing. Skipping frame.", level: .warning)
            endTiming("BeginFrame")
            return
        }

        currentCommandBuffer = commandQueue.makeCommandBuffer()
        currentCommandBuffer?.label = "FloppyTurd Frame Commands"

        if let view = metalView {
            currentDrawable = view.currentDrawable
            currentRenderPassDescriptor = view.currentRenderPassDescriptor
        } else {
            log("beginFrame: No MTKView set - skipping drawable/descriptor", level: .warning)
        }

        // FIX: Do NOT create render encoder here - wait until first draw call
        currentRenderEncoder = nil
        endTiming("BeginFrame")
    }

    public func endFrame() {
        startTiming("EndFrame")
        // FIX: End the render encoder if it exists
        currentRenderEncoder?.endEncoding()
        currentRenderEncoder = nil
        log("endFrame() called - render encoder ended", level: .debug)
        endTiming("EndFrame")
    }

    public func present() {
        startTiming("Present")
        // Proper commit/present order: present THEN commit
        if let drawable = currentDrawable {
            currentCommandBuffer?.present(drawable)
        }
        currentCommandBuffer?.commit()
        currentCommandBuffer = nil
        currentDrawable = nil
        currentRenderPassDescriptor = nil
        log("Frame presented successfully", level: .debug)
        endTiming("Present")
        
        // Flush profiling stats periodically
        flushTimings()
    }

    public func setViewport(x: Float, y: Float, width: Float, height: Float) {
        viewportSize = CGSize(width: CGFloat(width), height: CGFloat(height))
        log("Viewport set to (\(x), \(y), \(width), \(height))", level: .debug)
    }

    public func updateViewportSize(width: Float, height: Float) {
        viewportSize = CGSize(width: CGFloat(width), height: CGFloat(height))
        updateProjectionMatrix()
        log("Viewport size updated to (\(width), \(height))", level: .debug)
    }

    public func setClearColor(r: Float, g: Float, b: Float, a: Float) {
        clearColor = MTLClearColor(
            red: Double(r), green: Double(g), blue: Double(b), alpha: Double(a))
        log("Clear color set to (\(r), \(g), \(b), \(a))", level: .debug)
    }

    public func clearScreen() {
        guard let renderPassDescriptor = currentRenderPassDescriptor else {
            log("clearScreen: No render pass descriptor available", level: .warning)
            return
        }

        // Set clear color and load action - this will clear when the render encoder is created
        renderPassDescriptor.colorAttachments[0].clearColor = clearColor
        renderPassDescriptor.colorAttachments[0].loadAction = .clear

        log(
            "Screen clear configured with color (\(clearColor.red), \(clearColor.green), \(clearColor.blue), \(clearColor.alpha))",
            level: .debug)
    }

    public func clear() {
        clearScreen()
    }

    // FIX: Helper method to ensure render encoder is created only once per frame
    private func ensureRenderEncoder() -> MTLRenderCommandEncoder? {
        if currentRenderEncoder == nil {
            guard let commandBuffer = currentCommandBuffer,
                let renderPassDescriptor = currentRenderPassDescriptor
            else {
                log(
                    "ensureRenderEncoder: Missing command buffer or render pass descriptor",
                    level: .warning)
                return nil
            }

            currentRenderEncoder = commandBuffer.makeRenderCommandEncoder(
                descriptor: renderPassDescriptor)
            currentRenderEncoder?.label = "FloppyTurd Render Pass"

            // NEW: fully define raster-state each frame so we match Apple docs
            if let enc = currentRenderEncoder {
                // 1) viewport covers the whole drawable
                let vp = MTLViewport(
                    originX: 0,
                    originY: 0,
                    width: Double(viewportSize.width),
                    height: Double(viewportSize.height),
                    znear: 0,
                    zfar: 1)
                enc.setViewport(vp)
                // 2) solid fill
                enc.setTriangleFillMode(.fill)
                // 3) no culling while we diagnose
                enc.setCullMode(.none)
            }

            log("🎬 Created new render encoder for frame - viewport: \(viewportSize)", level: .debug)
        }
        return currentRenderEncoder
    }

    public func drawRectangle(
        x: Float, y: Float, width: Float, height: Float,
        r: Float, g: Float, b: Float, a: Float
    ) {
        guard let device = device,
            let renderPipelineState = renderPipelineState,
            let indexBuffer = indexBuffer,
            let uniformBuffer = uniformBuffer
        else {
            log("drawRectangle: Missing required Metal resources", level: .warning)
            return
        }

        // FIX: Use the single render encoder for the entire frame
        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawRectangle: Failed to get render encoder", level: .error)
            return
        }

        // Create dynamic vertex data for this specific rectangle
        // Format: [x, y, u, v, r, g, b, a] per vertex
        let vertices: [Float] = [
            // Bottom-left
            x, y + height, 0.0, 1.0, r, g, b, a,
            // Bottom-right
            x + width, y + height, 1.0, 1.0, r, g, b, a,
            // Top-right
            x + width, y, 1.0, 0.0, r, g, b, a,
            // Top-left
            x, y, 0.0, 0.0, r, g, b, a,
        ]

        // Create temporary vertex buffer for this rectangle
        guard
            let tempVertexBuffer = device.makeBuffer(
                bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: [])
        else {
            log("drawRectangle: Failed to create vertex buffer", level: .error)
            return
        }

        // FIX: Use the shared render encoder, don't create a new one or end it
        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)

        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer,
            indexBufferOffset: 0)

    }

    public func drawTexture(textureHandle: UInt32, x: Float, y: Float, width: Float, height: Float)
    {
        guard let texture = textures[textureHandle] else {
            log("drawTexture: Invalid texture handle \(textureHandle)", level: .warning)
            return
        }

        guard let texturedPipelineState = texturedPipelineState,
            let uniformBuffer = uniformBuffer
        else {
            log("drawTexture: Missing required Metal resources", level: .warning)
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawTexture: Failed to get render encoder", level: .error)
            return
        }

        // Use helper function to create texture transformation matrix
        let modelMatrix = MetalMatrixHelpers.textureTransformMatrix(
            position: (x: x, y: y),
            size: (width: width, height: height)
        )

        // Get current projection matrix
        let projectionMatrix = uniformBuffer.contents().bindMemory(
            to: simd_float4x4.self, capacity: 1
        ).pointee

        // Create MVP matrix
        let mvpMatrix = projectionMatrix * modelMatrix

        // Create temporary uniform buffer for this texture
        guard let device = device,
            let tempUniformBuffer = device.makeBuffer(
                bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: [])
        else {
            log("drawTexture: Failed to create temporary uniform buffer", level: .error)
            return
        }

        // Set up render encoder
        renderEncoder.setRenderPipelineState(texturedPipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(tempUniformBuffer, offset: 0, index: 1)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.setFragmentSamplerState(samplerState, index: 0)

        // Draw the texture
        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!,
            indexBufferOffset: 0)

        log(
            "drawTexture: Rendered texture \(textureHandle) at (\(x), \(y)) with size (\(width), \(height))",
            level: .debug)
    }

    public func drawText(
        text: String, x: Float, y: Float, fontSize: Float,
        r: Float, g: Float, b: Float, a: Float
    ) {
        // Support multi-line with top-left anchor: render each line below the previous
        // Split on literal backslash-n sequences and real newlines
        let expanded = text.replacingOccurrences(of: "\\n", with: "\n")
        let lines = expanded.split(separator: "\n", omittingEmptySubsequences: false)
        var currentY = y
        let lineHeight = fontSize * 1.1
        for line in lines {
            drawTextRaster(
                String(line), x: x, y: currentY, fontSize: fontSize, fill: SIMD4<Float>(r, g, b, a),
                outline: nil)
            currentY += lineHeight
        }
    }

    public func drawTextOutlined(
        _ text: String, x: Float, y: Float, fontSize: Float,
        textR: Float, textG: Float, textB: Float, textA: Float,
        outlineR: Float, outlineG: Float, outlineB: Float, outlineA: Float,
        outlineWidth: Float
    ) {
        drawTextRaster(
            text,
            x: x,
            y: y,
            fontSize: fontSize,
            fill: SIMD4<Float>(textR, textG, textB, textA),
            outline: (
                color: SIMD4<Float>(outlineR, outlineG, outlineB, outlineA), widthPx: outlineWidth
            )
        )
    }

    public func loadTexture(imagePath: String) -> UInt32 {
        guard let device = device else {
            log("loadTexture: No Metal device available", level: .error)
            return 0
        }

        // Create a simple test texture for now
        let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .rgba8Unorm,
            width: 64,
            height: 64,
            mipmapped: false
        )

        guard let texture = device.makeTexture(descriptor: textureDescriptor) else {
            log("loadTexture: Failed to create texture for \(imagePath)", level: .error)
            return 0
        }

        let handle = nextTextureHandle
        textures[handle] = texture
        nextTextureHandle += 1

        log("Texture loaded: \(imagePath) -> handle \(handle)", level: .debug)
        return handle
    }

    public func unloadTexture(handle: UInt32) {
        if textures.removeValue(forKey: handle) != nil {
            log("Texture unloaded: handle \(handle)", level: .debug)
        } else {
            log("unloadTexture: Invalid handle \(handle)", level: .warning)
        }
    }

    public func getTextureSize(textureHandle: UInt32) -> (Int, Int) {
        guard let texture = textures[textureHandle] else {
            log("getTextureSize: Invalid texture handle \(textureHandle)", level: .warning)
            return (0, 0)
        }

        return (texture.width, texture.height)
    }

    /// Register a texture from AssetManager and return a handle for MetalRenderer
    public func registerTexture(_ texture: MTLTexture) -> UInt32 {
        let textureId = ObjectIdentifier(texture)

        // Return existing handle if texture already registered
        if let existingHandle = textureToHandle[textureId] {
            handleReferenceCount[existingHandle, default: 0] += 1
            handleReuseCount += 1
            log(
                "Texture reused: handle \(existingHandle) (refs: \(handleReferenceCount[existingHandle]!))",
                level: .debug)
            return existingHandle
        }

        // Create new handle only for genuinely new textures
        let handle = nextTextureHandle
        textures[handle] = texture
        textureToHandle[textureId] = handle
        handleReferenceCount[handle] = 1
        newHandleCount += 1
        nextTextureHandle += 1

        log(
            "Texture registered: handle \(handle) (\(texture.width)x\(texture.height))",
            level: .debug)
        log(
            "Texture registration: pixel format \(texture.pixelFormat), usage \(texture.usage)",
            level: .debug)
        return handle
    }

    public func unregisterTexture(handle: UInt32) {
        guard let texture = textures[handle] else { return }

        let textureId = ObjectIdentifier(texture)
        let refCount = handleReferenceCount[handle, default: 0] - 1

        if refCount <= 0 {
            // Last reference - actually unregister
            textures.removeValue(forKey: handle)
            textureToHandle.removeValue(forKey: textureId)
            handleReferenceCount.removeValue(forKey: handle)

            log("Texture unregistered: handle \(handle)", level: .debug)
        } else {
            // Still has references
            handleReferenceCount[handle] = refCount
            log("Texture reference released: handle \(handle) (refs: \(refCount))", level: .debug)
        }
    }

    public func isHandleValid(_ handle: UInt32) -> Bool {
        return textures[handle] != nil
    }

    public func getHandleStatistics() -> (reused: Int, new: Int, active: Int) {
        return (reused: handleReuseCount, new: newHandleCount, active: textures.count)
    }

    /// Clear cached text textures and release their handles
    public func clearTextTextureCache() {
        for (key, entry) in textTextureCache {
            if entry.handle != 0 { unregisterTexture(handle: entry.handle) }
            textTextureCache.removeValue(forKey: key)
        }
        log("Text texture cache cleared", level: .info)
    }

    // MARK: - Additional Drawing Methods for Threading System

    public func clearScreen(_ r: Float, _ g: Float, _ b: Float, _ a: Float) {
        setClearColor(r: r, g: g, b: b, a: a)
        clearScreen()
    }

    public func drawSprite(textureHandle: UInt32, x: Float, y: Float, rotation: Float) {
        // For now, treat sprites as textures with full size
        if let texture = textures[textureHandle] {
            let width = Float(texture.width)
            let height = Float(texture.height)
            drawTexture(textureHandle: textureHandle, x: x, y: y, width: width, height: height)
        } else {
            log("drawSprite: Invalid sprite handle \(textureHandle)", level: .warning)
        }
    }

    public func drawSpriteScaled(
        textureHandle: UInt32, x: Float, y: Float, scaleX: Float, scaleY: Float, rotation: Float
    ) {
        startTiming("DrawSpriteScaled")
        defer { endTiming("DrawSpriteScaled") }
        
        guard let texture = textures[textureHandle] else {
            log("drawSpriteScaled: Invalid sprite handle \(textureHandle)", level: .warning)
            return
        }

        guard let uniformBuffer = uniformBuffer else {
            log("drawSpriteScaled: Missing required Metal resources", level: .warning)
            return
        }

        // Check if textured pipeline state is available
        guard let pipelineState = texturedPipelineState else {
            log("drawSpriteScaled: No pipeline state available", level: .warning)
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawSpriteScaled: Failed to get render encoder", level: .error)
            return
        }

        // Calculate sprite dimensions
        let spriteWidth = Float(texture.width) * scaleX
        let spriteHeight = Float(texture.height) * scaleY

        // Enhanced debug logging for texture rendering
        // log(
        //     "🖼️ Drawing sprite \(textureHandle): texture \(texture.width)x\(texture.height), screen \(spriteWidth)x\(spriteHeight), pos (\(x),\(y)), rot \(rotation)°",
        //     level: .debug)
        log("drawSpriteScaled: Texture pixel format: \(texture.pixelFormat)", level: .debug)
        log(
            "drawSpriteScaled: UV coordinates: (0,0) to (1,1) - full texture coverage",
            level: .debug)

        // DEBUG: Dump actual pixel data for 16x16 texture
        if texture.width == 16 && texture.height == 16 {
            dumpTexturePixelData(texture: texture, textureHandle: textureHandle)

            // DEBUG: Draw a magenta rectangle at the sprite position to verify positioning
            let debugDraw = false  // Set to true to enable debug rectangle
            if debugDraw {
                drawRectangle(
                    x: x - spriteWidth / 2, y: y - spriteHeight / 2, width: spriteWidth,
                    height: spriteHeight,
                    r: 1.0, g: 0.0, b: 1.0, a: 0.5)
                log("DEBUG: Drew debug rectangle at texture position", level: .debug)
                return  // Skip texture drawing to see just the rectangle
            }
        }

        // Use helper function to create sprite transformation matrix (top-left positioning)
        let modelMatrix = MetalMatrixHelpers.spriteTransformMatrix(
            position: (x: x, y: y),
            scale: (x: spriteWidth, y: spriteHeight),
            rotation: rotation
        )

        // Get current projection matrix
        let projectionMatrix = uniformBuffer.contents().bindMemory(
            to: simd_float4x4.self, capacity: 1
        ).pointee

        // Create MVP matrix
        let mvpMatrix = projectionMatrix * modelMatrix

        // DEBUG: Log the MVP matrix to see if coordinate system is correct
        // log("DEBUG: MVP Matrix for sprite at (\(x), \(y)):", level: .debug)
        // log("DEBUG: Projection Matrix: \(projectionMatrix)", level: .debug)
        // log("DEBUG: Model Matrix: \(modelMatrix)", level: .debug)
        // log("DEBUG: Final MVP Matrix: \(mvpMatrix)", level: .debug)

        // Create temporary uniform buffer for this sprite
        guard let device = device,
            let tempUniformBuffer = device.makeBuffer(
                bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: [])
        else {
            log("drawSpriteScaled: Failed to create temporary uniform buffer", level: .error)
            return
        }

        // Set up render encoder
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(tempUniformBuffer, offset: 0, index: 1)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.setFragmentSamplerState(samplerState, index: 0)

        // DEBUG: Log texture binding details
        log(
            "DEBUG: Texture bound to fragment shader - handle: \(textureHandle), texture: \(texture), sampler: \(samplerState?.label ?? "nil")",
            level: .debug)

        // DEBUG: Check buffers before drawing
        log(
            "DEBUG: Before drawIndexedPrimitives - indexBuffer: \(indexBuffer != nil ? "valid" : "nil"), vertexBuffer: \(vertexBuffer != nil ? "valid" : "nil")",
            level: .debug)

        // Draw the sprite
        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!,
            indexBufferOffset: 0)

        log("DEBUG: After drawIndexedPrimitives - call completed", level: .debug)

        // log(
        //     "drawSpriteScaled: Rendered sprite \(textureHandle) at (\(x), \(y)) with scale (\(scaleX), \(scaleY)) rotation \(rotation)°",
        //     level: .debug)
    }

    public func drawSpriteScaledWithSource(
        textureHandle: UInt32, x: Float, y: Float, scaleX: Float, scaleY: Float, rotation: Float,
        sourceX: Float, sourceY: Float, sourceWidth: Float, sourceHeight: Float
    ) {
        guard let texture = textures[textureHandle] else {
            log(
                "drawSpriteScaledWithSource: Invalid sprite handle \(textureHandle)",
                level: .warning)
            return
        }

        guard let uniformBuffer = uniformBuffer else {
            log("drawSpriteScaledWithSource: Missing required Metal resources", level: .warning)
            return
        }

        // Check if textured pipeline state is available
        guard let pipelineState = texturedPipelineState else {
            log("drawSpriteScaledWithSource: No pipeline state available", level: .warning)
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawSpriteScaledWithSource: Failed to get render encoder", level: .error)
            return
        }

        // Calculate sprite dimensions
        let spriteWidth = sourceWidth * scaleX
        let spriteHeight = sourceHeight * scaleY

        // Calculate pixel-perfect UV coordinates for the source rectangle
        // This prevents bleeding artifacts between adjacent textures in atlas
        let textureWidth = Float(texture.width)
        let textureHeight = Float(texture.height)

        // For pixel-perfect rendering, we need UV coordinates to align exactly with texel centers
        // Use half-pixel offset to ensure proper sampling
        let halfPixelU = 0.5 / textureWidth
        let halfPixelV = 0.5 / textureHeight

        // Calculate UV coordinates that align to texel centers
        let u0 = max(0.0, min(1.0, (sourceX / textureWidth) + halfPixelU))
        let v0 = max(0.0, min(1.0, (sourceY / textureHeight) + halfPixelV))
        let u1 = max(0.0, min(1.0, ((sourceX + sourceWidth) / textureWidth) - halfPixelU))
        let v1 = max(0.0, min(1.0, ((sourceY + sourceHeight) / textureHeight) - halfPixelV))

        // log(
        //     "🖼️ Drawing sprite with source rect: texture \(textureHandle), source (\(sourceX),\(sourceY),\(sourceWidth)x\(sourceHeight)), UV (\(u0),\(v0)) to (\(u1),\(v1)), screen \(spriteWidth)x\(spriteHeight), pos (\(x),\(y))",
        //     level: .debug)

        // Use sprite transformation matrix for centered positioning like other sprites
        let modelMatrix = MetalMatrixHelpers.spriteTransformMatrix(
            position: (x: x, y: y),
            scale: (x: spriteWidth, y: spriteHeight),
            rotation: 0.0
        )

        // Get current projection matrix
        let projectionMatrix = uniformBuffer.contents().bindMemory(
            to: simd_float4x4.self, capacity: 1
        ).pointee

        // Create MVP matrix
        let mvpMatrix = projectionMatrix * modelMatrix

        // Create temporary uniform buffer for this sprite
        guard let device = device,
            let tempUniformBuffer = device.makeBuffer(
                bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: [])
        else {
            log(
                "drawSpriteScaledWithSource: Failed to create temporary uniform buffer",
                level: .error)
            return
        }

        // Create vertex buffer with custom UV coordinates for the source rectangle
        // Use same vertex coordinates as static sprites to ensure consistent positioning
        let vertices: [Float] = [
            // Position (x, y), TexCoord (u, v), Color (r, g, b, a)
            0.0, 1.0, u0, v1, 1.0, 1.0, 1.0, 1.0,  // Bottom-left
            1.0, 1.0, u1, v1, 1.0, 1.0, 1.0, 1.0,  // Bottom-right
            1.0, 0.0, u1, v0, 1.0, 1.0, 1.0, 1.0,  // Top-right
            0.0, 0.0, u0, v0, 1.0, 1.0, 1.0, 1.0,  // Top-left
        ]

        guard
            let sourceVertexBuffer = device.makeBuffer(
                bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: [])
        else {
            log("drawSpriteScaledWithSource: Failed to create source vertex buffer", level: .error)
            return
        }

        // Set up render encoder
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setVertexBuffer(sourceVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(tempUniformBuffer, offset: 0, index: 1)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.setFragmentSamplerState(samplerState, index: 0)

        // Draw the sprite
        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!,
            indexBufferOffset: 0)

        // log(
        //     "drawSpriteScaledWithSource: Rendered sprite \(textureHandle) with source rect at (\(x), \(y))",
        //     level: .debug)
    }

    /// Draw a sprite with centered positioning (for rotation and special effects)
    public func drawSpriteScaledCentered(
        textureHandle: UInt32, x: Float, y: Float, scaleX: Float, scaleY: Float, rotation: Float
    ) {
        log(
            "drawSpriteScaledCentered: texture=\(textureHandle), pos=(\(x), \(y)), scale=(\(scaleX), \(scaleY)), rotation=\(rotation)°",
            level: .debug)

        guard let texture = textures[textureHandle] else {
            log("drawSpriteScaledCentered: Invalid sprite handle \(textureHandle)", level: .warning)
            return
        }

        guard let uniformBuffer = uniformBuffer else {
            log("drawSpriteScaledCentered: Missing required Metal resources", level: .warning)
            return
        }

        // Check if textured pipeline state is available
        guard let pipelineState = texturedPipelineState else {
            log("drawSpriteScaledCentered: No pipeline state available", level: .warning)
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawSpriteScaledCentered: Failed to get render encoder", level: .error)
            return
        }

        // Calculate sprite dimensions
        let spriteWidth = Float(texture.width) * scaleX
        let spriteHeight = Float(texture.height) * scaleY

        // Use centered sprite transformation matrix (with centering)
        let modelMatrix = MetalMatrixHelpers.spriteTransformMatrixCentered(
            position: (x: x, y: y),
            scale: (x: spriteWidth, y: spriteHeight),
            rotation: rotation
        )

        // Get current projection matrix
        let projectionMatrix = uniformBuffer.contents().bindMemory(
            to: simd_float4x4.self, capacity: 1
        ).pointee

        // Create MVP matrix
        let mvpMatrix = projectionMatrix * modelMatrix

        // Create temporary uniform buffer for this sprite
        guard let device = device,
            let tempUniformBuffer = device.makeBuffer(
                bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: [])
        else {
            log(
                "drawSpriteScaledCentered: Failed to create temporary uniform buffer", level: .error
            )
            return
        }

        // Set up render encoder
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(tempUniformBuffer, offset: 0, index: 1)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.setFragmentSamplerState(samplerState, index: 0)

        // Draw the sprite
        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!,
            indexBufferOffset: 0)

        // log(
        //     "drawSpriteScaledCentered: Rendered sprite \(textureHandle) centered at (\(x), \(y))",
        //     level: .debug)
    }

    /// Draw a sprite with custom pivot point rotation (for objects like spike balls rotating from base)
    /// Automatically uses RotSprite algorithm for high-quality pixel art rotation when available
    public func drawSpriteScaledPivoted(
        textureHandle: UInt32, x: Float, y: Float, scaleX: Float, scaleY: Float, rotation: Float,
        pivotX: Float, pivotY: Float
    ) {
        log(
            "drawSpriteScaledPivoted: textureId=\(textureHandle), position=(\(x), \(y)), scale=(\(scaleX), \(scaleY)), rotation=\(rotation), pivot=(\(pivotX), \(pivotY))",
            level: .debug)
        guard let texture = textures[textureHandle] else {
            log("drawSpriteScaledPivoted: Invalid sprite handle \(textureHandle)", level: .warning)
            return
        }

        log(
            "drawSpriteScaledPivoted: texture \(textureHandle) (\(texture.width)x\(texture.height)), rotation: \(rotation)°, pivot: (\(pivotX), \(pivotY))",
            level: .debug)

        // Check if we should use RotSprite for high-quality rotation
        // For pivot-based rotation, always use RotSprite when available (better quality for articulated objects)
        let shouldUseRotSprite =
            (abs(rotation) > 0.1 || abs(pivotX) > 0.1 || abs(pivotY) > 0.1)  // Significant rotation OR pivot offset
            && rotspriteUpscaleComputePipeline != nil  // RotSprite available
            && rotspriteRotateComputePipeline != nil && texture.width <= 256
            && texture.height <= 256  // Reasonable size for RotSprite

        if shouldUseRotSprite {
            log(
                "drawSpriteScaledPivoted: Using RotSprite for high-quality rotation (\(rotation)°)",
                level: .debug)
            drawSpriteRotSpriteInternal(
                texture: texture, textureHandle: textureHandle, x: x, y: y, scaleX: scaleX,
                scaleY: scaleY, rotation: rotation, pivotX: pivotX, pivotY: pivotY)
            return
        }

        // Standard matrix-based rotation for small rotations or when RotSprite is unavailable
        log("drawSpriteScaledPivoted: Using standard matrix rotation (\(rotation)°)", level: .debug)
        drawSpriteStandardPivoted(
            texture: texture, textureHandle: textureHandle, x: x, y: y, scaleX: scaleX,
            scaleY: scaleY, rotation: rotation, pivotX: pivotX, pivotY: pivotY)
    }

    /// Internal RotSprite rendering implementation
    private func drawSpriteRotSpriteInternal(
        texture: MTLTexture, textureHandle: UInt32, x: Float, y: Float, scaleX: Float,
        scaleY: Float, rotation: Float, pivotX: Float, pivotY: Float
    ) {
        // TODO: Re-implement caching to store both texture and rotation center
        // For now, disable caching to focus on getting the algorithm working
        // let cacheKey = "\(textureHandle)_\(rotation)"

        // Apply RotSprite algorithm with pivot point
        // CRITICAL: Process texture at 1x scale (raw dimensions), apply entity scale during rendering
        if let rotSpriteResult = processRotSprite(
            texture: texture, rotation: rotation, pivotX: pivotX, pivotY: pivotY)
        {
            log("drawSpriteScaledPivoted: Generated new RotSprite texture", level: .debug)
            // Apply the original scale during rendering
            renderRotSpriteTexture(
                rotSpriteResult.texture, at: (x: x, y: y), scale: (x: scaleX, y: scaleY),
                rotationCenter: rotSpriteResult.rotationCenter)
        } else {
            log(
                "drawSpriteScaledPivoted: RotSprite processing failed, falling back to standard rotation",
                level: .warning)
            drawSpriteStandardPivoted(
                texture: texture, textureHandle: textureHandle, x: x, y: y, scaleX: scaleX,
                scaleY: scaleY, rotation: rotation, pivotX: pivotX, pivotY: pivotY)
        }
    }

    /// Standard matrix-based pivoted rotation implementation
    private func drawSpriteStandardPivoted(
        texture: MTLTexture, textureHandle: UInt32, x: Float, y: Float, scaleX: Float,
        scaleY: Float, rotation: Float, pivotX: Float, pivotY: Float
    ) {
        guard let uniformBuffer = uniformBuffer else {
            log("drawSpriteStandardPivoted: Missing required Metal resources", level: .warning)
            return
        }

        // Check if textured pipeline state is available
        guard let pipelineState = texturedPipelineState else {
            log("drawSpriteStandardPivoted: No pipeline state available", level: .warning)
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawSpriteStandardPivoted: Failed to get render encoder", level: .error)
            return
        }

        // Calculate sprite dimensions
        let spriteWidth = Float(texture.width) * scaleX
        let spriteHeight = Float(texture.height) * scaleY

        // Use pivot-based sprite transformation matrix
        let modelMatrix = MetalMatrixHelpers.spriteTransformMatrixPivoted(
            position: (x: x, y: y),
            scale: (x: spriteWidth, y: spriteHeight),
            rotation: rotation,
            pivotX: pivotX,
            pivotY: pivotY
        )

        // Get current projection matrix
        let projectionMatrix = uniformBuffer.contents().bindMemory(
            to: simd_float4x4.self, capacity: 1
        ).pointee

        // Create MVP matrix
        let mvpMatrix = projectionMatrix * modelMatrix

        // Create temporary uniform buffer for this sprite
        guard let device = device,
            let tempUniformBuffer = device.makeBuffer(
                bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: [])
        else {
            log(
                "drawSpriteStandardPivoted: Failed to create temporary uniform buffer",
                level: .error)
            return
        }

        // Set up render encoder
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(tempUniformBuffer, offset: 0, index: 1)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.setFragmentSamplerState(samplerState, index: 0)

        // Draw the sprite
        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!,
            indexBufferOffset: 0)

        log(
            "drawSpriteStandardPivoted: Rendered sprite \(textureHandle) with pivot at (\(pivotX), \(pivotY))",
            level: .debug)
    }

    public func drawCircle(
        _ x: Float, _ y: Float, _ radius: Float, _ r: Float, _ g: Float, _ b: Float, _ a: Float
    ) {
        drawCircle(x: x, y: y, radius: radius, r: r, g: g, b: b, a: a, segments: 32)
    }

    // DEBUG: Function to dump texture pixel data
    private func dumpTexturePixelData(texture: MTLTexture, textureHandle: UInt32) {
        guard let device = device else { return }

        log(
            "DEBUG: Dumping texture \(textureHandle) - size: \(texture.width)x\(texture.height), format: \(texture.pixelFormat), storage: \(texture.storageMode)",
            level: .debug)

        // If texture is in shared storage mode, we can read it directly
        if texture.storageMode == .shared {
            let bytesPerRow = texture.width * 4  // RGBA = 4 bytes per pixel
            let region = MTLRegion(
                origin: MTLOrigin(x: 0, y: 0, z: 0),
                size: MTLSize(width: texture.width, height: texture.height, depth: 1))

            let bufferSize = texture.height * bytesPerRow
            let pixelData = UnsafeMutableRawPointer.allocate(byteCount: bufferSize, alignment: 4)
            defer { pixelData.deallocate() }

            texture.getBytes(pixelData, bytesPerRow: bytesPerRow, from: region, mipmapLevel: 0)

            let pixels = pixelData.bindMemory(to: UInt8.self, capacity: bufferSize)
            analyzeDumpedPixelData(pixels: pixels, texture: texture, textureHandle: textureHandle)
            return
        }

        // For private or managed storage, use blit encoder
        let bytesPerRow = texture.width * 4  // RGBA = 4 bytes per pixel
        let bufferSize = texture.height * bytesPerRow

        guard let buffer = device.makeBuffer(length: bufferSize, options: .storageModeShared) else {
            log("DEBUG: Failed to create buffer for texture data dump", level: .error)
            return
        }

        // Create a command buffer to copy texture to buffer
        guard let blitCommandBuffer = commandQueue?.makeCommandBuffer(),
            let blitEncoder = blitCommandBuffer.makeBlitCommandEncoder()
        else {
            log("DEBUG: Failed to create command buffer for texture data dump", level: .error)
            return
        }

        blitEncoder.copy(
            from: texture, sourceSlice: 0, sourceLevel: 0,
            sourceOrigin: MTLOrigin(x: 0, y: 0, z: 0),
            sourceSize: MTLSize(width: texture.width, height: texture.height, depth: 1), to: buffer,
            destinationOffset: 0, destinationBytesPerRow: bytesPerRow,
            destinationBytesPerImage: bufferSize, options: [])
        blitEncoder.endEncoding()

        blitCommandBuffer.commit()
        blitCommandBuffer.waitUntilCompleted()

        // Read the pixel data
        let pixelData = buffer.contents().bindMemory(to: UInt8.self, capacity: bufferSize)
        analyzeDumpedPixelData(pixels: pixelData, texture: texture, textureHandle: textureHandle)
    }

    private func analyzeDumpedPixelData(
        pixels: UnsafePointer<UInt8>, texture: MTLTexture, textureHandle: UInt32
    ) {
        // bytesPerRow not needed in this function anymore

        log(
            "DEBUG: Analyzing texture \(textureHandle) pixel data (\(texture.width)x\(texture.height) RGBA):",
            level: .debug)

        // Dump first few pixels to see if there's any non-zero data
        var hasNonZeroData = false
        var firstFewPixels = ""
        var nonTransparentPixels = 0
        var coloredPixels = 0
        let totalPixels = texture.width * texture.height

        // Check a sample of pixels for the corner display
        for y in 0..<min(4, texture.height) {
            for x in 0..<min(4, texture.width) {
                let pixelIndex = (y * texture.width + x) * 4
                let r = pixels[pixelIndex]
                let g = pixels[pixelIndex + 1]
                let b = pixels[pixelIndex + 2]
                let a = pixels[pixelIndex + 3]

                if r > 0 || g > 0 || b > 0 || a > 0 {
                    hasNonZeroData = true
                }

                firstFewPixels += " (\(r),\(g),\(b),\(a))"
            }
        }

        // Scan entire texture for colored pixels
        for y in 0..<texture.height {
            for x in 0..<texture.width {
                let pixelIndex = (y * texture.width + x) * 4
                let r = pixels[pixelIndex]
                let g = pixels[pixelIndex + 1]
                let b = pixels[pixelIndex + 2]
                let a = pixels[pixelIndex + 3]

                if a > 0 {
                    nonTransparentPixels += 1
                    // Check if this pixel has actual color (not just black)
                    if r > 0 || g > 0 || b > 0 {
                        coloredPixels += 1
                        // Log the first few colored pixels we find
                        if coloredPixels <= 5 {
                            log(
                                "DEBUG: Colored pixel \(coloredPixels) at (\(x),\(y)): RGBA(\(r),\(g),\(b),\(a))",
                                level: .debug)
                        }
                    }
                }
            }
        }

        log("DEBUG: First 16 pixels (4x4 corner): \(firstFewPixels)", level: .debug)
        log("DEBUG: Texture has non-zero data: \(hasNonZeroData)", level: .debug)
        log(
            "DEBUG: Non-transparent pixels: \(nonTransparentPixels)/\(totalPixels) (\(Int(Float(nonTransparentPixels) / Float(totalPixels) * 100))%)",
            level: .debug)
        log(
            "DEBUG: Colored pixels (non-black): \(coloredPixels)/\(nonTransparentPixels) (\(Int(Float(coloredPixels) / Float(max(1, nonTransparentPixels)) * 100))%)",
            level: .debug)

        if coloredPixels == 0 && nonTransparentPixels > 0 {
            log(
                "DEBUG: WARNING - All non-transparent pixels are black! Texture may not be loading colors correctly.",
                level: .error)
        } else if coloredPixels > 0 {
            log(
                "DEBUG: Found \(coloredPixels) colored pixels - texture appears to have color data",
                level: .debug)
        }
    }

    public func drawCircle(
        x: Float, y: Float, radius: Float, r: Float, g: Float, b: Float, a: Float,
        segments: Int = 32
    ) {
        guard let device = device,
            let renderPipelineState = renderPipelineState,
            let uniformBuffer = uniformBuffer
        else {
            log("drawCircle: Missing required Metal resources", level: .warning)
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawCircle: Failed to get render encoder", level: .error)
            return
        }

        // Generate circle vertices (triangle fan centered at origin, then translated)
        var vertices: [Float] = []

        // Center vertex
        vertices.append(contentsOf: [x, y, 0.5, 0.5, r, g, b, a])  // [x, y, u, v, r, g, b, a]

        // Generate perimeter vertices
        let angleStep = (Float.pi * 2.0) / Float(segments)
        for i in 0...segments {
            let angle = Float(i) * angleStep
            let vertexX = x + cos(angle) * radius
            let vertexY = y + sin(angle) * radius
            // Use polar coordinates for UV mapping for better texture support
            let u = 0.5 + cos(angle) * 0.5
            let v = 0.5 + sin(angle) * 0.5
            vertices.append(contentsOf: [vertexX, vertexY, u, v, r, g, b, a])
        }

        // Generate indices (triangle fan)
        var indices: [UInt16] = []
        for i in 1...segments {
            indices.append(contentsOf: [0, UInt16(i), UInt16(i + 1)])
        }

        // Create temporary buffers
        guard
            let tempVertexBuffer = device.makeBuffer(
                bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []),
            let tempIndexBuffer = device.makeBuffer(
                bytes: indices, length: indices.count * MemoryLayout<UInt16>.stride, options: [])
        else {
            log("drawCircle: Failed to create temporary buffers", level: .error)
            return
        }

        // Configure render encoder
        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)

        // Draw
        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: indices.count, indexType: .uint16,
            indexBuffer: tempIndexBuffer, indexBufferOffset: 0)

        log(
            "Circle drawn at (\(x), \(y)) with radius \(radius) using \(segments) segments",
            level: .debug)
    }

    public func drawTriangle(
        x1: Float, y1: Float, x2: Float, y2: Float, x3: Float, y3: Float, r: Float, g: Float,
        b: Float, a: Float
    ) {
        guard let device = device,
            let renderPipelineState = renderPipelineState,
            let uniformBuffer = uniformBuffer
        else {
            log("drawTriangle: Missing required Metal resources", level: .warning)
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawTriangle: Failed to get render encoder", level: .error)
            return
        }

        // Create triangle vertices
        let vertices: [Float] = [
            // Vertex 1
            x1, y1, 0.0, 0.0, r, g, b, a,
            // Vertex 2
            x2, y2, 1.0, 0.0, r, g, b, a,
            // Vertex 3
            x3, y3, 0.5, 1.0, r, g, b, a,
        ]

        let indices: [UInt16] = [0, 1, 2]

        // Create temporary buffers
        guard
            let tempVertexBuffer = device.makeBuffer(
                bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []),
            let tempIndexBuffer = device.makeBuffer(
                bytes: indices, length: indices.count * MemoryLayout<UInt16>.stride, options: [])
        else {
            log("drawTriangle: Failed to create temporary buffers", level: .error)
            return
        }

        // Configure render encoder
        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)

        // Draw
        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: indices.count, indexType: .uint16,
            indexBuffer: tempIndexBuffer, indexBufferOffset: 0)

        log(
            "Triangle drawn with vertices [(\(x1),\(y1)), (\(x2),\(y2)), (\(x3),\(y3))]",
            level: .debug)
    }

    public func drawLine(
        x1: Float, y1: Float, x2: Float, y2: Float, width: Float, r: Float, g: Float, b: Float,
        a: Float
    ) {
        // Calculate perpendicular vector for line thickness
        let dx = x2 - x1
        let dy = y2 - y1
        let length = sqrt(dx * dx + dy * dy)

        if length == 0 {
            log("drawLine: Zero-length line, skipping", level: .warning)
            return
        }

        // Normalize and create perpendicular vector
        let normX = dx / length
        let normY = dy / length
        let perpX = -normY * (width * 0.5)
        let perpY = normX * (width * 0.5)

        // Draw line as a rectangle (quad)
        let vertices: [Float] = [
            // Bottom-left
            x1 - perpX, y1 - perpY, 0.0, 0.0, r, g, b, a,
            // Bottom-right
            x2 - perpX, y2 - perpY, 1.0, 0.0, r, g, b, a,
            // Top-right
            x2 + perpX, y2 + perpY, 1.0, 1.0, r, g, b, a,
            // Top-left
            x1 + perpX, y1 + perpY, 0.0, 1.0, r, g, b, a,
        ]

        guard let device = device,
            let renderPipelineState = renderPipelineState,
            let uniformBuffer = uniformBuffer
        else {
            log("drawLine: Missing required Metal resources", level: .warning)
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawLine: Failed to get render encoder", level: .error)
            return
        }

        let indices: [UInt16] = [0, 1, 2, 2, 3, 0]

        // Create temporary buffers
        guard
            let tempVertexBuffer = device.makeBuffer(
                bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []),
            let tempIndexBuffer = device.makeBuffer(
                bytes: indices, length: indices.count * MemoryLayout<UInt16>.stride, options: [])
        else {
            log("drawLine: Failed to create temporary buffers", level: .error)
            return
        }

        // Configure render encoder
        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)

        // Draw
        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: indices.count, indexType: .uint16,
            indexBuffer: tempIndexBuffer, indexBufferOffset: 0)

        log("Line drawn from (\(x1),\(y1)) to (\(x2),\(y2)) with width \(width)", level: .debug)
    }

    public func drawRoundedRectangle(
        x: Float, y: Float, width: Float, height: Float, cornerRadius: Float, r: Float, g: Float,
        b: Float, a: Float
    ) {
        // For smooth rounded rectangles, we'll use a more sophisticated approach
        // For now, let's implement it as a combination of rectangles and quarter-circles

        let clampedRadius = min(cornerRadius, min(width, height) * 0.5)

        if clampedRadius <= 0 {
            // No rounding, draw regular rectangle
            drawRectangle(x: x, y: y, width: width, height: height, r: r, g: g, b: b, a: a)
            return
        }

        // Draw main rectangles (avoiding corners)
        // Center rectangle
        drawRectangle(
            x: x + clampedRadius, y: y + clampedRadius,
            width: width - 2 * clampedRadius, height: height - 2 * clampedRadius,
            r: r, g: g, b: b, a: a)

        // Top rectangle
        drawRectangle(
            x: x + clampedRadius, y: y,
            width: width - 2 * clampedRadius, height: clampedRadius,
            r: r, g: g, b: b, a: a)

        // Bottom rectangle
        drawRectangle(
            x: x + clampedRadius, y: y + height - clampedRadius,
            width: width - 2 * clampedRadius, height: clampedRadius,
            r: r, g: g, b: b, a: a)

        // Left rectangle
        drawRectangle(
            x: x, y: y + clampedRadius,
            width: clampedRadius, height: height - 2 * clampedRadius,
            r: r, g: g, b: b, a: a)

        // Right rectangle
        drawRectangle(
            x: x + width - clampedRadius, y: y + clampedRadius,
            width: clampedRadius, height: height - 2 * clampedRadius,
            r: r, g: g, b: b, a: a)

        // Draw corner circles
        let segments = 8  // Quarter circle segments for performance

        // Top-left corner
        drawQuarterCircle(
            centerX: x + clampedRadius, centerY: y + clampedRadius,
            radius: clampedRadius, startAngle: Float.pi,
            r: r, g: g, b: b, a: a, segments: segments)

        // Top-right corner
        drawQuarterCircle(
            centerX: x + width - clampedRadius, centerY: y + clampedRadius,
            radius: clampedRadius, startAngle: Float.pi * 1.5,
            r: r, g: g, b: b, a: a, segments: segments)

        // Bottom-right corner
        drawQuarterCircle(
            centerX: x + width - clampedRadius, centerY: y + height - clampedRadius,
            radius: clampedRadius, startAngle: 0,
            r: r, g: g, b: b, a: a, segments: segments)

        // Bottom-left corner
        drawQuarterCircle(
            centerX: x + clampedRadius, centerY: y + height - clampedRadius,
            radius: clampedRadius, startAngle: Float.pi * 0.5,
            r: r, g: g, b: b, a: a, segments: segments)

        log(
            "Rounded rectangle drawn at (\(x), \(y)) size (\(width), \(height)) with corner radius \(clampedRadius)",
            level: .debug)
    }

    private func drawQuarterCircle(
        centerX: Float, centerY: Float, radius: Float, startAngle: Float,
        r: Float, g: Float, b: Float, a: Float, segments: Int
    ) {
        guard let device = device,
            let renderPipelineState = renderPipelineState,
            let uniformBuffer = uniformBuffer
        else {
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            return
        }

        var vertices: [Float] = []
        var indices: [UInt16] = []

        // Center vertex
        vertices.append(contentsOf: [centerX, centerY, 0.5, 0.5, r, g, b, a])

        // Arc vertices
        let angleStep = (Float.pi * 0.5) / Float(segments)
        for i in 0...segments {
            let angle = startAngle + Float(i) * angleStep
            let vertexX = centerX + cos(angle) * radius
            let vertexY = centerY + sin(angle) * radius
            let u = 0.5 + cos(angle) * 0.5
            let v = 0.5 + sin(angle) * 0.5
            vertices.append(contentsOf: [vertexX, vertexY, u, v, r, g, b, a])
        }

        // Generate triangle fan indices
        for i in 1...segments {
            indices.append(contentsOf: [0, UInt16(i), UInt16(i + 1)])
        }

        guard
            let tempVertexBuffer = device.makeBuffer(
                bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []),
            let tempIndexBuffer = device.makeBuffer(
                bytes: indices, length: indices.count * MemoryLayout<UInt16>.stride, options: [])
        else {
            return
        }

        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)

        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: indices.count, indexType: .uint16,
            indexBuffer: tempIndexBuffer, indexBufferOffset: 0)
    }

    // MARK: - Text Rendering (CT-based)
    public func loadFont(fontName: String, fontSize: Float) -> Bool {
        log("Loading font: \(fontName) with size: \(fontSize)", level: .debug)
        // Load TTF into CTFont and cache metrics; no SDF/MSDF
        guard
            let fontURL = Bundle.main.url(
                forResource: fontName, withExtension: "ttf", subdirectory: "fonts")
                ?? Bundle.main.url(forResource: fontName, withExtension: "ttf")
        else {
            log("Failed to find TTF font: \(fontName).ttf", level: .error)
            return false
        }
        guard let fontData = try? Data(contentsOf: fontURL),
            let dataProvider = CGDataProvider(data: fontData as CFData),
            let cgFont = CGFont(dataProvider)
        else {
            log("Failed to create CGFont from: \(fontURL.path)", level: .error)
            return false
        }
        let ctFont = CTFontCreateWithGraphicsFont(cgFont, CGFloat(fontSize), nil, nil)
        self.ctFont = ctFont
        let ascent = CTFontGetAscent(ctFont)
        let descent = CTFontGetDescent(ctFont)
        let leading = CTFontGetLeading(ctFont)
        let lineHeight = ascent + descent + leading
        fontMetrics = FontMetrics(
            size: Float(lineHeight),
            lineHeight: Float(lineHeight),
            ascender: Float(ascent),
            descender: Float(descent),
            base: Float(ascent),
            atlasWidth: 0,
            atlasHeight: 0
        )
        return true
    }

    // Removed legacy SDF/MSDF loading helpers

    // Removed MSDF stubs and CSV parsing

    public func drawTextWithOutline(
        _ text: String, x: Float, y: Float, fontSize: Float,
        textR: Float, textG: Float, textB: Float, textA: Float,
        outlineR: Float, outlineG: Float, outlineB: Float, outlineA: Float,
        outlineWidth: Float
    ) {
        log("drawTextWithOutline not yet implemented", level: .warning)
    }

    public func drawTextWithShadow(
        _ text: String, x: Float, y: Float, fontSize: Float,
        textR: Float, textG: Float, textB: Float, textA: Float,
        shadowR: Float, shadowG: Float, shadowB: Float, shadowA: Float,
        shadowOffsetX: Float, shadowOffsetY: Float
    ) {
        log("drawTextWithShadow not yet implemented", level: .warning)
    }

    // MARK: - Screen Size Query

    public func getScreenSize() -> (width: Float, height: Float) {
        return (width: Float(viewportSize.width), height: Float(viewportSize.height))
    }

    // MARK: - Enhanced Screen Information

    public func getScreenInfo() -> GameCore.ScreenInfo {
        // CRITICAL FIX: Use MTKView drawable size for actual rendered dimensions
        // UIScreen.nativeBounds always returns portrait dimensions regardless of orientation
        // MTKView.drawableSize gives us the actual current render target dimensions
        var pixelWidth: Float
        var pixelHeight: Float
        var logicalWidth: Float
        var logicalHeight: Float
        var scaleFactor: Float

        if let metalView = metalView {
            // Use MTKView drawable size for accurate current dimensions
            let drawableSize = metalView.drawableSize
            pixelWidth = Float(drawableSize.width)
            pixelHeight = Float(drawableSize.height)

            // Get logical size from view bounds
            let viewBounds = metalView.bounds
            logicalWidth = Float(viewBounds.width)
            logicalHeight = Float(viewBounds.height)

            // Calculate scale from ratio
            scaleFactor = Float(UIScreen.main.nativeScale)

            log(
                "Using MTKView dimensions - Drawable: \(pixelWidth)x\(pixelHeight), Bounds: \(logicalWidth)x\(logicalHeight)",
                level: .debug)
        } else {
            // Fallback to UIScreen if metalView not set yet
            let mainScreen = UIScreen.main
            let logicalBounds = mainScreen.bounds
            logicalWidth = Float(logicalBounds.width)
            logicalHeight = Float(logicalBounds.height)

            // Use current window scene orientation instead of nativeBounds
            if let windowScene = UIApplication.shared.connectedScenes.first as? UIWindowScene {
                let interfaceOrientation = windowScene.interfaceOrientation
                let nativeBounds = mainScreen.nativeBounds

                if interfaceOrientation.isPortrait {
                    // Portrait: use native bounds as-is
                    pixelWidth = Float(nativeBounds.width)
                    pixelHeight = Float(nativeBounds.height)
                } else {
                    // Landscape: swap dimensions
                    pixelWidth = Float(nativeBounds.height)
                    pixelHeight = Float(nativeBounds.width)
                }
            } else {
                // Last resort fallback
                let nativeBounds = mainScreen.nativeBounds
                pixelWidth = Float(nativeBounds.width)
                pixelHeight = Float(nativeBounds.height)
            }

            scaleFactor = Float(mainScreen.nativeScale)
            log("Using UIScreen fallback - Pixel: \(pixelWidth)x\(pixelHeight)", level: .warning)
        }

        // Get device model (simplified)
        let deviceModel = getDeviceModel()

        // Create and return ScreenInfo struct
        // IMPORTANT: Return by value only. Do not store the pointer passed from C++.
        var screenInfo = GameCore.ScreenInfo()
        screenInfo.logicalWidth = logicalWidth
        screenInfo.logicalHeight = logicalHeight

        // CRITICAL FIX: Use MTKView drawable size EXACTLY as-is
        // MTKView already gives us the correct dimensions for the current orientation
        // DO NOT swap or adjust dimensions - this causes the double-update bug
        screenInfo.pixelWidth = pixelWidth
        screenInfo.pixelHeight = pixelHeight

        // Determine orientation from actual drawable dimensions
        // This is the ground truth - if height > width, we're in portrait
        screenInfo.isPortrait = pixelHeight > pixelWidth

        screenInfo.scaleFactor = scaleFactor
        screenInfo.deviceModel = std.string(deviceModel)

        log(
            "ScreenInfo: \(Int(pixelWidth))x\(Int(pixelHeight)), portrait: \(screenInfo.isPortrait)",
            level: .info)

        return screenInfo
    }

    /// Force an immediate screen info update and push to C++ ConfigManager
    /// Call this after orientation changes to ensure dimensions are refreshed
    public func updateScreenInfo() {
        let screenInfo = getScreenInfo()

        log(
            "Forcing screen info update: \(screenInfo.pixelWidth)x\(screenInfo.pixelHeight), portrait: \(screenInfo.isPortrait)",
            level: .info)

        // Push updated screen info to C++ ConfigManager
        GameCorePlatform.GameCore.setScreenInfoDirect(screenInfo)

        log("Screen info pushed to C++ ConfigManager", level: .debug)
    }

    private func getDeviceModel() -> String {
        var systemInfo = utsname()
        uname(&systemInfo)
        let modelCode = withUnsafePointer(to: &systemInfo.machine) {
            $0.withMemoryRebound(to: CChar.self, capacity: 1) {
                ptr in String.init(validatingCString: ptr)
            }
        }

        // Map common device codes to readable names
        switch modelCode {
        case "iPhone16,1": return "iPhone 15 Pro"
        case "iPhone16,2": return "iPhone 15 Pro Max"
        case "iPhone15,4": return "iPhone 15"
        case "iPhone15,5": return "iPhone 15 Plus"
        case "iPhone17,1": return "iPhone 16 Pro"
        case "iPhone17,2": return "iPhone 16 Pro Max"
        case "iPhone17,3": return "iPhone 16"
        case "iPhone17,4": return "iPhone 16 Plus"
        default: return modelCode ?? "Unknown iPhone"
        }
    }

    // MARK: - Texture Metadata

    public func getTextureMetadata(textureId: String) -> GameCore.TextureMetadata? {
        log("Getting texture metadata for: \(textureId)", level: .debug)

        // Use AssetManager to get texture metadata - it handles both cached and uncached textures
        let metadata = AssetManager.shared.getTextureMetadata(name: textureId)

        // Convert to GameCore.TextureMetadata format
        var platformMetadata = GameCore.TextureMetadata()
        platformMetadata.width = Int32(metadata.width)
        platformMetadata.height = Int32(metadata.height)
        platformMetadata.channels = Int32(metadata.channels)
        platformMetadata.format = std.string(metadata.format)
        platformMetadata.dataSize = Int(metadata.dataSize)
        platformMetadata.isLoaded = metadata.isLoaded
        platformMetadata.assetPath = std.string(metadata.assetPath)
        platformMetadata.platformHandle = 0  // We don't expose handles through metadata

        log(
            "Texture metadata for \(textureId): \(metadata.width)x\(metadata.height), loaded: \(metadata.isLoaded)",
            level: .debug)

        return platformMetadata
    }

    // MARK: - Helper Methods

    private func generateGlyphMetricsFromTTF(ctFont: CTFont, atlasWidth: Float, atlasHeight: Float)
        -> Bool
    {
        log("Generating glyph metrics from TTF font", level: .debug)

        // Clear existing glyph map
        glyphMap.removeAll()

        // Simple ordered character set (ASCII 32-126)
        let characters = Array(
            " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
        )

        // Fixed grid layout: 16x6 = 96 characters (ASCII 32-127)
        let gridCols = 16
        let gridRows = 6
        let cellWidth = atlasWidth / Float(gridCols)
        let cellHeight = atlasHeight / Float(gridRows)

        // Generate metrics for each character in fixed grid order
        for (index, character) in characters.enumerated() {
            // Calculate grid position
            let col = index % gridCols
            let row = index / gridCols

            // Calculate UV coordinates based on grid position
            let u = Float(col) * cellWidth / atlasWidth
            let v = Float(row) * cellHeight / atlasHeight
            let uWidth = cellWidth / atlasWidth
            let vHeight = cellHeight / atlasHeight

            // Get glyph metrics for this character
            let unichars = [UniChar(character.unicodeScalars.first!.value)]
            var glyphs = [CGGlyph](repeating: 0, count: 1)
            let success = CTFontGetGlyphsForCharacters(ctFont, unichars, &glyphs, 1)

            guard success && glyphs[0] != 0 else {
                log("Failed to get glyph for character: \(character)", level: .warning)
                continue
            }

            var glyph = glyphs[0]

            // Get glyph metrics
            var boundingRect = CGRect.zero
            CTFontGetBoundingRectsForGlyphs(ctFont, .horizontal, &glyph, &boundingRect, 1)

            var advance = CGSize.zero
            CTFontGetAdvancesForGlyphs(ctFont, .horizontal, &glyph, &advance, 1)

            // Create glyph info with predictable grid-based UV coordinates
            let glyphInfo = GlyphInfo(
                atlasX: u,
                atlasY: v,
                atlasWidth: uWidth,
                atlasHeight: vHeight,
                bearingX: Float(boundingRect.origin.x),
                bearingY: Float(boundingRect.origin.y),
                advance: Float(advance.width),
                width: Float(boundingRect.width),
                height: Float(boundingRect.height)
            )

            glyphMap[character] = glyphInfo
            log(
                "Generated glyph metrics for '\(character)' at grid position (\(col), \(row)): UV(\(u), \(v)) Size(\(uWidth), \(vHeight))",
                level: .debug)
        }

        log(
            "Generated \(glyphMap.count) glyph metrics from TTF using fixed grid layout",
            level: .debug)
        return !glyphMap.isEmpty
    }

    private func loadBMFontGlyphs(fontName: String) -> Bool {
        log("Loading BMFont glyphs for: \(fontName)", level: .debug)

        // Try to load BMFont file from bundle
        guard
            let fntURL = Bundle.main.url(
                forResource: fontName, withExtension: "fnt", subdirectory: "fonts")
        else {
            log("BMFont file not found in fonts subdirectory, trying main bundle", level: .debug)
            guard let fntURL = Bundle.main.url(forResource: fontName, withExtension: "fnt") else {
                log("Failed to find BMFont file: \(fontName).fnt", level: .error)
                return false
            }
            return loadBMFontFromURL(fntURL)
        }

        return loadBMFontFromURL(fntURL)
    }

    private func loadBMFontFromURL(_ fntURL: URL) -> Bool {
        do {
            let fntContent = try String(contentsOf: fntURL)
            let lines = fntContent.components(separatedBy: .newlines)

            var atlasWidth: Float = 0
            var atlasHeight: Float = 0

            // Parse BMFont file
            for line in lines {
                let trimmedLine = line.trimmingCharacters(in: .whitespaces)
                if trimmedLine.isEmpty { continue }

                let parts = trimmedLine.components(separatedBy: " ")
                if parts.isEmpty { continue }

                let command = parts[0]

                switch command {
                case "info":
                    // Parse info line (font info)
                    break
                case "common":
                    // Parse common line (atlas info)
                    for part in parts.dropFirst() {
                        if part.hasPrefix("scaleW=") {
                            atlasWidth = Float(part.dropFirst(7)) ?? 0
                        } else if part.hasPrefix("scaleH=") {
                            atlasHeight = Float(part.dropFirst(7)) ?? 0
                        }
                    }
                case "char":
                    // Parse character line
                    var charId: Int = 0
                    var x: Float = 0
                    var y: Float = 0
                    var width: Float = 0
                    var height: Float = 0
                    var xoffset: Float = 0
                    var yoffset: Float = 0
                    var xadvance: Float = 0

                    for part in parts.dropFirst() {
                        if part.hasPrefix("id=") {
                            charId = Int(part.dropFirst(3)) ?? 0
                        } else if part.hasPrefix("x=") {
                            x = Float(part.dropFirst(2)) ?? 0
                        } else if part.hasPrefix("y=") {
                            y = Float(part.dropFirst(2)) ?? 0
                        } else if part.hasPrefix("width=") {
                            width = Float(part.dropFirst(6)) ?? 0
                        } else if part.hasPrefix("height=") {
                            height = Float(part.dropFirst(7)) ?? 0
                        } else if part.hasPrefix("xoffset=") {
                            xoffset = Float(part.dropFirst(8)) ?? 0
                        } else if part.hasPrefix("yoffset=") {
                            yoffset = Float(part.dropFirst(8)) ?? 0
                        } else if part.hasPrefix("xadvance=") {
                            xadvance = Float(part.dropFirst(9)) ?? 0
                        }
                    }

                    // Create glyph info
                    let glyphInfo = GlyphInfo(
                        atlasX: x / atlasWidth,
                        atlasY: y / atlasHeight,
                        atlasWidth: width / atlasWidth,
                        atlasHeight: height / atlasHeight,
                        bearingX: xoffset,
                        bearingY: yoffset,
                        advance: xadvance,
                        width: width,
                        height: height
                    )

                    // Store in glyph map
                    let char = Character(UnicodeScalar(charId)!)
                    glyphMap[char] = glyphInfo
                default:
                    break
                }
            }

            log("Loaded \(glyphMap.count) glyphs from BMFont file", level: .debug)
            return !glyphMap.isEmpty

        } catch {
            log("Failed to load BMFont file: \(error)", level: .error)
            return false
        }
    }

    private func createTexture(width: Int, height: Int, pixelFormat: MTLPixelFormat = .rgba8Unorm)
        -> MTLTexture?
    {
        guard let device = device else { return nil }

        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: pixelFormat,
            width: width,
            height: height,
            mipmapped: false
        )

        return device.makeTexture(descriptor: descriptor)
    }

    // MARK: - Text Measurement
    // Rasterize text to texture and draw. Optional outline.
    private func drawTextRaster(
        _ text: String,
        x: Float,
        y: Float,
        fontSize: Float,
        fill: SIMD4<Float>,
        outline: (color: SIMD4<Float>, widthPx: Float)?,
        isCentered: Bool = false
    ) {
        startTiming("DrawTextRaster")
        defer { endTiming("DrawTextRaster") }
        
        guard let device = device, let ctBase = self.ctFont else { return }
        let sizeInPoints = CGFloat(fontSize) / deviceScale
        let ctFontSized = CTFontCreateCopyWithAttributes(ctBase, sizeInPoints, nil, nil)
        let key =
            "raster|\(text)|\(fontSize)|fill:\(fill.x),\(fill.y),\(fill.z),\(fill.w)|outline:\(outline?.widthPx ?? 0)"
        let pad: CGFloat = 4
        var entry = textTextureCache[key]
        if entry == nil {
            // Measure
            let color = CGColor(
                red: CGFloat(fill.x), green: CGFloat(fill.y), blue: CGFloat(fill.z),
                alpha: CGFloat(fill.w))
            let attr: [NSAttributedString.Key: Any] = [
                kCTFontAttributeName as NSAttributedString.Key: ctFontSized,
                kCTForegroundColorAttributeName as NSAttributedString.Key: color,
            ]
            let attributed = NSAttributedString(string: text, attributes: attr)
            let line = CTLineCreateWithAttributedString(attributed)
            let widthPt = CTLineGetTypographicBounds(line, nil, nil, nil)
            let ascent = CTFontGetAscent(ctFontSized)
            let descent = CTFontGetDescent(ctFontSized)
            let heightPt = ascent + descent
            let w = Int(ceil(widthPt * deviceScale + 2 * pad))
            let h = Int(ceil(heightPt * deviceScale + 2 * pad))
            let cs = CGColorSpaceCreateDeviceRGB()
            guard
                let ctx = CGContext(
                    data: nil, width: w, height: h, bitsPerComponent: 8, bytesPerRow: 4 * w,
                    space: cs, bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)
            else { return }
            ctx.setAllowsAntialiasing(true)
            ctx.setShouldAntialias(true)
            ctx.interpolationQuality = .high
            ctx.setFillColor(UIColor.clear.cgColor)
            ctx.fill(CGRect(x: 0, y: 0, width: w, height: h))
            // Scale for points -> pixels (no vertical flip)
            ctx.scaleBy(x: deviceScale, y: deviceScale)
            ctx.textMatrix = .identity
            let origin = CGPoint(x: pad / deviceScale, y: pad / deviceScale + descent)
            ctx.textPosition = origin
            if let outline = outline, outline.widthPx > 0.5 {
                let widthPt = CGFloat(outline.widthPx) / deviceScale
                let runArray = CTLineGetGlyphRuns(line) as! [CTRun]
                ctx.setLineWidth(widthPt)
                ctx.setLineJoin(.round)
                ctx.setLineCap(.round)
                ctx.setStrokeColor(
                    red: CGFloat(outline.color.x), green: CGFloat(outline.color.y),
                    blue: CGFloat(outline.color.z), alpha: CGFloat(outline.color.w))
                for run in runArray {
                    let runGlyphCount = CTRunGetGlyphCount(run)
                    var glyphs = [CGGlyph](repeating: 0, count: runGlyphCount)
                    var positions = [CGPoint](repeating: .zero, count: runGlyphCount)
                    CTRunGetGlyphs(run, CFRange(location: 0, length: 0), &glyphs)
                    CTRunGetPositions(run, CFRange(location: 0, length: 0), &positions)
                    for i in 0..<runGlyphCount {
                        if let path = CTFontCreatePathForGlyph(ctFontSized, glyphs[i], nil) {
                            ctx.saveGState()
                            // Match CTLineDraw baseline by adding the same origin used above
                            ctx.translateBy(x: origin.x, y: origin.y)
                            ctx.translateBy(x: positions[i].x, y: positions[i].y)
                            ctx.addPath(path)
                            ctx.strokePath()
                            ctx.restoreGState()
                        }
                    }
                }
            }
            // Fill
            ctx.setFillColor(
                red: CGFloat(fill.x), green: CGFloat(fill.y), blue: CGFloat(fill.z),
                alpha: CGFloat(fill.w))
            CTLineDraw(line, ctx)
            guard let cg = ctx.makeImage() else { return }
            let loader = MTKTextureLoader(device: device)
            guard
                let tex = try? loader.newTexture(
                    cgImage: cg, options: [.SRGB: false, .generateMipmaps: false])
            else { return }
            entry = (tex, CGSize(width: w, height: h), ascent * deviceScale, pad, 0)
            textTextureCache[key] = entry
        }
        guard var cached = entry else { return }
        // Draw textured quad
        let drawX: Float
        let drawY: Float
        if isCentered {
            drawX = x - Float(cached.sizePx.width) * 0.5
            drawY = y - Float(cached.sizePx.height) * 0.5
        } else {
            let baseline = y - Float(cached.ascentPx)
            drawX = x
            drawY = baseline
        }
        // Register once and reuse the same handle across frames; release handle only on explicit cache clear
        if cached.handle == 0 || !isHandleValid(cached.handle) {
            let handle = registerTexture(cached.texture)
            cached.handle = handle
            textTextureCache[key] = cached
        }
        drawTexture(
            textureHandle: cached.handle, x: drawX, y: drawY, width: Float(cached.sizePx.width),
            height: Float(cached.sizePx.height))
    }

    public func measureText(_ text: String, fontSize: Float) -> (width: Float, height: Float) {
        // Prefer CoreText measurement for exact advances/kerning and tight bounds, at requested size
        if let ctBase = self.ctFont {
            let sizeInPoints = CGFloat(fontSize) / deviceScale
            let ctFontSized = CTFontCreateCopyWithAttributes(ctBase, sizeInPoints, nil, nil)
            let trackingPx: Float = max(0.0, fontSize * 0.08)
            var maxWidthPx: Float = 0
            var lineCount: Int = 0
            // Measure each line separately to add our tracking consistently
            let lines = text.split(separator: "\n", omittingEmptySubsequences: false)
            for lineTextSub in lines {
                let lineText = String(lineTextSub)
                let attr = [kCTFontAttributeName as NSAttributedString.Key: ctFontSized]
                let attributed = NSAttributedString(string: lineText, attributes: attr)
                let line = CTLineCreateWithAttributedString(attributed)
                let widthPt = CTLineGetTypographicBounds(line, nil, nil, nil)
                // Add tracking for (glyphs-1). Approximate glyph count by character count for UI strings
                let glyphCount = max(0, lineText.count - 1)
                let widthPx = Float(widthPt) * Float(deviceScale) + trackingPx * Float(glyphCount)
                maxWidthPx = max(maxWidthPx, widthPx)
                lineCount += 1
            }
            // Vertical: use CT font metrics at this size
            let lineHeightPt =
                CTFontGetAscent(ctFontSized) + CTFontGetDescent(ctFontSized)
                + CTFontGetLeading(ctFontSized)
            let totalHeightPx = Float(lineHeightPt * CGFloat(lineCount) * deviceScale)
            return (width: maxWidthPx, height: totalHeightPx)
        }
        // Fallback to previous estimate if CT not available
        guard let fontMetrics = fontMetrics else { return (0, 0) }
        let scale = fontSize / max(fontMetrics.lineHeight, 1.0)
        let trackingPx: Float = max(0.0, fontSize * 0.08)
        var maxWidth: Float = 0
        var currentLineWidth: Float = 0
        var lineCount: Int = 1
        for char in text {
            if char == "\n" {
                maxWidth = max(maxWidth, currentLineWidth)
                currentLineWidth = 0
                lineCount += 1
                continue
            }
            guard let glyph = glyphMap[char] else {
                currentLineWidth += fontSize * 0.5
                continue
            }
            currentLineWidth += glyph.advance * scale + trackingPx
        }
        maxWidth = max(maxWidth, currentLineWidth)
        if !text.isEmpty && !text.hasSuffix("\n") { maxWidth -= trackingPx }
        let lineHeight = fontMetrics.lineHeight * scale
        let totalHeight = Float(lineCount) * lineHeight
        return (width: maxWidth, height: totalHeight)
    }

    public func drawTextCentered(
        _ text: String, x: Float, y: Float, fontSize: Float, r: Float, g: Float, b: Float, a: Float
    ) {
        // For multi-line center, split and stack around center Y
        // Split on literal backslash-n sequences and real newlines
        let expanded = text.replacingOccurrences(of: "\\n", with: "\n")
        let lines = expanded.split(separator: "\n", omittingEmptySubsequences: false)
        let lineHeight = fontSize * 1.1
        let totalHeight = lineHeight * Float(lines.count)
        var startY = y - totalHeight * 0.5 + lineHeight * 0.5
        for line in lines {
            // compute baseline correction for each line via measure
            let measured = measureText(String(line), fontSize: fontSize)
            var baselineY = startY - measured.height * 0.5
            if let ctBase = self.ctFont {
                let sizeInPoints = CGFloat(fontSize) / deviceScale
                let ctFontSized = CTFontCreateCopyWithAttributes(ctBase, sizeInPoints, nil, nil)
                let attr = [kCTFontAttributeName as NSAttributedString.Key: ctFontSized]
                let attributed = NSAttributedString(string: String(line), attributes: attr)
                let lineCT = CTLineCreateWithAttributedString(attributed)
                var overall = CGRect.null
                let runs = CTLineGetGlyphRuns(lineCT) as! [CTRun]
                for run in runs {
                    let rb = CTRunGetImageBounds(run, nil, CFRange(location: 0, length: 0))
                    overall = overall.union(rb)
                }
                baselineY = Float(CGFloat(startY) - overall.midY * deviceScale)
            }
            drawTextRaster(
                String(line), x: x, y: baselineY, fontSize: fontSize,
                fill: SIMD4<Float>(r, g, b, a), outline: nil, isCentered: true)
            startY += lineHeight
        }
    }

    public func drawTextCenteredOutlined(
        _ text: String, x: Float, y: Float, fontSize: Float,
        textR: Float, textG: Float, textB: Float, textA: Float,
        outlineR: Float, outlineG: Float, outlineB: Float, outlineA: Float,
        outlineWidth: Float
    ) {
        let measured2 = measureText(text, fontSize: fontSize)
        var baselineY = y - measured2.height * 0.5
        if let ctBase = self.ctFont {
            let sizeInPoints = CGFloat(fontSize) / deviceScale
            let ctFontSized = CTFontCreateCopyWithAttributes(ctBase, sizeInPoints, nil, nil)
            let attr = [kCTFontAttributeName as NSAttributedString.Key: ctFontSized]
            let attributed = NSAttributedString(string: text, attributes: attr)
            let line = CTLineCreateWithAttributedString(attributed)
            var overall = CGRect.null
            let runs = CTLineGetGlyphRuns(line) as! [CTRun]
            for run in runs {
                let rb = CTRunGetImageBounds(run, nil, CFRange(location: 0, length: 0))
                overall = overall.union(rb)
            }
            baselineY = Float(CGFloat(y) - overall.midY * deviceScale)
        }
        log(
            "drawTextCenteredOutlined: center=(\(x),\(y)) size=(\(measured2.width),\(measured2.height)) baselineY=\(baselineY) outlineWidth=\(outlineWidth)",
            level: .debug)
        drawTextRaster(
            text,
            x: x,
            y: y,
            fontSize: fontSize,
            fill: SIMD4<Float>(textR, textG, textB, textA),
            outline: (
                color: SIMD4<Float>(outlineR, outlineG, outlineB, outlineA), widthPx: outlineWidth
            ),
            isCentered: true)
    }

    // MARK: - RotSprite Implementation

    /// Process a texture using the RotSprite algorithm for high-quality rotation
    private func processRotSprite(
        texture: MTLTexture, rotation: Float, pivotX: Float, pivotY: Float
    ) -> (texture: MTLTexture, rotationCenter: SIMD2<Float>)? {
        log("🔥 ROTSPRITE CALLED: rotation=\(rotation)°, pivot=(\(pivotX), \(pivotY))", level: .info)

        guard let device = device,
            let commandQueue = commandQueue,
            let upscalePipeline = rotspriteUpscaleComputePipeline,
            let rotatePipeline = rotspriteRotateComputePipeline
        else {
            return nil
        }

        let originalWidth = Float(texture.width)
        let originalHeight = Float(texture.height)

        // Normalized pivot offsets (no extra flip—keep sign for direction)
        // Assuming pivotY negative for above center (towards smaller Y, top)
        let normalizedPivotX = pivotX / originalWidth
        let normalizedPivotY = pivotY / originalHeight  // Key fix: No '-', so -45/90 = -0.5 for top

        // Input center for upscaled (pivot position normalized, 0=top-left Y)
        let inputCenterX = 0.5 + normalizedPivotX
        let inputCenterY = 0.5 + normalizedPivotY  // For top pivot: 0.5 - 0.5 = 0 (top)

        // Square Quad approach: Calculate maximum distance from pivot to any corner
        // This ensures we have enough space regardless of rotation angle
        let halfWidth = originalWidth / 2.0
        let halfHeight = originalHeight / 2.0
        let corners: [simd_float2] = [
            simd_float2(-halfWidth - pivotX, -halfHeight - pivotY),  // Top-left relative to pivot
            simd_float2(halfWidth - pivotX, -halfHeight - pivotY),  // Top-right
            simd_float2(halfWidth - pivotX, halfHeight - pivotY),  // Bottom-right
            simd_float2(-halfWidth - pivotX, halfHeight - pivotY),  // Bottom-left
        ]

        // Find maximum distance from pivot (0,0) to any corner
        var maxDistanceFromPivot: Float = 0.0
        for corner in corners {
            let distance = sqrt(corner.x * corner.x + corner.y * corner.y)
            maxDistanceFromPivot = max(maxDistanceFromPivot, distance)
        }

        // Square size needs to fit a circle with radius = maxDistanceFromPivot
        let squareSize = ceil(maxDistanceFromPivot * 2.0 + 4.0)  // Add small buffer for safety

        let bbWidth = squareSize
        let bbHeight = squareSize

        // For square approach: pivot is always at the center of the square
        // Since the square is sized to contain the maximum possible rotation
        let outputCenterX: Float = 0.5  // Center of square
        let outputCenterY: Float = 0.5  // Center of square

        // DEBUG: Log square quad calculations
        log(
            "🔥 SQUARE QUAD: rotation=\(rotation)°, original=(\(originalWidth)x\(originalHeight))",
            level: .info)
        log(
            "🔥 SQUARE QUAD: pivot=(\(pivotX), \(pivotY)), maxDistance=\(maxDistanceFromPivot)",
            level: .info)
        log(
            "🔥 SQUARE QUAD: squareSize=\(squareSize), output center (\(outputCenterX), \(outputCenterY))",
            level: .info)

        // Create params with separate centers
        let params = RotSpriteParams(
            rotationDegrees: rotation,
            originalWidth: UInt32(originalWidth),
            originalHeight: UInt32(originalHeight),
            inputCenterX: inputCenterX,
            inputCenterY: inputCenterY,
            outputCenterX: outputCenterX,
            outputCenterY: outputCenterY
        )

        log(
            "RotSprite: Processing \(texture.width)x\(texture.height), pivot (\(pivotX), \(pivotY)), rotation \(rotation)°",
            level: .debug)
        log(
            "RotSprite: Bounding box (\(bbWidth)x\(bbHeight)), output center (\(outputCenterX), \(outputCenterY))",
            level: .debug)

        // Create upscaled texture
        let upscaledSize = SIMD2<UInt32>(UInt32(originalWidth) * 8, UInt32(originalHeight) * 8)
        let upscaledDescriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: texture.pixelFormat,
            width: Int(upscaledSize.x),
            height: Int(upscaledSize.y),
            mipmapped: false
        )
        upscaledDescriptor.usage = [.shaderRead, .shaderWrite]

        guard let upscaledTexture = device.makeTexture(descriptor: upscaledDescriptor) else {
            log("processRotSprite: Failed to create upscaled texture", level: .error)
            return nil
        }

        // Output texture at DOWNSCALED bb size
        let outputDescriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: texture.pixelFormat,
            width: Int(bbWidth),
            height: Int(bbHeight),
            mipmapped: false
        )
        outputDescriptor.usage = [.shaderRead, .shaderWrite]

        guard let outputTexture = device.makeTexture(descriptor: outputDescriptor) else {
            log("processRotSprite: Failed to create output texture", level: .error)
            return nil
        }

        // Create command buffer
        guard let commandBuffer = commandQueue.makeCommandBuffer(),
            let computeEncoder = commandBuffer.makeComputeCommandEncoder()
        else {
            log(
                "processRotSprite: Failed to create command buffer or compute encoder",
                level: .error)
            return nil
        }

        // Create parameter buffer
        guard
            let paramBuffer = device.makeBuffer(
                bytes: [params], length: MemoryLayout<RotSpriteParams>.stride, options: [])
        else {
            log("processRotSprite: Failed to create parameter buffer", level: .error)
            return nil
        }

        // Phase 1: Scale2x upscaling
        computeEncoder.setComputePipelineState(upscalePipeline)
        computeEncoder.setTexture(texture, index: 0)  // Input
        computeEncoder.setTexture(upscaledTexture, index: 1)  // Output
        computeEncoder.setBuffer(paramBuffer, offset: 0, index: 0)

        let upscaleThreadsPerGroup = MTLSize(width: 8, height: 8, depth: 1)
        let upscaleThreadGroups = MTLSize(
            width: (Int(upscaledSize.x) + upscaleThreadsPerGroup.width - 1)
                / upscaleThreadsPerGroup.width,
            height: (Int(upscaledSize.y) + upscaleThreadsPerGroup.height - 1)
                / upscaleThreadsPerGroup.height,
            depth: 1
        )

        computeEncoder.dispatchThreadgroups(
            upscaleThreadGroups, threadsPerThreadgroup: upscaleThreadsPerGroup)

        // Phase 2: Rotate and downscale
        computeEncoder.setComputePipelineState(rotatePipeline)
        computeEncoder.setTexture(upscaledTexture, index: 0)  // Input
        computeEncoder.setTexture(outputTexture, index: 1)  // Output
        computeEncoder.setBuffer(paramBuffer, offset: 0, index: 0)

        let rotateThreadsPerGroup = MTLSize(width: 8, height: 8, depth: 1)
        let rotateThreadGroups = MTLSize(
            width: (Int(bbWidth) + rotateThreadsPerGroup.width - 1) / rotateThreadsPerGroup.width,
            height: (Int(bbHeight) + rotateThreadsPerGroup.height - 1)
                / rotateThreadsPerGroup.height,
            depth: 1
        )

        computeEncoder.dispatchThreadgroups(
            rotateThreadGroups, threadsPerThreadgroup: rotateThreadsPerGroup)

        // Commit and wait
        computeEncoder.endEncoding()
        commandBuffer.commit()
        commandBuffer.waitUntilCompleted()

        if commandBuffer.error != nil {
            log(
                "processRotSprite: RotSprite compute failed: \(commandBuffer.error!.localizedDescription)",
                level: .error)
            return nil
        }

        log(
            "processRotSprite: Successfully processed texture \(texture.width)x\(texture.height) with rotation \(rotation)°",
            level: .debug)
        return (texture: outputTexture, rotationCenter: SIMD2(outputCenterX, outputCenterY))
    }

    /// Render a RotSprite-processed texture using standard sprite rendering
    private func renderRotSpriteTexture(
        _ texture: MTLTexture, at position: (x: Float, y: Float), scale: (x: Float, y: Float),
        rotationCenter: SIMD2<Float>
    ) {
        // Note: Assuming 'position' is the world position of the PIVOT (base center). If it's sprite center, adjust accordingly.

        guard let uniformBuffer = uniformBuffer else {
            log("renderRotSpriteTexture: Missing required Metal resources", level: .warning)
            return
        }

        guard let pipelineState = texturedPipelineState else {
            log("renderRotSpriteTexture: No pipeline state available", level: .warning)
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            log("renderRotSpriteTexture: Failed to get render encoder", level: .error)
            return
        }

        let textureWidth = Float(texture.width)
        let textureHeight = Float(texture.height)
        let spriteWidth = textureWidth * scale.x  // Now correct bb size * scale
        let spriteHeight = textureHeight * scale.y

        // Adjust top-left position so pivot is at 'position'
        let adjustedPosX = position.x - rotationCenter.x * spriteWidth
        let adjustedPosY = position.y - rotationCenter.y * spriteHeight  // Assuming Y positive down

        // Use basic top-left transform (no rotation on quad)
        let modelMatrix = MetalMatrixHelpers.spriteTransformMatrix(
            position: (x: adjustedPosX, y: adjustedPosY),
            scale: (x: spriteWidth, y: spriteHeight),
            rotation: 0.0
        )

        // Get current projection matrix
        let projectionMatrix = uniformBuffer.contents().bindMemory(
            to: simd_float4x4.self, capacity: 1
        ).pointee

        // Create MVP matrix
        let mvpMatrix = projectionMatrix * modelMatrix

        // Create temporary uniform buffer for this sprite
        guard let device = device,
            let tempUniformBuffer = device.makeBuffer(
                bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: [])
        else {
            log("renderRotSpriteTexture: Failed to create temporary uniform buffer", level: .error)
            return
        }

        // Set up render encoder with standard vertex buffer
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(tempUniformBuffer, offset: 0, index: 1)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.setFragmentSamplerState(samplerState, index: 0)

        // Draw the sprite
        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!,
            indexBufferOffset: 0)

        log(
            "Rendered RotSprite: bb (\(textureWidth)x\(textureHeight)), adjusted pos (\(adjustedPosX),\(adjustedPosY)), pivot at (\(position.x),\(position.y))",
            level: .debug)
    }

    // MARK: - Pixel-Perfect Parallax Rendering Pipeline

    /// Snap UV coordinates to texel centers to eliminate sub-pixel sampling artifacts
    /// This ensures UV coordinates align exactly with pixel boundaries
    private func snapUVToTexelCenter(_ uv: Float, textureSize: Float) -> Float {
        return floor(uv * textureSize + 0.5) / textureSize
    }

    /// Snap position to pixel boundaries to eliminate sub-pixel positioning
    /// This ensures sprites are rendered at exact pixel locations
    private func snapToPixelBoundary(_ position: Float) -> Float {
        return floor(position + 0.5)
    }

    /// Create pixel-perfect vertex data with snapped UV coordinates
    /// Position snapping happens in the transformation matrix, not vertex data
    private func createPixelPerfectVertices(
        x: Float, y: Float, width: Float, height: Float,
        textureWidth: Float, textureHeight: Float,
        sourceX: Float = 0.0, sourceY: Float = 0.0,
        sourceWidth: Float? = nil, sourceHeight: Float? = nil
    ) -> [Float] {
        // Calculate UV coordinates for source rectangle or full texture
        let actualSourceWidth = sourceWidth ?? textureWidth
        let actualSourceHeight = sourceHeight ?? textureHeight

        // Calculate pixel-perfect UV coordinates
        let u0 = snapUVToTexelCenter(sourceX / textureWidth, textureSize: textureWidth)
        let v0 = snapUVToTexelCenter(sourceY / textureHeight, textureSize: textureHeight)
        let u1 = snapUVToTexelCenter(
            (sourceX + actualSourceWidth) / textureWidth, textureSize: textureWidth)
        let v1 = snapUVToTexelCenter(
            (sourceY + actualSourceHeight) / textureHeight, textureSize: textureHeight)

        // Create vertices with snapped positions and UV coordinates
        return [
            // Position (x, y), TexCoord (u, v), Color (r, g, b, a)
            0.0, 1.0, u0, v1, 1.0, 1.0, 1.0, 1.0,  // Bottom-left
            1.0, 1.0, u1, v1, 1.0, 1.0, 1.0, 1.0,  // Bottom-right
            1.0, 0.0, u1, v0, 1.0, 1.0, 1.0, 1.0,  // Top-right
            0.0, 0.0, u0, v0, 1.0, 1.0, 1.0, 1.0,  // Top-left
        ]
    }

    /// Specialized parallax sprite rendering with pixel-perfect positioning and UV snapping
    /// This eliminates sub-pixel flickering in scrolling backgrounds
    public func drawParallaxSprite(
        textureHandle: UInt32, x: Float, y: Float, scaleX: Float, scaleY: Float,
        rotation: Float = 0.0, sourceX: Float = 0.0, sourceY: Float = 0.0,
        sourceWidth: Float? = nil, sourceHeight: Float? = nil
    ) {
        guard let texture = textures[textureHandle] else {
            log("drawParallaxSprite: Invalid texture handle \(textureHandle)", level: .warning)
            return
        }

        guard let device = device,
            let uniformBuffer = uniformBuffer,
            let parallaxSamplerState = parallaxSamplerState,
            let pipelineState = texturedPipelineState
        else {
            log("drawParallaxSprite: Missing required Metal resources", level: .warning)
            return
        }

        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawParallaxSprite: Failed to get render encoder", level: .error)
            return
        }

        // Calculate snapped sprite dimensions
        let spriteWidth = floor(Float(texture.width) * scaleX + 0.5)
        let spriteHeight = floor(Float(texture.height) * scaleY + 0.5)

        // Use integer-only positioning for the transformation matrix
        let snapX = snapToPixelBoundary(x)
        let snapY = snapToPixelBoundary(y)

        // Create transformation matrix with snapped position
        let modelMatrix = MetalMatrixHelpers.spriteTransformMatrix(
            position: (x: snapX, y: snapY),
            scale: (x: spriteWidth, y: spriteHeight),
            rotation: rotation
        )

        // Get current projection matrix and create MVP
        let projectionMatrix = uniformBuffer.contents().bindMemory(
            to: simd_float4x4.self, capacity: 1
        ).pointee
        let mvpMatrix = projectionMatrix * modelMatrix

        // Create temporary uniform buffer
        guard
            let tempUniformBuffer = device.makeBuffer(
                bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: [])
        else {
            log("drawParallaxSprite: Failed to create temporary uniform buffer", level: .error)
            return
        }

        // Create pixel-perfect vertex buffer with UV snapping
        let textureWidth = Float(texture.width)
        let textureHeight = Float(texture.height)
        let vertices = createPixelPerfectVertices(
            x: snapX, y: snapY, width: spriteWidth, height: spriteHeight,
            textureWidth: textureWidth, textureHeight: textureHeight,
            sourceX: sourceX, sourceY: sourceY,
            sourceWidth: sourceWidth, sourceHeight: sourceHeight
        )

        guard
            let parallaxVertexBuffer = device.makeBuffer(
                bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: [])
        else {
            log("drawParallaxSprite: Failed to create parallax vertex buffer", level: .error)
            return
        }

        // Set up render encoder with specialized parallax sampler
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setVertexBuffer(parallaxVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(tempUniformBuffer, offset: 0, index: 1)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.setFragmentSamplerState(parallaxSamplerState, index: 0)  // Use parallax sampler

        // Draw the parallax sprite
        renderEncoder.drawIndexedPrimitives(
            type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!,
            indexBufferOffset: 0)

        log(
            "drawParallaxSprite: Rendered parallax sprite \(textureHandle) at snapped position (\(snapX), \(snapY)) size (\(spriteWidth), \(spriteHeight))",
            level: .debug)
    }

    /// Specialized integer-position sprite rendering for backgrounds
    /// Forces all positioning to exact pixel boundaries
    public func drawBackgroundSprite(
        textureHandle: UInt32, pixelX: Int, pixelY: Int, pixelWidth: Int, pixelHeight: Int
    ) {
        drawParallaxSprite(
            textureHandle: textureHandle,
            x: Float(pixelX), y: Float(pixelY),
            scaleX: Float(pixelWidth) / Float(textures[textureHandle]?.width ?? 1),
            scaleY: Float(pixelHeight) / Float(textures[textureHandle]?.height ?? 1),
            rotation: 0.0
        )
    }
}
