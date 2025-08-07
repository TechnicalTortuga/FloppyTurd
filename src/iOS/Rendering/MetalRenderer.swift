//
//  MetalRenderer.swift
//  FloppyTurd
//
//  Created by Gnosis Engine
//  Copyright © 2024 Floppy Turd Studios. All rights reserved.
//

import Foundation
import Metal
import MetalKit
import UIKit
import CoreGraphics
import CoreText
import simd
import GameCorePlatform

// MARK: - SDF Font Data Structures

struct GlyphInfo {
    let atlasX: Float          // X position in atlas (0-1)
    let atlasY: Float          // Y position in atlas (0-1) 
    let atlasWidth: Float      // Width in atlas (0-1)
    let atlasHeight: Float     // Height in atlas (0-1)
    let bearingX: Float        // Left bearing
    let bearingY: Float        // Top bearing
    let advance: Float         // Horizontal advance
    let width: Float           // Glyph width in pixels
    let height: Float          // Glyph height in pixels
}

struct FontMetrics {
    let size: Float           // Font size
    let lineHeight: Float     // Line spacing
    let ascender: Float       // Distance from baseline to top
    let descender: Float      // Distance from baseline to bottom
    let base: Float           // Baseline position from .fnt file
    let atlasWidth: Float     // Atlas texture width in pixels
    let atlasHeight: Float    // Atlas texture height in pixels
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

/**
 * @class MetalRenderer
 * @brief Swift implementation for iOS Metal rendering via threading system
 * 
 * This class is designed to be used exclusively by the CommandProcessor on the main thread.
 * All methods are @MainActor isolated for thread safety and simplified implementation.
 * 
 * Provides high-performance Metal-based 2D rendering with clean, direct method calls.
 */
@MainActor
public class MetalRenderer {
    
    // MARK: - Properties (private for encapsulation, @MainActor isolated for thread safety)
    private var device: MTLDevice?
    private var commandQueue: MTLCommandQueue?
    private var renderPipelineState: MTLRenderPipelineState?
    private var texturedPipelineState: MTLRenderPipelineState?

    
    // SDF Text pipeline states
    private var sdfTextPipelineState: MTLRenderPipelineState?
    private var sdfTextOutlinePipelineState: MTLRenderPipelineState?
    private var sdfTextShadowPipelineState: MTLRenderPipelineState?

    private var vertexBuffer: MTLBuffer?
    private var indexBuffer: MTLBuffer?
    private var uniformBuffer: MTLBuffer?
    private var samplerState: MTLSamplerState?
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
    private var currentRenderEncoder: MTLRenderCommandEncoder? // FIX: Track the single render encoder
    private var currentDrawable: CAMetalDrawable?
    private var viewportSize: CGSize = CGSize.zero
    private var clearColor: MTLClearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 1.0, alpha: 1.0)  // Blue background for debugging
    
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
    
    // SDF Font Atlas Management
    private var fontAtlas: MTLTexture?
    private var fontMetrics: FontMetrics?
    private var glyphMap: [Character: GlyphInfo] = [:]
    private var fontAtlasCache: [String: UIImage] = [:]

    
    // MARK: - Initialization (@MainActor ensures main thread execution)
    
    public init() {
        log("🔧 MetalRenderer init() called - setting up Metal on main thread", level: .info)
        setupMetal()
        log("🔧 MetalRenderer init() completed - device: \(device != nil), commandQueue: \(commandQueue != nil)", level: .info)
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
            log("Failed to load default font 'Whacky_Joe' - text rendering will use placeholders", level: .warning)
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
              let fragmentFunction = library.makeFunction(name: "fragment_main") else {
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
        vertexDescriptor.layouts[0].stride = 32 // 2 + 2 + 4 floats * 4 bytes
        vertexDescriptor.layouts[0].stepRate = 1
        vertexDescriptor.layouts[0].stepFunction = .perVertex
        
        // Create render pipeline descriptor
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.vertexFunction = vertexFunction
        pipelineDescriptor.fragmentFunction = fragmentFunction
        pipelineDescriptor.vertexDescriptor = vertexDescriptor
        pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
        
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
        guard let texturedFragmentFunction = library.makeFunction(name: "textured_fragment_main") else {
            log("Failed to create textured fragment function", level: .error)
            return
        }
        
        let texturedPipelineDescriptor = MTLRenderPipelineDescriptor()
        texturedPipelineDescriptor.vertexFunction = vertexFunction
        texturedPipelineDescriptor.fragmentFunction = texturedFragmentFunction
        texturedPipelineDescriptor.vertexDescriptor = vertexDescriptor
        texturedPipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm_srgb  // Match the texture format
        
        // Enable blending for textured rendering (required for sprites with transparency)
        texturedPipelineDescriptor.colorAttachments[0].isBlendingEnabled = true
        texturedPipelineDescriptor.colorAttachments[0].rgbBlendOperation = .add
        texturedPipelineDescriptor.colorAttachments[0].alphaBlendOperation = .add
        texturedPipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        texturedPipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = .sourceAlpha
        texturedPipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        texturedPipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
        
        // Also create a non-blending pipeline for testing

        
        do {
            texturedPipelineState = try device.makeRenderPipelineState(descriptor: texturedPipelineDescriptor)
            log("Textured pipeline state created successfully", level: .debug)
        } catch {
            log("Failed to create textured pipeline state: \(error)", level: .error)
        }
        
        // Create SDF text pipeline states
        setupSDFTextPipelines(device: device, library: library, vertexFunction: vertexFunction, vertexDescriptor: vertexDescriptor)
        
        // Create sampler state for texture sampling
        let samplerDescriptor = MTLSamplerDescriptor()
        // Use nearest filtering for pixel art textures (prevents blurring on blow-up)
        samplerDescriptor.minFilter = .nearest
        samplerDescriptor.magFilter = .nearest
        samplerDescriptor.mipFilter = .notMipmapped
        samplerDescriptor.sAddressMode = .clampToEdge
        samplerDescriptor.tAddressMode = .clampToEdge
        samplerState = device.makeSamplerState(descriptor: samplerDescriptor)
        
        log("Sampler state created successfully", level: .debug)
    }
    
    private func setupSDFTextPipelines(device: MTLDevice, library: MTLLibrary, vertexFunction: MTLFunction, vertexDescriptor: MTLVertexDescriptor) {
        // SDF Text Pipeline (basic)
        if let sdfTextFragment = library.makeFunction(name: "sdf_text_fragment") {
            let sdfDescriptor = MTLRenderPipelineDescriptor()
            sdfDescriptor.vertexFunction = vertexFunction
            sdfDescriptor.fragmentFunction = sdfTextFragment
            sdfDescriptor.vertexDescriptor = vertexDescriptor
            sdfDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
            
            // Enable blending for text transparency
            sdfDescriptor.colorAttachments[0].isBlendingEnabled = true
            sdfDescriptor.colorAttachments[0].rgbBlendOperation = .add
            sdfDescriptor.colorAttachments[0].alphaBlendOperation = .add
            sdfDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
            sdfDescriptor.colorAttachments[0].sourceAlphaBlendFactor = .sourceAlpha
            sdfDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
            sdfDescriptor.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
            
            do {
                sdfTextPipelineState = try device.makeRenderPipelineState(descriptor: sdfDescriptor)
                log("SDF text pipeline state created successfully", level: .debug)
            } catch {
                log("Failed to create SDF text pipeline state: \(error)", level: .error)
            }
        }
        
        // SDF Text with Outline Pipeline
        if let sdfOutlineFragment = library.makeFunction(name: "sdf_text_outline_fragment") {
            let sdfOutlineDescriptor = MTLRenderPipelineDescriptor()
            sdfOutlineDescriptor.vertexFunction = vertexFunction
            sdfOutlineDescriptor.fragmentFunction = sdfOutlineFragment
            sdfOutlineDescriptor.vertexDescriptor = vertexDescriptor
            sdfOutlineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
            
            // Enable blending for text transparency
            sdfOutlineDescriptor.colorAttachments[0].isBlendingEnabled = true
            sdfOutlineDescriptor.colorAttachments[0].rgbBlendOperation = .add
            sdfOutlineDescriptor.colorAttachments[0].alphaBlendOperation = .add
            sdfOutlineDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
            sdfOutlineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = .sourceAlpha
            sdfOutlineDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
            sdfOutlineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
            
            do {
                sdfTextOutlinePipelineState = try device.makeRenderPipelineState(descriptor: sdfOutlineDescriptor)
                log("SDF text outline pipeline state created successfully", level: .debug)
            } catch {
                log("Failed to create SDF text outline pipeline state: \(error)", level: .error)
            }
        }
        
        // SDF Text with Shadow Pipeline
        if let sdfShadowFragment = library.makeFunction(name: "sdf_text_shadow_fragment") {
            let sdfShadowDescriptor = MTLRenderPipelineDescriptor()
            sdfShadowDescriptor.vertexFunction = vertexFunction
            sdfShadowDescriptor.fragmentFunction = sdfShadowFragment
            sdfShadowDescriptor.vertexDescriptor = vertexDescriptor
            sdfShadowDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
            
            // Enable blending for text transparency
            sdfShadowDescriptor.colorAttachments[0].isBlendingEnabled = true
            sdfShadowDescriptor.colorAttachments[0].rgbBlendOperation = .add
            sdfShadowDescriptor.colorAttachments[0].alphaBlendOperation = .add
            sdfShadowDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
            sdfShadowDescriptor.colorAttachments[0].sourceAlphaBlendFactor = .sourceAlpha
            sdfShadowDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
            sdfShadowDescriptor.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
            
            do {
                sdfTextShadowPipelineState = try device.makeRenderPipelineState(descriptor: sdfShadowDescriptor)
                log("SDF text shadow pipeline state created successfully", level: .debug)
            } catch {
                log("Failed to create SDF text shadow pipeline state: \(error)", level: .error)
            }
        }
        

    }
    
    private func setupBuffers() {
        guard let device = device else {
            log("Cannot setup buffers: device is nil", level: .error)
            return
        }
        
        // Create vertex buffer for a quad with position, texCoord, and color
        // Format: [x, y, u, v, r, g, b, a] per vertex
        let vertices: [Float] = [
            // Bottom-left
            0.0, 1.0,  0.0, 1.0,  1.0, 1.0, 1.0, 1.0,
            // Bottom-right
            1.0, 1.0,  1.0, 1.0,  1.0, 1.0, 1.0, 1.0,
            // Top-right
            1.0, 0.0,  1.0, 0.0,  1.0, 1.0, 1.0, 1.0,
            // Top-left
            0.0, 0.0,  0.0, 0.0,  1.0, 1.0, 1.0, 1.0
        ]
        
        vertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: [])
        vertexBuffer?.label = "Quad Vertices"
        
        // Create index buffer
        let indices: [UInt16] = [0, 1, 2, 2, 3, 0]
        indexBuffer = device.makeBuffer(bytes: indices, length: indices.count * MemoryLayout<UInt16>.stride, options: [])
        indexBuffer?.label = "Quad Indices"
        
        log("Vertex and index buffers created successfully", level: .debug)
    }
    
    // MARK: - Public Interface (Threading System Integration)
    
    public func initialize() -> Bool {
        let isInitialized = device != nil && commandQueue != nil
        log("initialize() check - device: \(device != nil), commandQueue: \(commandQueue != nil), result: \(isInitialized)", level: .debug)
        return isInitialized
    }
    
    public func shutdown() {
        textures.removeAll()
        vertexBuffer = nil
        indexBuffer = nil
        uniformBuffer = nil
        samplerState = nil
        renderPipelineState = nil
        texturedPipelineState = nil
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
        view.colorPixelFormat = .bgra8Unorm  // Use non-sRGB to avoid double conversion
        view.clearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 1.0, alpha: 1.0)  // Blue background for debugging
        view.framebufferOnly = false
        view.enableSetNeedsDisplay = false
        view.isPaused = false
        view.preferredFramesPerSecond = 60
        
        viewportSize = view.drawableSize
        
        // Create uniform buffer for MVP matrix
        if let device = device {
            uniformBuffer = device.makeBuffer(length: MemoryLayout<simd_float4x4>.stride, options: [])
            uniformBuffer?.label = "Uniforms"
            
            // Set up orthographic projection matrix
            updateProjectionMatrix()
        }
        
        log("MetalRenderer connected to MTKView with size \(viewportSize) (width: \(viewportSize.width), height: \(viewportSize.height))", level: .debug)
    }
    
    private func updateProjectionMatrix() {
        guard let uniformBuffer = uniformBuffer else { return }
        
        let width = Float(viewportSize.width)
        let height = Float(viewportSize.height)
        
        // Use helper function to create projection matrix
        let projectionMatrix = MetalMatrixHelpers.viewportProjectionMatrix(width: width, height: height)
        
        let contents = uniformBuffer.contents().bindMemory(to: simd_float4x4.self, capacity: 1)
        contents.pointee = projectionMatrix
        
        // NOTE: didModifyRange not needed on iOS - cache coherency is automatic
        
        print("🔧 Updated projection matrix for viewport \(width)x\(height)")
        log("Updated projection matrix for viewport \(width)x\(height)", level: .debug)
    }
    
    public func beginFrame() {
        guard let commandQueue = commandQueue else { return }
        
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
    }
    
    public func endFrame() {
        // FIX: End the render encoder if it exists
        currentRenderEncoder?.endEncoding()
        currentRenderEncoder = nil
        log("endFrame() called - render encoder ended", level: .debug)
    }

    public func present() {
        // Proper commit/present order: present THEN commit
        if let drawable = currentDrawable {
            currentCommandBuffer?.present(drawable)
        }
        currentCommandBuffer?.commit()
        currentCommandBuffer = nil
        currentDrawable = nil
        currentRenderPassDescriptor = nil
        log("Frame presented successfully", level: .debug)
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
        clearColor = MTLClearColor(red: Double(r), green: Double(g), blue: Double(b), alpha: Double(a))
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
        
        log("Screen clear configured with color (\(clearColor.red), \(clearColor.green), \(clearColor.blue), \(clearColor.alpha))", level: .debug)
    }

    public func clear() {
        clearScreen()
    }
    
    // FIX: Helper method to ensure render encoder is created only once per frame
    private func ensureRenderEncoder() -> MTLRenderCommandEncoder? {
        if currentRenderEncoder == nil {
            guard let commandBuffer = currentCommandBuffer,
                  let renderPassDescriptor = currentRenderPassDescriptor else {
                log("ensureRenderEncoder: Missing command buffer or render pass descriptor", level: .warning)
                return nil
            }
            
            currentRenderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
            currentRenderEncoder?.label = "FloppyTurd Render Pass"
            
            // NEW: fully define raster-state each frame so we match Apple docs
            if let enc = currentRenderEncoder {
                // 1) viewport covers the whole drawable
                let vp = MTLViewport(originX: 0,
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
    
    public func drawRectangle(x: Float, y: Float, width: Float, height: Float, 
                             r: Float, g: Float, b: Float, a: Float) {
        guard let device = device,
              let renderPipelineState = renderPipelineState,
              let indexBuffer = indexBuffer,
              let uniformBuffer = uniformBuffer else {
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
            x, y + height,  0.0, 1.0,  r, g, b, a,
            // Bottom-right
            x + width, y + height,  1.0, 1.0,  r, g, b, a,
            // Top-right
            x + width, y,  1.0, 0.0,  r, g, b, a,
            // Top-left
            x, y,  0.0, 0.0,  r, g, b, a
        ]
        
        // Debug logging for rectangle drawing
        log("🎯 Drawing rectangle at (\(x), \(y)) size (\(width), \(height)) color (\(r), \(g), \(b), \(a))", level: .debug)
        log("🎯 Vertices: [\(vertices[0]),\(vertices[1]) \(vertices[8]),\(vertices[9]) \(vertices[16]),\(vertices[17]) \(vertices[24]),\(vertices[25])]", level: .debug)
        
        // Create temporary vertex buffer for this rectangle
        guard let tempVertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []) else {
            log("drawRectangle: Failed to create vertex buffer", level: .error)
            return
        }
        
        // FIX: Use the shared render encoder, don't create a new one or end it
        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)
        
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer, indexBufferOffset: 0)
        
        // Enhanced runtime logging for debug
        log("[DEBUG] drawRectangle: x=\(x), y=\(y), width=\(width), height=\(height), color=(\(r),\(g),\(b),\(a)), viewport=(\(viewportSize.width),\(viewportSize.height))", level: .info)
        
        log("Rectangle drawn at (\(x), \(y)) with size (\(width), \(height)) and color (\(r), \(g), \(b), \(a)) - Viewport: \(viewportSize.width)x\(viewportSize.height)", level: .debug)
    }
    
    public func drawTexture(textureHandle: UInt32, x: Float, y: Float, width: Float, height: Float) {
        guard let texture = textures[textureHandle] else {
            log("drawTexture: Invalid texture handle \(textureHandle)", level: .warning)
            return
        }
        
        guard let texturedPipelineState = texturedPipelineState,
              let uniformBuffer = uniformBuffer else {
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
        let projectionMatrix = uniformBuffer.contents().bindMemory(to: simd_float4x4.self, capacity: 1).pointee
        
        // Create MVP matrix
        let mvpMatrix = projectionMatrix * modelMatrix
        
        // Create temporary uniform buffer for this texture
        guard let device = device,
              let tempUniformBuffer = device.makeBuffer(bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: []) else {
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
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!, indexBufferOffset: 0)
        
        log("drawTexture: Rendered texture \(textureHandle) at (\(x), \(y)) with size (\(width), \(height))", level: .debug)
    }
    
    public func drawText(text: String, x: Float, y: Float, fontSize: Float, 
                        r: Float, g: Float, b: Float, a: Float) {
        // Call the real SDF text rendering function
        drawTextSDF(text, x: x, y: y, fontSize: fontSize, r: r, g: g, b: b, a: a)
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
            log("Texture reused: handle \(existingHandle) (refs: \(handleReferenceCount[existingHandle]!))", level: .debug)
            return existingHandle
        }
        
        // Create new handle only for genuinely new textures
        let handle = nextTextureHandle
        textures[handle] = texture
        textureToHandle[textureId] = handle
        handleReferenceCount[handle] = 1
        newHandleCount += 1
        nextTextureHandle += 1
        
        log("Texture registered: handle \(handle) (\(texture.width)x\(texture.height))", level: .debug)
        log("Texture registration: pixel format \(texture.pixelFormat), usage \(texture.usage)", level: .debug)
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
    
    public func drawSpriteScaled(textureHandle: UInt32, x: Float, y: Float, scaleX: Float, scaleY: Float, rotation: Float) {
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
        log("🖼️ Drawing sprite \(textureHandle): texture \(texture.width)x\(texture.height), screen \(spriteWidth)x\(spriteHeight), pos (\(x),\(y)), rot \(rotation)°", level: .debug)
        log("drawSpriteScaled: Texture pixel format: \(texture.pixelFormat)", level: .debug)
        log("drawSpriteScaled: UV coordinates: (0,0) to (1,1) - full texture coverage", level: .debug)
        
        // DEBUG: Dump actual pixel data for 16x16 texture
        if texture.width == 16 && texture.height == 16 {
            dumpTexturePixelData(texture: texture, textureHandle: textureHandle)
            
            // DEBUG: Draw a magenta rectangle at the sprite position to verify positioning
            let debugDraw = false // Set to true to enable debug rectangle
            if debugDraw {
                drawRectangle(x: x - spriteWidth/2, y: y - spriteHeight/2, width: spriteWidth, height: spriteHeight, 
                            r: 1.0, g: 0.0, b: 1.0, a: 0.5)
                log("DEBUG: Drew debug rectangle at texture position", level: .debug)
                return // Skip texture drawing to see just the rectangle
            }
        }
        
        // Use helper function to create sprite transformation matrix (top-left positioning)
        let modelMatrix = MetalMatrixHelpers.spriteTransformMatrix(
            position: (x: x, y: y),
            scale: (x: spriteWidth, y: spriteHeight),
            rotation: rotation
        )
        
        log("drawSpriteScaled: Model matrix created for position (\(x), \(y)) scale (\(spriteWidth), \(spriteHeight)) rotation \(rotation)°", level: .debug)
        
        // Get current projection matrix
        let projectionMatrix = uniformBuffer.contents().bindMemory(to: simd_float4x4.self, capacity: 1).pointee
        
        // Create MVP matrix
        let mvpMatrix = projectionMatrix * modelMatrix
        
        // DEBUG: Log the MVP matrix to see if coordinate system is correct
        log("DEBUG: MVP Matrix for sprite at (\(x), \(y)):", level: .debug)
        log("DEBUG: Projection Matrix: \(projectionMatrix)", level: .debug)
        log("DEBUG: Model Matrix: \(modelMatrix)", level: .debug)
        log("DEBUG: Final MVP Matrix: \(mvpMatrix)", level: .debug)
        
        // Create temporary uniform buffer for this sprite
        guard let device = device,
              let tempUniformBuffer = device.makeBuffer(bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: []) else {
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
        log("DEBUG: Texture bound to fragment shader - handle: \(textureHandle), texture: \(texture), sampler: \(samplerState?.label ?? "nil")", level: .debug)
        
        // DEBUG: Check buffers before drawing
        log("DEBUG: Before drawIndexedPrimitives - indexBuffer: \(indexBuffer != nil ? "valid" : "nil"), vertexBuffer: \(vertexBuffer != nil ? "valid" : "nil")", level: .debug)
        
        // Draw the sprite
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!, indexBufferOffset: 0)
        
        log("DEBUG: After drawIndexedPrimitives - call completed", level: .debug)
        
        log("drawSpriteScaled: Rendered sprite \(textureHandle) at (\(x), \(y)) with scale (\(scaleX), \(scaleY)) rotation \(rotation)°", level: .debug)
    }
    
    public func drawSpriteScaledWithSource(textureHandle: UInt32, x: Float, y: Float, scaleX: Float, scaleY: Float, rotation: Float, sourceX: Float, sourceY: Float, sourceWidth: Float, sourceHeight: Float) {
        guard let texture = textures[textureHandle] else {
            log("drawSpriteScaledWithSource: Invalid sprite handle \(textureHandle)", level: .warning)
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
        
        // Calculate UV coordinates for the source rectangle
        let textureWidth = Float(texture.width)
        let textureHeight = Float(texture.height)
        let u0 = sourceX / textureWidth
        let v0 = sourceY / textureHeight
        let u1 = (sourceX + sourceWidth) / textureWidth
        let v1 = (sourceY + sourceHeight) / textureHeight
        
        log("🖼️ Drawing sprite with source rect: texture \(textureHandle), source (\(sourceX),\(sourceY),\(sourceWidth)x\(sourceHeight)), UV (\(u0),\(v0)) to (\(u1),\(v1)), screen \(spriteWidth)x\(spriteHeight), pos (\(x),\(y))", level: .debug)
        
        // Use sprite transformation matrix for centered positioning like other sprites
        let modelMatrix = MetalMatrixHelpers.spriteTransformMatrix(
            position: (x: x, y: y),
            scale: (x: spriteWidth, y: spriteHeight),
            rotation: 0.0
        )
        
        // Get current projection matrix
        let projectionMatrix = uniformBuffer.contents().bindMemory(to: simd_float4x4.self, capacity: 1).pointee
        
        // Create MVP matrix
        let mvpMatrix = projectionMatrix * modelMatrix
        
        // Create temporary uniform buffer for this sprite
        guard let device = device,
              let tempUniformBuffer = device.makeBuffer(bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: []) else {
            log("drawSpriteScaledWithSource: Failed to create temporary uniform buffer", level: .error)
            return
        }
        
        // Create vertex buffer with custom UV coordinates for the source rectangle
        // Use same vertex coordinates as static sprites to ensure consistent positioning
        let vertices: [Float] = [
            // Position (x, y), TexCoord (u, v), Color (r, g, b, a)
            0.0, 1.0, u0, v1, 1.0, 1.0, 1.0, 1.0,   // Bottom-left
            1.0, 1.0, u1, v1, 1.0, 1.0, 1.0, 1.0,   // Bottom-right
            1.0, 0.0, u1, v0, 1.0, 1.0, 1.0, 1.0,   // Top-right
            0.0, 0.0, u0, v0, 1.0, 1.0, 1.0, 1.0    // Top-left
        ]
        
        guard let sourceVertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []) else {
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
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!, indexBufferOffset: 0)
        
        log("drawSpriteScaledWithSource: Rendered sprite \(textureHandle) with source rect at (\(x), \(y))", level: .debug)
    }
    
    /// Draw a sprite with centered positioning (for rotation and special effects)
    public func drawSpriteScaledCentered(textureHandle: UInt32, x: Float, y: Float, scaleX: Float, scaleY: Float, rotation: Float) {
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
        let projectionMatrix = uniformBuffer.contents().bindMemory(to: simd_float4x4.self, capacity: 1).pointee
        
        // Create MVP matrix
        let mvpMatrix = projectionMatrix * modelMatrix
        
        // Create temporary uniform buffer for this sprite
        guard let device = device,
              let tempUniformBuffer = device.makeBuffer(bytes: [mvpMatrix], length: MemoryLayout<simd_float4x4>.stride, options: []) else {
            log("drawSpriteScaledCentered: Failed to create temporary uniform buffer", level: .error)
            return
        }
        
        // Set up render encoder
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(tempUniformBuffer, offset: 0, index: 1)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.setFragmentSamplerState(samplerState, index: 0)
        
        // Draw the sprite
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer!, indexBufferOffset: 0)
        
        log("drawSpriteScaledCentered: Rendered sprite \(textureHandle) centered at (\(x), \(y))", level: .debug)
    }
    
    public func drawCircle(_ x: Float, _ y: Float, _ radius: Float, _ r: Float, _ g: Float, _ b: Float, _ a: Float) {
        drawCircle(x: x, y: y, radius: radius, r: r, g: g, b: b, a: a, segments: 32)
    }
    
    // DEBUG: Function to dump texture pixel data
    private func dumpTexturePixelData(texture: MTLTexture, textureHandle: UInt32) {
        guard let device = device else { return }
        
        log("DEBUG: Dumping texture \(textureHandle) - size: \(texture.width)x\(texture.height), format: \(texture.pixelFormat), storage: \(texture.storageMode)", level: .debug)
        
        // If texture is in shared storage mode, we can read it directly
        if texture.storageMode == .shared {
            let bytesPerRow = texture.width * 4 // RGBA = 4 bytes per pixel
            let region = MTLRegion(origin: MTLOrigin(x: 0, y: 0, z: 0), 
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
        let bytesPerRow = texture.width * 4 // RGBA = 4 bytes per pixel
        let bufferSize = texture.height * bytesPerRow
        
        guard let buffer = device.makeBuffer(length: bufferSize, options: .storageModeShared) else {
            log("DEBUG: Failed to create buffer for texture data dump", level: .error)
            return
        }
        
        // Create a command buffer to copy texture to buffer
        guard let commandBuffer = commandQueue?.makeCommandBuffer(),
              let blitEncoder = commandBuffer.makeBlitCommandEncoder() else {
            log("DEBUG: Failed to create command buffer for texture data dump", level: .error)
            return
        }
        
        blitEncoder.copy(from: texture, sourceSlice: 0, sourceLevel: 0, sourceOrigin: MTLOrigin(x: 0, y: 0, z: 0), sourceSize: MTLSize(width: texture.width, height: texture.height, depth: 1), to: buffer, destinationOffset: 0, destinationBytesPerRow: bytesPerRow, destinationBytesPerImage: bufferSize, options: [])
        blitEncoder.endEncoding()
        
        commandBuffer.commit()
        commandBuffer.waitUntilCompleted()
        
        // Read the pixel data
        let pixelData = buffer.contents().bindMemory(to: UInt8.self, capacity: bufferSize)
        analyzeDumpedPixelData(pixels: pixelData, texture: texture, textureHandle: textureHandle)
    }
    
    private func analyzeDumpedPixelData(pixels: UnsafePointer<UInt8>, texture: MTLTexture, textureHandle: UInt32) {
        // bytesPerRow not needed in this function anymore
        
        log("DEBUG: Analyzing texture \(textureHandle) pixel data (\(texture.width)x\(texture.height) RGBA):", level: .debug)
        
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
                            log("DEBUG: Colored pixel \(coloredPixels) at (\(x),\(y)): RGBA(\(r),\(g),\(b),\(a))", level: .debug)
                        }
                    }
                }
            }
        }
        
        log("DEBUG: First 16 pixels (4x4 corner): \(firstFewPixels)", level: .debug)
        log("DEBUG: Texture has non-zero data: \(hasNonZeroData)", level: .debug)
        log("DEBUG: Non-transparent pixels: \(nonTransparentPixels)/\(totalPixels) (\(Int(Float(nonTransparentPixels) / Float(totalPixels) * 100))%)", level: .debug)
        log("DEBUG: Colored pixels (non-black): \(coloredPixels)/\(nonTransparentPixels) (\(Int(Float(coloredPixels) / Float(max(1, nonTransparentPixels)) * 100))%)", level: .debug)
        
        if coloredPixels == 0 && nonTransparentPixels > 0 {
            log("DEBUG: WARNING - All non-transparent pixels are black! Texture may not be loading colors correctly.", level: .error)
        } else if coloredPixels > 0 {
            log("DEBUG: Found \(coloredPixels) colored pixels - texture appears to have color data", level: .debug)
        }
    }
    
    public func drawCircle(x: Float, y: Float, radius: Float, r: Float, g: Float, b: Float, a: Float, segments: Int = 32) {
        guard let device = device,
              let renderPipelineState = renderPipelineState,
              let uniformBuffer = uniformBuffer else {
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
        vertices.append(contentsOf: [x, y, 0.5, 0.5, r, g, b, a]) // [x, y, u, v, r, g, b, a]
        
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
        guard let tempVertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []),
              let tempIndexBuffer = device.makeBuffer(bytes: indices, length: indices.count * MemoryLayout<UInt16>.stride, options: []) else {
            log("drawCircle: Failed to create temporary buffers", level: .error)
            return
        }
        
        // Configure render encoder
        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)
        
        // Draw
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: indices.count, indexType: .uint16, indexBuffer: tempIndexBuffer, indexBufferOffset: 0)
        
        log("Circle drawn at (\(x), \(y)) with radius \(radius) using \(segments) segments", level: .debug)
    }
    
    public func drawTriangle(x1: Float, y1: Float, x2: Float, y2: Float, x3: Float, y3: Float, r: Float, g: Float, b: Float, a: Float) {
        guard let device = device,
              let renderPipelineState = renderPipelineState,
              let uniformBuffer = uniformBuffer else {
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
            x3, y3, 0.5, 1.0, r, g, b, a
        ]
        
        let indices: [UInt16] = [0, 1, 2]
        
        // Create temporary buffers
        guard let tempVertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []),
              let tempIndexBuffer = device.makeBuffer(bytes: indices, length: indices.count * MemoryLayout<UInt16>.stride, options: []) else {
            log("drawTriangle: Failed to create temporary buffers", level: .error)
            return
        }
        
        // Configure render encoder
        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)
        
        // Draw
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: indices.count, indexType: .uint16, indexBuffer: tempIndexBuffer, indexBufferOffset: 0)
        
        log("Triangle drawn with vertices [(\(x1),\(y1)), (\(x2),\(y2)), (\(x3),\(y3))]", level: .debug)
    }
    
    public func drawLine(x1: Float, y1: Float, x2: Float, y2: Float, width: Float, r: Float, g: Float, b: Float, a: Float) {
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
            x1 + perpX, y1 + perpY, 0.0, 1.0, r, g, b, a
        ]
        
        guard let device = device,
              let renderPipelineState = renderPipelineState,
              let uniformBuffer = uniformBuffer else {
            log("drawLine: Missing required Metal resources", level: .warning)
            return
        }
        
        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawLine: Failed to get render encoder", level: .error)
            return
        }
        
        let indices: [UInt16] = [0, 1, 2, 2, 3, 0]
        
        // Create temporary buffers
        guard let tempVertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []),
              let tempIndexBuffer = device.makeBuffer(bytes: indices, length: indices.count * MemoryLayout<UInt16>.stride, options: []) else {
            log("drawLine: Failed to create temporary buffers", level: .error)
            return
        }
        
        // Configure render encoder
        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)
        
        // Draw
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: indices.count, indexType: .uint16, indexBuffer: tempIndexBuffer, indexBufferOffset: 0)
        
        log("Line drawn from (\(x1),\(y1)) to (\(x2),\(y2)) with width \(width)", level: .debug)
    }
    
    public func drawRoundedRectangle(x: Float, y: Float, width: Float, height: Float, cornerRadius: Float, r: Float, g: Float, b: Float, a: Float) {
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
        drawRectangle(x: x + clampedRadius, y: y + clampedRadius, 
                     width: width - 2 * clampedRadius, height: height - 2 * clampedRadius, 
                     r: r, g: g, b: b, a: a)
        
        // Top rectangle
        drawRectangle(x: x + clampedRadius, y: y, 
                     width: width - 2 * clampedRadius, height: clampedRadius, 
                     r: r, g: g, b: b, a: a)
        
        // Bottom rectangle  
        drawRectangle(x: x + clampedRadius, y: y + height - clampedRadius,
                     width: width - 2 * clampedRadius, height: clampedRadius,
                     r: r, g: g, b: b, a: a)
        
        // Left rectangle
        drawRectangle(x: x, y: y + clampedRadius,
                     width: clampedRadius, height: height - 2 * clampedRadius,
                     r: r, g: g, b: b, a: a)
        
        // Right rectangle
        drawRectangle(x: x + width - clampedRadius, y: y + clampedRadius,
                     width: clampedRadius, height: height - 2 * clampedRadius,
                     r: r, g: g, b: b, a: a)
        
        // Draw corner circles
        let segments = 8 // Quarter circle segments for performance
        
        // Top-left corner
        drawQuarterCircle(centerX: x + clampedRadius, centerY: y + clampedRadius, 
                         radius: clampedRadius, startAngle: Float.pi, 
                         r: r, g: g, b: b, a: a, segments: segments)
        
        // Top-right corner
        drawQuarterCircle(centerX: x + width - clampedRadius, centerY: y + clampedRadius,
                         radius: clampedRadius, startAngle: Float.pi * 1.5,
                         r: r, g: g, b: b, a: a, segments: segments)
        
        // Bottom-right corner
        drawQuarterCircle(centerX: x + width - clampedRadius, centerY: y + height - clampedRadius,
                         radius: clampedRadius, startAngle: 0,
                         r: r, g: g, b: b, a: a, segments: segments)
        
        // Bottom-left corner
        drawQuarterCircle(centerX: x + clampedRadius, centerY: y + height - clampedRadius,
                         radius: clampedRadius, startAngle: Float.pi * 0.5,
                         r: r, g: g, b: b, a: a, segments: segments)
        
        log("Rounded rectangle drawn at (\(x), \(y)) size (\(width), \(height)) with corner radius \(clampedRadius)", level: .debug)
    }
    
    private func drawQuarterCircle(centerX: Float, centerY: Float, radius: Float, startAngle: Float, 
                                  r: Float, g: Float, b: Float, a: Float, segments: Int) {
        guard let device = device,
              let renderPipelineState = renderPipelineState,
              let uniformBuffer = uniformBuffer else {
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
        
        guard let tempVertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []),
              let tempIndexBuffer = device.makeBuffer(bytes: indices, length: indices.count * MemoryLayout<UInt16>.stride, options: []) else {
            return
        }
        
        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)
        
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: indices.count, indexType: .uint16, indexBuffer: tempIndexBuffer, indexBufferOffset: 0)
    }
    
    // MARK: - SDF Text Rendering
    
    public func loadFont(fontName: String, fontSize: Float) -> Bool {
        log("Loading font: \(fontName) with size: \(fontSize)", level: .debug)
        
        // Check if we already have the font loaded
        if fontAtlas != nil && !glyphMap.isEmpty {
            log("Font already loaded: \(fontName)", level: .debug)
            return true
        }
        
        // First, load the TTF font to get glyph metrics
        guard let fontURL = Bundle.main.url(forResource: fontName, withExtension: "ttf", subdirectory: "fonts") ?? Bundle.main.url(forResource: fontName, withExtension: "ttf") else {
            log("Failed to find TTF font: \(fontName).ttf", level: .error)
            return false
        }
        
        // Load font data and create Core Text font
        guard let fontData = try? Data(contentsOf: fontURL),
              let dataProvider = CGDataProvider(data: fontData as CFData),
              let cgFont = CGFont(dataProvider) else {
            log("Failed to create CGFont from: \(fontURL.path)", level: .error)
            return false
        }
        
        // Create CTFont for glyph metrics
        let ctFont = CTFontCreateWithGraphicsFont(cgFont, CGFloat(fontSize), nil, nil)
        
        // Try to load pre-generated atlas
        var atlasImage: UIImage? = nil
        var isSDFAtlas = false
        
        // Try different naming conventions for the atlas
        let atlasNames = [
            "WhackyJoe_32",  // Specific atlas name
            "Whacky_Joe_32", // Alternative spelling
            fontName,        // Exact font name
            "\(fontName)_32", // Font name with size
            "\(fontName)_\(Int(fontSize))" // Font name with requested size
        ]
        
        for name in atlasNames {
            // Try asset catalog first
            atlasImage = UIImage(named: name)
            if atlasImage != nil {
                log("Found pre-generated atlas: \(name) in asset catalog", level: .debug)
                // Check if this is an SDF atlas by looking for SDF-specific naming
                isSDFAtlas = name.contains("SDF") || name.contains("sdf")
                break
            }
            
            // Try bundle with fonts subdirectory
            if let url = Bundle.main.url(forResource: name, withExtension: "png", subdirectory: "fonts") {
                atlasImage = UIImage(contentsOfFile: url.path)
                if atlasImage != nil {
                    log("Found pre-generated atlas: \(name).png in fonts subdirectory", level: .debug)
                    // Check if this is an SDF atlas by looking for SDF-specific naming
                    isSDFAtlas = name.contains("SDF") || name.contains("sdf")
                    break
                }
            }
        }
        
        if let atlasImage = atlasImage, isSDFAtlas {
            // Use pre-generated SDF atlas
            guard let device = device else {
                log("Device not available for font loading", level: .error)
                return false
            }
            
            let textureLoader = MTKTextureLoader(device: device)
            do {
                fontAtlas = try textureLoader.newTexture(cgImage: atlasImage.cgImage!, options: [
                    .textureUsage: MTLTextureUsage.shaderRead.rawValue,
                    .textureStorageMode: MTLStorageMode.shared.rawValue
                ])
                
                // Generate glyph metrics from TTF font
                if generateGlyphMetricsFromTTF(ctFont: ctFont, atlasWidth: Float(atlasImage.size.width), atlasHeight: Float(atlasImage.size.height)) {
                    fontMetrics = FontMetrics(
                        size: fontSize,
                        lineHeight: fontSize * 1.2,
                        ascender: fontSize * 0.8,
                        descender: fontSize * 0.2,
                        base: fontSize * 0.8,
                        atlasWidth: Float(atlasImage.size.width),
                        atlasHeight: Float(atlasImage.size.height)
                    )
                    
                    log("Font loaded successfully with pre-generated SDF atlas for: \(fontName)", level: .debug)
                    return true
                } else {
                    log("Failed to generate glyph metrics for: \(fontName)", level: .error)
                    return false
                }
                
            } catch {
                log("Failed to create texture from pre-generated atlas: \(error)", level: .error)
            }
        } else if atlasImage != nil {
            log("Found pre-generated atlas but it's not an SDF atlas, falling back to TTF generation", level: .debug)
        }
        
        // Fallback: Generate SDF atlas from TTF
        log("Pre-generated atlas not found, generating SDF from TTF font", level: .debug)
        
        // Generate SDF atlas
        guard let sdfAtlas = generateSDFAtlas(from: ctFont, atlasSize: 512, padding: 8) else {
            log("Failed to generate SDF atlas", level: .error)
            return false
        }
        
        // Cache the generated atlas
        let cacheKey = "\(fontName)_\(Int(fontSize))"
        fontAtlasCache[cacheKey] = sdfAtlas
        
        // Create Metal texture
        guard let device = device else {
            log("Device not available for font loading", level: .error)
            return false
        }
        
        let textureLoader = MTKTextureLoader(device: device)
        do {
            // Force single-channel format for SDF texture
            fontAtlas = try textureLoader.newTexture(cgImage: sdfAtlas.cgImage!, options: [
                .textureUsage: MTLTextureUsage.shaderRead.rawValue,
                .textureStorageMode: MTLStorageMode.shared.rawValue,
                .SRGB: false,
                .generateMipmaps: false
            ])
            
            fontMetrics = FontMetrics(
                size: fontSize,
                lineHeight: fontSize * 1.2,
                ascender: fontSize * 0.8,
                descender: fontSize * 0.2,
                base: fontSize * 0.8,
                atlasWidth: Float(sdfAtlas.size.width),
                atlasHeight: Float(sdfAtlas.size.height)
            )
            
            log("Font loaded successfully with generated SDF atlas for: \(fontName)", level: .debug)
            return true
            
        } catch {
            log("Failed to create texture from font atlas: \(error)", level: .error)
            return false
        }
    }
    
    private func loadTTFFont(fontName: String, fontSize: Float) -> Bool {
        log("Loading TTF font: \(fontName) with size: \(fontSize)", level: .debug)
        
        // Try to load TTF font from bundle
        guard let fontURL = Bundle.main.url(forResource: fontName, withExtension: "ttf", subdirectory: "fonts") else {
            log("TTF font not found in fonts subdirectory, trying main bundle", level: .debug)
            guard let fontURL = Bundle.main.url(forResource: fontName, withExtension: "ttf") else {
                log("Failed to find TTF font: \(fontName).ttf", level: .error)
                return false
            }
            return loadTTFFontFromURL(fontURL, fontName: fontName, fontSize: fontSize)
        }
        
        return loadTTFFontFromURL(fontURL, fontName: fontName, fontSize: fontSize)
    }
    
    private func loadTTFFontFromURL(_ fontURL: URL, fontName: String, fontSize: Float) -> Bool {
        guard let fontData = try? Data(contentsOf: fontURL),
              let dataProvider = CGDataProvider(data: fontData as CFData),
              let cgFont = CGFont(dataProvider) else {
            log("Failed to create CTFont from URL: \(fontURL)", level: .error)
            return false
        }
        
        let font = CTFontCreateWithGraphicsFont(cgFont, CGFloat(fontSize), nil, nil)
        
        log("Successfully created CTFont from: \(fontURL.lastPathComponent)", level: .debug)
        
        // Get font metrics
        let ascent = CTFontGetAscent(font)
        let descent = CTFontGetDescent(font)
        let leading = CTFontGetLeading(font)
        let lineHeight = ascent + descent + leading
        
        // Generate SDF atlas
        let atlasSize = 1024 // 1024x1024 atlas
        let padding = 4 // Padding between glyphs
        
        guard let atlasImage = generateSDFAtlas(from: font, atlasSize: atlasSize, padding: padding) else {
            log("Failed to generate SDF atlas", level: .error)
            return false
        }
        
        // Cache the atlas image
        let cacheKey = "\(fontName)_\(Int(fontSize))"
        fontAtlasCache[cacheKey] = atlasImage
        
        // Convert UIImage to Metal texture
        guard let device = device,
              let cgImage = atlasImage.cgImage else {
            log("Failed to get CGImage from atlas or device not available", level: .error)
            return false
        }
        
        do {
            let textureLoader = MTKTextureLoader(device: device)
            fontAtlas = try textureLoader.newTexture(cgImage: cgImage, options: [
                .SRGB: false,
                .generateMipmaps: false
            ])
            log("Successfully created Metal texture from font atlas", level: .debug)
        } catch {
            log("Failed to create Metal texture from font atlas: \(error)", level: .error)
            return false
        }
        
        // Store font metrics
        fontMetrics = FontMetrics(
            size: Float(lineHeight),
            lineHeight: Float(lineHeight),
            ascender: Float(ascent),
            descender: Float(descent),
            base: Float(ascent),
            atlasWidth: Float(atlasSize),
            atlasHeight: Float(atlasSize)
        )
        
        log("Generated SDF atlas for TTF font: \(atlasSize)x\(atlasSize), \(glyphMap.count) characters", level: .debug)
        return true
    }
    

    
    public func drawTextSDF(_ text: String, x: Float, y: Float, fontSize: Float, r: Float, g: Float, b: Float, a: Float) {
        guard let fontAtlas = fontAtlas,
              let fontMetrics = fontMetrics,
              let device = device,
              let samplerState = samplerState else {
            log("drawText: Font system not properly initialized", level: .error)
            return
        }
        
        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawText: Failed to get render encoder", level: .error)
            return
        }
        
        // Build all vertices and indices for the entire text string in one batch
        var allVertices: [Float] = []
        var allIndices: [UInt16] = []
        var vertexCount: UInt16 = 0
        
        let scale = fontSize / fontMetrics.size
        var cursorX = x
        let cursorY = y
        
        // Render text with proper SDF scaling and baseline alignment
        var currentLineY = cursorY
        let lineHeight = fontMetrics.lineHeight * scale  // Remove artificial Y stretch factor
        
        for char in text {
            // Handle line breaks
            if char == "\n" {
                currentLineY += lineHeight  // Add line height to move down to next line
                cursorX = x  // Reset to start of line
                continue
            }
            
            guard let glyph = glyphMap[char] else {
                log("Character '\(char)' not found in font atlas", level: .warning)
                cursorX += fontSize * 0.5 // Default advance for unknown chars
                continue
            }
            
            // Calculate glyph screen position with cell-based positioning
            let glyphX = cursorX  // No bearing offset for cell-centered glyphs
            // For cell-based atlas, use baseline calculation without artificial centering
            let glyphY = currentLineY - fontMetrics.ascender * scale  // Pure baseline positioning
            let glyphWidth = glyph.width * scale
            let glyphHeight = glyph.height * scale  // No vertical stretch to ensure accurate positioning
            
            // Calculate UV coordinates - Metal uses Y-up coordinate system
            let u1 = glyph.atlasX
            let v1 = 1.0 - glyph.atlasY - glyph.atlasHeight  // Flip Y for Metal
            let u2 = glyph.atlasX + glyph.atlasWidth
            let v2 = 1.0 - glyph.atlasY  // Flip Y for Metal
            
            // Add vertices for this glyph to the batch
            let glyphVertices: [Float] = [
                // Bottom-left
                glyphX, glyphY, u1, v1, r, g, b, a,
                // Bottom-right
                glyphX + glyphWidth, glyphY, u2, v1, r, g, b, a,
                // Top-right
                glyphX + glyphWidth, glyphY + glyphHeight, u2, v2, r, g, b, a,
                // Top-left
                glyphX, glyphY + glyphHeight, u1, v2, r, g, b, a
            ]
            
            allVertices.append(contentsOf: glyphVertices)
            
            // Add indices for this glyph (offset by current vertex count)
            let glyphIndices: [UInt16] = [
                vertexCount + 0, vertexCount + 1, vertexCount + 2,
                vertexCount + 2, vertexCount + 3, vertexCount + 0
            ]
            allIndices.append(contentsOf: glyphIndices)
            vertexCount += 4
            
            // Advance cursor with improved character spacing
            cursorX += glyph.advance * scale + (fontSize * 0.1)  // Use 10% of font size for spacing
        }
        
        // Only render if we have valid characters
        guard !allVertices.isEmpty else {
            log("drawText: No valid characters found in text '\(text)'", level: .error)
            return
        }
        
        // Prepare vertex and index buffers for efficient batch rendering
        guard let batchVertexBuffer = device.makeBuffer(bytes: allVertices, length: allVertices.count * MemoryLayout<Float>.stride, options: []),
              let batchIndexBuffer = device.makeBuffer(bytes: allIndices, length: allIndices.count * MemoryLayout<UInt16>.stride, options: []) else {
            log("drawText: Failed to create batch buffers for text '\(text)'", level: .error)
            return
        }
        
        // Configure render encoder for SDF text rendering
        guard let sdfPipeline = sdfTextPipelineState else {
            log("drawText: SDF text pipeline state not available", level: .error)
            return
        }
        
        renderEncoder.setRenderPipelineState(sdfPipeline)
        renderEncoder.setVertexBuffer(batchVertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)
        renderEncoder.setFragmentTexture(fontAtlas, index: 0)
        renderEncoder.setFragmentSamplerState(samplerState, index: 0)
        
        // Draw all characters in one call
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: allIndices.count, indexType: .uint16, indexBuffer: batchIndexBuffer, indexBufferOffset: 0)
        
        log("Text '\(text)' drawn at (\(x), \(y)) with size \(fontSize)", level: .debug)
        
        // DEBUG: Log actual vertex data being sent
        if !allVertices.isEmpty {
            log("Text vertex data - First vertex: [\(allVertices[0]), \(allVertices[1]), \(allVertices[2]), \(allVertices[3]), \(allVertices[4])]", level: .debug)
            log("Font atlas texture size: \(fontAtlas.width)x\(fontAtlas.height), format: \(fontAtlas.pixelFormat)", level: .debug)
        }
    }
    
    public func drawTextWithOutline(_ text: String, x: Float, y: Float, fontSize: Float, 
                                   textR: Float, textG: Float, textB: Float, textA: Float,
                                   outlineR: Float, outlineG: Float, outlineB: Float, outlineA: Float,
                                   outlineWidth: Float) {
        // Implementation would be similar to drawText but using sdfTextOutlinePipelineState
        // and passing outline parameters to the shader
        log("drawTextWithOutline not yet implemented", level: .warning)
    }
    
    public func drawTextWithShadow(_ text: String, x: Float, y: Float, fontSize: Float,
                                  textR: Float, textG: Float, textB: Float, textA: Float,
                                  shadowR: Float, shadowG: Float, shadowB: Float, shadowA: Float,
                                  shadowOffsetX: Float, shadowOffsetY: Float) {
        // Implementation would be similar to drawText but using sdfTextShadowPipelineState
        // and passing shadow parameters to the shader
        log("drawTextWithShadow not yet implemented", level: .warning)
    }
    
    private func generateSDFAtlas(from font: CTFont, atlasSize: Int, padding: Int) -> UIImage? {
        log("Generating SDF atlas from TTF font, size: \(atlasSize)x\(atlasSize)", level: .debug)
        
        // First create a high-resolution bitmap to draw glyphs
        let superSampleFactor = 2  // Moderate supersampling for good quality and performance
        let highResSize = atlasSize * superSampleFactor
        
        // Create high-res context for drawing glyphs
        let colorSpace = CGColorSpaceCreateDeviceGray()
        guard let highResContext = CGContext(data: nil, width: highResSize, height: highResSize, bitsPerComponent: 8, bytesPerRow: highResSize, space: colorSpace, bitmapInfo: CGImageAlphaInfo.none.rawValue) else {
            log("Failed to create high-res graphics context for atlas", level: .error)
            return nil
        }
        
        // Clear with black background (outside glyph)
        highResContext.setFillColor(CGColor(gray: 0.0, alpha: 1.0))
        highResContext.fill(CGRect(x: 0, y: 0, width: highResSize, height: highResSize))
        
        // Characters to include in the atlas (basic ASCII + common symbols)
        let characters = Array(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")
        
        // Fixed grid layout: 16x6 = 96 characters (ASCII 32-127)
        let gridCols = 16
        let gridRows = 6
        let cellWidth = CGFloat(atlasSize) / CGFloat(gridCols)
        let cellHeight = CGFloat(atlasSize) / CGFloat(gridRows)
        
        glyphMap.removeAll()
        
        // Draw glyphs to high-res bitmap using fixed grid layout
        for (index, char) in characters.enumerated() {
            // Calculate grid position
            let col = index % gridCols
            let row = index / gridCols
            
            log("Processing character '\(char)' (ASCII \(char.unicodeScalars.first!.value)) at grid position (\(col), \(row))", level: .debug)
            
            // Calculate cell position
            let currentX = CGFloat(col) * cellWidth
            let currentY = CGFloat(row) * cellHeight
            // Get glyph for this character
            let unichars = [UniChar(char.unicodeScalars.first!.value)]
            var glyphs = [CGGlyph](repeating: 0, count: 1)
            let success = CTFontGetGlyphsForCharacters(font, unichars, &glyphs, 1)
            
            guard success && glyphs[0] != 0 else {
                log("Failed to get glyph for character: \(char)", level: .warning)
                continue
            }
            
            var glyph = glyphs[0]
            
            // Get glyph metrics
            var boundingRect = CGRect.zero
            CTFontGetBoundingRectsForGlyphs(font, .horizontal, &glyph, &boundingRect, 1)
            
            var advance = CGSize.zero
            CTFontGetAdvancesForGlyphs(font, .horizontal, &glyph, &advance, 1)
            
            // Use fixed cell dimensions for consistent layout
            _ = cellWidth  // Fixed cell width
            _ = cellHeight // Fixed cell height
            
            // Render glyph to high-res context
            highResContext.saveGState()
            highResContext.textMatrix = CGAffineTransform.identity
            
            // Scale position for high-res rendering
            let highResX = currentX * CGFloat(superSampleFactor)
            let highResY = currentY * CGFloat(superSampleFactor)
            
            // Center the glyph within the fixed cell (scaled for high-res)
            let cellCenterX = highResX + (cellWidth * CGFloat(superSampleFactor)) / 2.0
            let cellCenterY = highResY + (cellHeight * CGFloat(superSampleFactor)) / 2.0
            
            // Position glyph at center of cell (with special handling for punctuation)
            let glyphCenterX = cellCenterX - (boundingRect.width * CGFloat(superSampleFactor)) / 2.0
            
            // Check if this is a character that should align to baseline (only small bottom-sitting chars)
            let isBottomAligned = ".,:_".contains(char)  // Only periods, commas, colons, underscores
            let glyphCenterY: CGFloat
            if isBottomAligned {
                // Position small punctuation at top of cell (inverted for Metal coordinates)
                let cellTop = highResY
                glyphCenterY = cellTop + (boundingRect.height * CGFloat(superSampleFactor)) + (cellHeight * CGFloat(superSampleFactor) * 0.1) // 10% padding from top
            } else {
                // Normal characters (including ! ? [] {} () etc.) remain centered
                glyphCenterY = cellCenterY - (boundingRect.height * CGFloat(superSampleFactor)) / 2.0
            }
            
            highResContext.translateBy(x: glyphCenterX - boundingRect.minX * CGFloat(superSampleFactor), 
                                      y: glyphCenterY - boundingRect.minY * CGFloat(superSampleFactor))
            highResContext.scaleBy(x: CGFloat(superSampleFactor), y: CGFloat(superSampleFactor))
            
            let glyphPath = CTFontCreatePathForGlyph(font, glyph, nil)
            if let path = glyphPath {
                highResContext.addPath(path)
                highResContext.setFillColor(CGColor(gray: 1.0, alpha: 1.0)) // White = inside glyph
                highResContext.fillPath()
            }
            
            highResContext.restoreGState()
            
            // Store glyph information with fixed grid coordinates
            // Note: Y-axis is flipped for Metal texture coordinates
            // IMPORTANT: For consistent rendering, we use the cell dimensions for both atlas UV and screen rendering
            // This ensures that punctuation and small characters render at the correct size
            
            let glyphInfo = GlyphInfo(
                atlasX: Float(currentX) / Float(atlasSize),
                atlasY: Float(currentY) / Float(atlasSize),
                atlasWidth: Float(cellWidth) / Float(atlasSize),
                atlasHeight: Float(cellHeight) / Float(atlasSize),
                bearingX: 0.0,  // No bearing offset for cell-centered glyphs
                bearingY: 0.0,  // No bearing offset for cell-centered glyphs
                advance: Float(advance.width),
                width: Float(cellWidth),  // Use cell width for consistent rendering
                height: Float(cellHeight) // Use cell height for consistent rendering
            )
            
            glyphMap[char] = glyphInfo
        }
        
        // Get the high-res bitmap data
        guard let highResImage = highResContext.makeImage() else {
            log("Failed to create high-res image from context", level: .error)
            return nil
        }
        
        log("High-res image created: \(highResImage.width)x\(highResImage.height)", level: .debug)
        
        // Convert to raw pixel data for SDF calculation
        guard let highResData = highResImage.dataProvider?.data,
              let highResPixels = CFDataGetBytePtr(highResData) else {
            log("Failed to get pixel data from high-res image", level: .error)
            return nil
        }
        
        // Generate SDF from the high-res bitmap
        let sdfData = generateSignedDistanceField(from: highResPixels, 
                                                 width: highResSize, 
                                                 height: highResSize, 
                                                 outputWidth: atlasSize, 
                                                 outputHeight: atlasSize,
                                                 spread: Float(padding * superSampleFactor))
        
        // Create SDF image
        let sdfDataPtr = UnsafeMutablePointer<UInt8>.allocate(capacity: sdfData.count)
        sdfDataPtr.initialize(from: sdfData, count: sdfData.count)
        defer { sdfDataPtr.deallocate() }
        
        guard let sdfContext = CGContext(data: sdfDataPtr, width: atlasSize, height: atlasSize, bitsPerComponent: 8, bytesPerRow: atlasSize, space: colorSpace, bitmapInfo: CGImageAlphaInfo.none.rawValue),
              let sdfCGImage = sdfContext.makeImage() else {
            log("Failed to create SDF context or image", level: .error)
            return nil
        }
        
        // Save debug image to app container for inspection
        if let documentPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first {
            let debugImagePath = documentPath.appendingPathComponent("sdf_atlas_debug.png")
            let debugImageURL = debugImagePath as CFURL
            
            if let destination = CGImageDestinationCreateWithURL(debugImageURL, "public.png" as CFString, 1, nil) {
                CGImageDestinationAddImage(destination, sdfCGImage, nil)
                CGImageDestinationFinalize(destination)
                log("SDF atlas debug image saved to: \(debugImagePath.path)", level: .debug)
            }
        }
        
        return UIImage(cgImage: sdfCGImage)
    }
    
    // MARK: - SDF Generation Algorithm
    
    /// Generates a signed distance field from a high-resolution bitmap
    /// This implements an efficient distance transform algorithm - O(n) complexity
    private func generateSignedDistanceField(from pixels: UnsafePointer<UInt8>, 
                                           width: Int, height: Int, 
                                           outputWidth: Int, outputHeight: Int,
                                           spread: Float) -> [UInt8] {
        
        log("Generating SDF: input \(width)x\(height), output \(outputWidth)x\(outputHeight), spread \(spread)", level: .debug)
        
        // First, downsample the input to output resolution
        var bitmap = [Bool](repeating: false, count: outputWidth * outputHeight)
        
        for y in 0..<outputHeight {
            for x in 0..<outputWidth {
                // Map output coordinates to input coordinates
                let inputX = Int(Float(x) * Float(width) / Float(outputWidth))
                let inputY = Int(Float(y) * Float(height) / Float(outputHeight))
                
                let inputIndex = inputY * width + inputX
                if inputIndex < width * height {
                    bitmap[y * outputWidth + x] = pixels[inputIndex] >= 128
                }
            }
        }
        
        // Debug: Check if bitmap has any data
        let totalTrue = bitmap.filter { $0 }.count
        let totalFalse = bitmap.filter { !$0 }.count
        log("Bitmap analysis: \(totalTrue) inside pixels, \(totalFalse) outside pixels", level: .debug)
        
        // Generate distance fields for inside and outside
        let insideDistances = computeDistanceTransform(bitmap: bitmap, width: outputWidth, height: outputHeight, findInside: true)
        let outsideDistances = computeDistanceTransform(bitmap: bitmap, width: outputWidth, height: outputHeight, findInside: false)
        
        // Combine into signed distance field
        var sdfData = [UInt8](repeating: 127, count: outputWidth * outputHeight)
        let maxDist = spread / 2.0  // Use half spread as max distance for better range
        
        for i in 0..<(outputWidth * outputHeight) {
            let inside = bitmap[i]
            
            // For SDF: inside pixels use distance to outside (negative), outside pixels use distance to inside (positive)
            let distanceToOpposite = inside ? outsideDistances[i] : insideDistances[i]
            let signedDistance = inside ? -distanceToOpposite : distanceToOpposite
            
            // Normalize to 0-255 range, with 128 = edge (distance 0) to match shader expectations
            let normalizedDistance = (signedDistance / maxDist) * 128.0 + 128.0
            let clampedDistance = max(0.0, min(255.0, normalizedDistance))
            
            sdfData[i] = UInt8(clampedDistance)
        }
        
        // Debug: Check final SDF values
        let minVal = sdfData.min() ?? 0
        let maxVal = sdfData.max() ?? 255
        let avgVal = Int(sdfData.reduce(0) { $0 + Int($1) }) / sdfData.count
        log("SDF generation completed - Min: \(minVal), Max: \(maxVal), Avg: \(avgVal)", level: .debug)
        
        return sdfData
    }
    
    /// Efficient distance transform using separable algorithm - O(n) complexity
    private func computeDistanceTransform(bitmap: [Bool], width: Int, height: Int, findInside: Bool) -> [Float] {
        var distances = [Float](repeating: Float.greatestFiniteMagnitude, count: width * height)
        
        // Initialize distances - 0 for target pixels, infinity for others
        for i in 0..<(width * height) {
            if bitmap[i] == findInside {
                distances[i] = 0.0
            }
        }
        
        // Forward pass - process rows left to right
        for y in 0..<height {
            for x in 1..<width {
                let idx = y * width + x
                let leftIdx = y * width + (x - 1)
                distances[idx] = min(distances[idx], distances[leftIdx] + 1.0)
            }
        }
        
        // Backward pass - process rows right to left
        for y in 0..<height {
            for x in stride(from: width - 2, through: 0, by: -1) {
                let idx = y * width + x
                let rightIdx = y * width + (x + 1)
                distances[idx] = min(distances[idx], distances[rightIdx] + 1.0)
            }
        }
        
        // Forward pass - process columns top to bottom
        for x in 0..<width {
            for y in 1..<height {
                let idx = y * width + x
                let topIdx = (y - 1) * width + x
                distances[idx] = min(distances[idx], distances[topIdx] + 1.0)
            }
        }
        
        // Backward pass - process columns bottom to top
        for x in 0..<width {
            for y in stride(from: height - 2, through: 0, by: -1) {
                let idx = y * width + x
                let bottomIdx = (y + 1) * width + x
                distances[idx] = min(distances[idx], distances[bottomIdx] + 1.0)
            }
        }
        
        // Convert Manhattan distance to approximate Euclidean distance
        // This is a good approximation that's much faster than true Euclidean
        for i in 0..<distances.count {
            if distances[i] < Float.greatestFiniteMagnitude {
                // Apply a scaling factor to approximate Euclidean distance
                distances[i] = distances[i] * 0.8  // Approximate correction factor
            } else {
                distances[i] = 64.0  // Max distance for pixels that are very far
            }
        }
        
        return distances
    }
    // MARK: - Screen Size Query
    
    public func getScreenSize() -> (width: Float, height: Float) {
        return (width: Float(viewportSize.width), height: Float(viewportSize.height))
    }
    
    // MARK: - Enhanced Screen Information
    
    public func getScreenInfo() -> GameCore.ScreenInfo {
        // Get the main screen for device information
        let mainScreen = UIScreen.main
        
        // Get logical bounds (in points)
        let logicalBounds = mainScreen.bounds
        let logicalWidth = Float(logicalBounds.width)
        let logicalHeight = Float(logicalBounds.height)
        
        // Get pixel bounds (native scale)
        let pixelBounds = mainScreen.nativeBounds
        let pixelWidth = Float(pixelBounds.width)
        let pixelHeight = Float(pixelBounds.height)
        
        // Calculate scale factor
        let scaleFactor = Float(mainScreen.nativeScale)
        
        // Determine orientation
        let isPortrait = logicalHeight > logicalWidth
        
        // Get device model (simplified)
        let deviceModel = getDeviceModel()
        
        log("Screen Info - Logical: \(logicalWidth)x\(logicalHeight), Pixel: \(pixelWidth)x\(pixelHeight), Scale: \(scaleFactor), Portrait: \(isPortrait), Device: \(deviceModel)", level: .debug)
        
        // Create and return ScreenInfo struct
        var screenInfo = GameCore.ScreenInfo()
        screenInfo.logicalWidth = logicalWidth
        screenInfo.logicalHeight = logicalHeight
        screenInfo.pixelWidth = pixelWidth
        screenInfo.pixelHeight = pixelHeight
        screenInfo.scaleFactor = scaleFactor
        screenInfo.isPortrait = isPortrait
        screenInfo.deviceModel = std.string(deviceModel)
        
        return screenInfo
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
        platformMetadata.platformHandle = 0 // We don't expose handles through metadata
        
        log("Texture metadata for \(textureId): \(metadata.width)x\(metadata.height), loaded: \(metadata.isLoaded)", level: .debug)
        
        return platformMetadata
    }
    
    // MARK: - Helper Methods
    
    private func generateGlyphMetricsFromTTF(ctFont: CTFont, atlasWidth: Float, atlasHeight: Float) -> Bool {
        log("Generating glyph metrics from TTF font", level: .debug)
        
        // Clear existing glyph map
        glyphMap.removeAll()
        
        // Simple ordered character set (ASCII 32-126)
        let characters = Array(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")
        
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
            log("Generated glyph metrics for '\(character)' at grid position (\(col), \(row)): UV(\(u), \(v)) Size(\(uWidth), \(vHeight))", level: .debug)
        }
        
        log("Generated \(glyphMap.count) glyph metrics from TTF using fixed grid layout", level: .debug)
        return !glyphMap.isEmpty
    }
    
    private func loadBMFontGlyphs(fontName: String) -> Bool {
        log("Loading BMFont glyphs for: \(fontName)", level: .debug)
        
        // Try to load BMFont file from bundle
        guard let fntURL = Bundle.main.url(forResource: fontName, withExtension: "fnt", subdirectory: "fonts") else {
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
                    var x: Float = 0, y: Float = 0
                    var width: Float = 0, height: Float = 0
                    var xoffset: Float = 0, yoffset: Float = 0
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
    
    private func createTexture(width: Int, height: Int, pixelFormat: MTLPixelFormat = .rgba8Unorm) -> MTLTexture? {
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
    
    public func measureText(_ text: String, fontSize: Float) -> (width: Float, height: Float) {
        guard let fontMetrics = fontMetrics else {
            log("measureText: Font metrics not available", level: .error)
            return (width: 0, height: 0)
        }
        
        let scale = fontSize / fontMetrics.size
        var maxWidth: Float = 0
        var currentLineWidth: Float = 0
        var totalHeight: Float = 0
        var lineCount: Int = 1
        
        for char in text {
            // Handle line breaks
            if char == "\n" {
                maxWidth = max(maxWidth, currentLineWidth)
                currentLineWidth = 0
                lineCount += 1
                continue
            }
            
            guard let glyph = glyphMap[char] else {
                currentLineWidth += fontSize * 0.5 // Default advance for unknown chars
                continue
            }
            
            currentLineWidth += glyph.advance * scale + (fontSize * 0.1) // Add character spacing (match drawTextSDF)
        }
        
        // Check the last line
        maxWidth = max(maxWidth, currentLineWidth)
        
        // Remove the last character spacing from the last line
        if !text.isEmpty && !text.hasSuffix("\n") {
            maxWidth -= (fontSize * 0.1)
        }
        
        // Calculate total height based on line count
        let lineHeight = fontMetrics.lineHeight * scale  // Remove artificial Y stretch factor
        totalHeight = Float(lineCount) * lineHeight
        
        return (width: maxWidth, height: totalHeight)
    }
    
    public func drawTextCentered(_ text: String, x: Float, y: Float, fontSize: Float, r: Float, g: Float, b: Float, a: Float) {
        let textSize = measureText(text, fontSize: fontSize)
        
        // Center horizontally around the provided X coordinate
        let centeredX = x - textSize.width * 0.5
        
        // Center vertically around the provided Y coordinate
        // The Y coordinate from UISystem is the center point where text should be centered
        let centeredY = y - textSize.height * 0.5
        
        // Debug: Log text centering calculations
        log("MetalRenderer: drawTextCentered '\(text)' - Input center (\(x),\(y)), TextSize (\(textSize.width),\(textSize.height)), Final position (\(centeredX),\(centeredY))", level: .debug)
        
        drawTextSDF(text, x: centeredX, y: centeredY, fontSize: fontSize, r: r, g: g, b: b, a: a)
    }
}