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
import simd

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
    private var clearColor: MTLClearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
    
    // MTKView connection (weak, private)
    private weak var metalView: MTKView?
    
    // Resource management (private)
    private var textures: [UInt32: MTLTexture] = [:]
    private var nextTextureHandle: UInt32 = 1
    
    // SDF Font Atlas Management
    private var fontAtlas: MTLTexture?
    private var fontMetrics: FontMetrics?
    private var glyphMap: [Character: GlyphInfo] = [:]
    private var fontAtlasCache: [String: UIImage] = [:] // Cache for generated font atlases
    
    // MARK: - Initialization (@MainActor ensures main thread execution)
    
    public init() {
        log("MetalRenderer init() called - setting up Metal on main thread", level: .debug)
        setupMetal()
        log("MetalRenderer init() completed - device: \(device != nil), commandQueue: \(commandQueue != nil)", level: .debug)
    }
    
    deinit {
        // Note: Cannot call @MainActor shutdown() from deinit
        // Resources will be cleaned up when the actor is deallocated
        // or shutdown() should be called explicitly before deallocation
    }
    
    // MARK: - Metal Setup
    
    private func setupMetal() {
        log("setupMetal() starting - initializing Metal device and command queue", level: .debug)
        
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
        
        log("setupMetal() completed - ready for rendering", level: .debug)
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
        texturedPipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
        
        // Enable blending for textured rendering
        texturedPipelineDescriptor.colorAttachments[0].isBlendingEnabled = true
        texturedPipelineDescriptor.colorAttachments[0].rgbBlendOperation = .add
        texturedPipelineDescriptor.colorAttachments[0].alphaBlendOperation = .add
        texturedPipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        texturedPipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = .sourceAlpha
        texturedPipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        texturedPipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
        
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
        samplerDescriptor.minFilter = .linear
        samplerDescriptor.magFilter = .linear
        samplerDescriptor.mipFilter = .linear
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
            -1.0, -1.0,  0.0, 1.0,  1.0, 1.0, 1.0, 1.0,
            // Bottom-right
             1.0, -1.0,  1.0, 1.0,  1.0, 1.0, 1.0, 1.0,
            // Top-right
             1.0,  1.0,  1.0, 0.0,  1.0, 1.0, 1.0, 1.0,
            // Top-left
            -1.0,  1.0,  0.0, 0.0,  1.0, 1.0, 1.0, 1.0
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
        view.colorPixelFormat = .bgra8Unorm
        view.clearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
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
        
        // Create orthographic projection matrix
        let left: Float = 0.0
        let right: Float = width
        let bottom: Float = height
        let top: Float = 0.0
        let near: Float = -1.0
        let far: Float = 1.0
        
        // FIX: Metal uses column-major matrices, so we need to transpose this
        let projectionMatrix = simd_float4x4(
            simd_float4(2.0 / (right - left), 0, 0, 0),
            simd_float4(0, 2.0 / (top - bottom), 0, 0),
            simd_float4(0, 0, -2.0 / (far - near), 0),
            simd_float4(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1)
        )
        
        let contents = uniformBuffer.contents().bindMemory(to: simd_float4x4.self, capacity: 1)
        contents.pointee = projectionMatrix
        
        // NOTE: didModifyRange not needed on iOS - cache coherency is automatic
        
        print("🔧 Updated projection matrix for viewport \(width)x\(height): left=\(left), right=\(right), top=\(top), bottom=\(bottom)")
        log("Updated projection matrix for viewport \(width)x\(height): left=\(left), right=\(right), top=\(top), bottom=\(bottom)", level: .debug)
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
        
        // FIX: Set clear color and load action - this will clear when the render encoder is created
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
            
            print("🎬 Created new render encoder for frame - viewport: \(viewportSize)")
            log("Created new render encoder for frame - viewport: \(viewportSize)", level: .debug)
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
        
        // Use print for immediate debug output
        print("🎯 Drawing rectangle at (\(x), \(y)) size (\(width), \(height)) color (\(r), \(g), \(b), \(a))")
        print("🎯 Vertices: [\(vertices[0]),\(vertices[1]) \(vertices[8]),\(vertices[9]) \(vertices[16]),\(vertices[17]) \(vertices[24]),\(vertices[25])]")
        log("Drawing rectangle: vertices=[\(vertices[0]),\(vertices[1]) \(vertices[8]),\(vertices[9]) \(vertices[16]),\(vertices[17]) \(vertices[24]),\(vertices[25])]", level: .debug)
        
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
        
        log("Rectangle drawn at (\(x), \(y)) with size (\(width), \(height)) and color (\(r), \(g), \(b), \(a)) - Viewport: \(viewportSize.width)x\(viewportSize.height)", level: .debug)
    }
    
    public func drawTexture(textureHandle: UInt32, x: Float, y: Float, width: Float, height: Float) {
        guard let texture = textures[textureHandle] else {
            log("drawTexture: Invalid texture handle \(textureHandle)", level: .warning)
            return
        }
        
        guard let texturedPipelineState = texturedPipelineState,
              let vertexBuffer = vertexBuffer,
              let indexBuffer = indexBuffer,
              let uniformBuffer = uniformBuffer else {
            log("drawTexture: Missing required Metal resources", level: .warning)
            return
        }
        
        // FIX: Use the single render encoder for the entire frame
        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawTexture: Failed to get render encoder", level: .error)
            return
        }
        
        renderEncoder.setRenderPipelineState(texturedPipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.setVertexBuffer(uniformBuffer, offset: 0, index: 1)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.setFragmentSamplerState(samplerState, index: 0)
        
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer, indexBufferOffset: 0)
        
        log("Texture \(textureHandle) drawn at (\(x), \(y)) with size (\(width), \(height))", level: .debug)
    }
    
    public func drawText(text: String, x: Float, y: Float, fontSize: Float, 
                        r: Float, g: Float, b: Float, a: Float) {
        // Text rendering implementation would go here
        // For now, just log the call
        log("drawText: '\(text)' at (\(x), \(y)) with fontSize \(fontSize)", level: .debug)
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
    
    // MARK: - Additional Drawing Methods for Threading System
    
    public func clearScreen(_ r: Float, _ g: Float, _ b: Float, _ a: Float) {
        setClearColor(r: r, g: g, b: b, a: a)
        clearScreen()
    }
    
    public func drawSprite(_ sprite: UInt32, _ x: Float, _ y: Float, _ rotation: Float) {
        // For now, treat sprites as textures with full size
        if let texture = textures[sprite] {
            let width = Float(texture.width)
            let height = Float(texture.height)
            drawTexture(textureHandle: sprite, x: x, y: y, width: width, height: height)
        } else {
            log("drawSprite: Invalid sprite handle \(sprite)", level: .warning)
        }
    }
    
    public func drawSpriteScaled(_ sprite: UInt32, _ x: Float, _ y: Float, _ scaleX: Float, _ scaleY: Float, _ rotation: Float) {
        // For now, treat sprites as textures with scaled size
        if let texture = textures[sprite] {
            let width = Float(texture.width) * scaleX
            let height = Float(texture.height) * scaleY
            drawTexture(textureHandle: sprite, x: x, y: y, width: width, height: height)
        } else {
            log("drawSpriteScaled: Invalid sprite handle \(sprite)", level: .warning)
        }
    }
    
    public func drawCircle(_ x: Float, _ y: Float, _ radius: Float, _ r: Float, _ g: Float, _ b: Float, _ a: Float) {
        drawCircle(x: x, y: y, radius: radius, r: r, g: g, b: b, a: a, segments: 32)
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
        // Clear any cached atlas to ensure we regenerate with new algorithm
        let cacheKey = "\(fontName)_\(Int(fontSize))"
        fontAtlasCache.removeValue(forKey: cacheKey)
        
        // First, try to load the .fnt file to get glyph metrics
        guard loadBMFontMetrics(fontName: fontName) else {
            log("Failed to load font metrics for: \(fontName)", level: .error)
            return false
        }
        
        // Generate an SDF atlas from the font
        guard let fontAtlasImage = generateFontAtlas(fontName: fontName, fontSize: fontSize) else {
            log("Failed to generate SDF atlas for: \(fontName)", level: .error)
            return false
        }
        
        // Create Metal texture from the atlas image
        guard let device = device else {
            log("Device not available for font loading", level: .error)
            return false
        }
        
        let textureLoader = MTKTextureLoader(device: device)
        do {
            fontAtlas = try textureLoader.newTexture(cgImage: fontAtlasImage.cgImage!, options: [
                .textureUsage: MTLTextureUsage.shaderRead.rawValue,
                .textureStorageMode: MTLStorageMode.shared.rawValue
            ])
            
            log("Font atlas texture created successfully for: \(fontName)", level: .debug)
            return true
            
        } catch {
            log("Failed to create texture from font atlas: \(error)", level: .error)
            return false
        }
    }
    
    private func loadBMFontMetrics(fontName: String) -> Bool {
        // Load the .fnt file from the bundle - try multiple possible locations
        var fontData: String?
        
        // Try loading from main bundle first
        if let fontPath = Bundle.main.path(forResource: fontName, ofType: "fnt") {
            fontData = try? String(contentsOfFile: fontPath)
        }
        
        // If not found, try loading from the fonts subfolder in assets
        if fontData == nil, let fontPath = Bundle.main.path(forResource: fontName, ofType: "fnt", inDirectory: "fonts") {
            fontData = try? String(contentsOfFile: fontPath)
        }
        
        // If still not found, try loading as a bundled resource directly
        if fontData == nil, let fontURL = Bundle.main.url(forResource: fontName, withExtension: "fnt") {
            fontData = try? String(contentsOf: fontURL)
        }
        
        guard let fntContent = fontData else {
            log("Failed to load .fnt file for font: \(fontName) from any location", level: .error)
            return false
        }
        
        // Parse the BMFont format
        let lines = fntContent.components(separatedBy: .newlines)
        var lineHeight: Float = 48
        var base: Float = 38
        var atlasWidth: Float = 512
        var atlasHeight: Float = 512
        
        glyphMap.removeAll()
        
        for line in lines {
            let trimmedLine = line.trimmingCharacters(in: .whitespaces)
            
            if trimmedLine.hasPrefix("common") {
                // Parse: common lineHeight=48 base=38 scaleW=512 scaleH=512 pages=1 packed=0
                if let lineHeightMatch = extractValue(from: trimmedLine, key: "lineHeight") {
                    lineHeight = Float(lineHeightMatch) ?? 48
                }
                if let baseMatch = extractValue(from: trimmedLine, key: "base") {
                    base = Float(baseMatch) ?? 38
                }
                if let scaleWMatch = extractValue(from: trimmedLine, key: "scaleW") {
                    atlasWidth = Float(scaleWMatch) ?? 512
                }
                if let scaleHMatch = extractValue(from: trimmedLine, key: "scaleH") {
                    atlasHeight = Float(scaleHMatch) ?? 512
                }
            } else if trimmedLine.hasPrefix("char id=") {
                // Parse: char id=65 x=0 y=0 width=32 height=32 xoffset=0 yoffset=8 xadvance=28 page=0 chnl=0
                if let glyph = parseCharLine(trimmedLine, atlasWidth: atlasWidth, atlasHeight: atlasHeight) {
                    if let character = UnicodeScalar(glyph.charId) {
                        glyphMap[Character(character)] = glyph.glyphInfo
                    }
                }
            }
        }
        
        // Store font metrics
        fontMetrics = FontMetrics(
            size: lineHeight,
            lineHeight: lineHeight,
            ascender: lineHeight * 0.8,
            descender: lineHeight * 0.2,
            base: base,
            atlasWidth: atlasWidth,
            atlasHeight: atlasHeight
        )
        
        log("Loaded BMFont with \(glyphMap.count) characters, atlas: \(atlasWidth)x\(atlasHeight)", level: .debug)
        return true
    }
    
    private func extractValue(from line: String, key: String) -> String? {
        let pattern = "\(key)=(\\d+)"
        let regex = try? NSRegularExpression(pattern: pattern)
        let range = NSRange(line.startIndex..<line.endIndex, in: line)
        
        if let match = regex?.firstMatch(in: line, options: [], range: range) {
            let matchRange = Range(match.range(at: 1), in: line)!
            return String(line[matchRange])
        }
        return nil
    }
    
    private func parseCharLine(_ line: String, atlasWidth: Float, atlasHeight: Float) -> (charId: Int, glyphInfo: GlyphInfo)? {
        // Extract values using regex patterns
        let patterns = [
            "id=(\\d+)",
            "x=(\\d+)",
            "y=(\\d+)", 
            "width=(\\d+)",
            "height=(\\d+)",
            "xoffset=(-?\\d+)",
            "yoffset=(-?\\d+)",
            "xadvance=(\\d+)"
        ]
        
        var values: [Int] = []
        
        for pattern in patterns {
            let regex = try? NSRegularExpression(pattern: pattern)
            let range = NSRange(line.startIndex..<line.endIndex, in: line)
            
            if let match = regex?.firstMatch(in: line, options: [], range: range) {
                let matchRange = Range(match.range(at: 1), in: line)!
                let valueString = String(line[matchRange])
                values.append(Int(valueString) ?? 0)
            } else {
                values.append(0)
            }
        }
        
        guard values.count == 8 else { return nil }
        
        let charId = values[0]
        let x = Float(values[1])
        let y = Float(values[2])
        let width = Float(values[3])
        let height = Float(values[4])
        let xoffset = Float(values[5])
        let yoffset = Float(values[6])
        let xadvance = Float(values[7])
        
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
        
        return (charId, glyphInfo)
    }
    
    public func drawText(_ text: String, x: Float, y: Float, fontSize: Float, r: Float, g: Float, b: Float, a: Float) {
        guard let fontAtlas = fontAtlas,
              let fontMetrics = fontMetrics,
              let device = device,
              let samplerState = samplerState else {
            log("drawText: Font system not properly initialized", level: .warning)
            return
        }
        
        guard let renderEncoder = ensureRenderEncoder() else {
            log("drawText: Failed to get render encoder", level: .error)
            return
        }
        
        // Text rendering - no debug overlays needed
        
        // Build all vertices and indices for the entire text string in one batch
        var allVertices: [Float] = []
        var allIndices: [UInt16] = []
        var vertexCount: UInt16 = 0
        
        let scale = fontSize / fontMetrics.size
        var cursorX = x
        let cursorY = y
        
        // Render text with proper SDF scaling and baseline alignment
        
        for char in text {
            guard let glyph = glyphMap[char] else {
                log("Character '\(char)' not found in font atlas", level: .warning)
                cursorX += fontSize * 0.5 // Default advance for unknown chars
                continue
            }
            
            // Calculate glyph screen position with proper baseline alignment
            // Use the font's baseline (base) from .fnt file for consistent alignment
            let glyphX = cursorX + glyph.bearingX * scale
            let glyphY = cursorY - (fontMetrics.base - glyph.bearingY) * scale  // Correct baseline calculation
            let glyphWidth = glyph.width * scale
            let glyphHeight = glyph.height * scale
            
            // Calculate UV coordinates with proper Y-flip for Metal texture coordinates
            // The atlas shows characters in the upper portion, so we need to map correctly
            let u1 = glyph.atlasX
            let v1 = glyph.atlasY  // Don't flip - use direct atlas coordinates
            let u2 = glyph.atlasX + glyph.atlasWidth
            let v2 = glyph.atlasY + glyph.atlasHeight  // Don't flip - use direct atlas coordinates
            
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
            
            // Advance cursor with proper character spacing
            cursorX += glyph.advance * scale + 4.0  // Add 4 pixels of padding between characters
        }
        
        // Only render if we have valid characters
        guard !allVertices.isEmpty else {
            log("drawText: No valid characters found in text '\(text)'", level: .error)
            return
        }
        
        // Prepare vertex and index buffers for efficient batch rendering
        
        // Create single buffers for all characters
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
    
    private func generateFontAtlas(fontName: String, fontSize: Float) -> UIImage? {
        // First try to load from cache
        let cacheKey = "\(fontName)_\(Int(fontSize))"
        if let cachedImage = fontAtlasCache[cacheKey] {
            log("Using cached font atlas for: \(cacheKey)", level: .debug)
            return cachedImage
        }
        
        // Try to load the TTF font file from bundle
        guard let fontPath = Bundle.main.path(forResource: fontName, ofType: "ttf"),
              let fontData = NSData(contentsOfFile: fontPath),
              let dataProvider = CGDataProvider(data: fontData),
              let cgFont = CGFont(dataProvider) else {
            log("Failed to load TTF font: \(fontName), will generate simple atlas", level: .warning)
            // Generate a simple white atlas as fallback
            return generateSimpleAtlas()
        }
        
        let ctFont = CTFontCreateWithGraphicsFont(cgFont, CGFloat(fontSize), nil, nil)
        
        log("Successfully loaded TTF font: \(fontName) at size \(fontSize)", level: .debug)
        
        // Generate atlas based on the glyph metrics we parsed from .fnt
        guard let atlasImage = renderFontAtlasToImage(font: ctFont, fontSize: fontSize) else {
            log("Failed to render font atlas image, using simple atlas", level: .warning)
            return generateSimpleAtlas()
        }
        
        // Cache the generated atlas
        fontAtlasCache[cacheKey] = atlasImage
        log("Generated and cached font atlas for: \(cacheKey)", level: .debug)
        
        return atlasImage
    }
    
    private func generateSimpleAtlas() -> UIImage? {
        guard let metrics = fontMetrics else {
            log("No font metrics available for simple atlas generation", level: .error)
            return nil
        }
        
        let atlasSize = CGSize(width: CGFloat(metrics.atlasWidth), height: CGFloat(metrics.atlasHeight))
        
        // Create a simple white atlas for debugging
        UIGraphicsBeginImageContextWithOptions(atlasSize, false, 1.0)
        guard let context = UIGraphicsGetCurrentContext() else {
            log("Failed to create graphics context for simple atlas", level: .error)
            return nil
        }
        
        // Fill with white background
        context.setFillColor(UIColor.white.cgColor)
        context.fill(CGRect(origin: .zero, size: atlasSize))
        
        log("Generated simple white atlas of size \(atlasSize)", level: .debug)
        
        let atlasImage = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()
        
        return atlasImage
    }
    
    private func renderFontAtlasToImage(font: CTFont, fontSize: Float) -> UIImage? {
        guard let metrics = fontMetrics else {
            log("No font metrics available for atlas generation", level: .error)
            return nil
        }
        
        let atlasSize = CGSize(width: CGFloat(metrics.atlasWidth), height: CGFloat(metrics.atlasHeight))
        
        log("Rendering \(glyphMap.count) glyphs to SDF atlas of size \(atlasSize)", level: .debug)
        
        // Create the final SDF atlas image
        guard let sdfAtlasImage = createSDFAtlas(font: font, atlasSize: atlasSize) else {
            log("Failed to create SDF atlas", level: .error)
            return nil
        }
        
        log("Successfully generated SDF atlas with \(glyphMap.count) glyphs", level: .debug)
        return sdfAtlasImage
    }
    
    private func createSDFAtlas(font: CTFont, atlasSize: CGSize) -> UIImage? {
        // Create a bitmap context for the SDF atlas
        let colorSpace = CGColorSpaceCreateDeviceGray()
        let bytesPerPixel = 1
        let bytesPerRow = Int(atlasSize.width) * bytesPerPixel
        let bitmapInfo = CGImageAlphaInfo.none.rawValue
        
        guard let context = CGContext(
            data: nil,
            width: Int(atlasSize.width),
            height: Int(atlasSize.height),
            bitsPerComponent: 8,
            bytesPerRow: bytesPerRow,
            space: colorSpace,
            bitmapInfo: bitmapInfo
        ) else {
            log("Failed to create bitmap context for SDF atlas", level: .error)
            return nil
        }
        
        // Clear the context to black (distance = 0)
        context.setFillColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
        context.fill(CGRect(origin: .zero, size: atlasSize))
        
        // Render each glyph as SDF
        var renderedCount = 0
        for (character, glyphInfo) in glyphMap {
            if renderGlyphToSDF(
                character: character,
                glyphInfo: glyphInfo,
                font: font,
                context: context,
                atlasSize: atlasSize
            ) {
                renderedCount += 1
                
                // Debug log for first few characters
                if renderedCount <= 3 {
                    let pixelX = glyphInfo.atlasX * fontMetrics!.atlasWidth
                    let pixelY = glyphInfo.atlasY * fontMetrics!.atlasHeight
                    log("Rendered SDF '\(character)' at (\(pixelX), \(pixelY)) size (\(glyphInfo.width), \(glyphInfo.height))", level: .debug)
                }
            }
        }
        
        // Create image from context
        guard let cgImage = context.makeImage() else {
            log("Failed to create CGImage from SDF context", level: .error)
            return nil
        }
        
        let sdfImage = UIImage(cgImage: cgImage)
        log("Successfully rendered \(renderedCount) glyphs to SDF atlas", level: .debug)
        
                    // DEBUG: Save atlas to documents folder for inspection
            saveAtlasImageToDocuments(sdfImage, filename: "SDF_Atlas_Debug.png")
        
        return sdfImage
    }
    
    private func renderGlyphToSDF(
        character: Character,
        glyphInfo: GlyphInfo,
        font: CTFont,
        context: CGContext,
        atlasSize: CGSize
    ) -> Bool {
        guard let metrics = fontMetrics else { return false }
        
        // Calculate the glyph position in the atlas
        let pixelX = glyphInfo.atlasX * metrics.atlasWidth
        let pixelY = glyphInfo.atlasY * metrics.atlasHeight
        let glyphWidth = Int(glyphInfo.width)
        let glyphHeight = Int(glyphInfo.height)
        
        // Add significant padding to ensure tall characters aren't clipped
        // Make the frame substantially bigger to prevent any edge cutoff
        let extraHeight = max(12, Int(Double(glyphHeight) * 0.5)) // Add 50% more height or minimum 12 pixels
        let extraWidth = max(8, Int(Double(glyphWidth) * 0.3)) // Add 30% more width or minimum 8 pixels
        let paddedHeight = glyphHeight + extraHeight
        let paddedWidth = glyphWidth + extraWidth
        
        // Skip if glyph is too small
        guard glyphWidth > 0 && glyphHeight > 0 else { return false }
        
        // Create a high-resolution bitmap for the glyph (2x resolution for better SDF quality)
        let scale: CGFloat = 2.0
        let highResWidth = Int(CGFloat(paddedWidth) * scale) // Use padded width
        let highResHeight = Int(CGFloat(paddedHeight) * scale) // Use padded height for full character
        
        guard let highResContext = CGContext(
            data: nil,
            width: highResWidth,
            height: highResHeight,
            bitsPerComponent: 8,
            bytesPerRow: highResWidth,
            space: CGColorSpaceCreateDeviceGray(),
            bitmapInfo: CGImageAlphaInfo.none.rawValue
        ) else {
            return false
        }
        
        // Clear high-res context
        highResContext.setFillColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
        highResContext.fill(CGRect(x: 0, y: 0, width: highResWidth, height: highResHeight))
        
        // Set up text rendering
        highResContext.setFillColor(red: 1.0, green: 1.0, blue: 1.0, alpha: 1.0)
        highResContext.textMatrix = CGAffineTransform.identity
        
        // Create attributed string for the character
        let attributedString = NSAttributedString(string: String(character), attributes: [
            .font: CTFontCreateCopyWithAttributes(font, CTFontGetSize(font) * scale, nil, nil),
            .foregroundColor: UIColor.white
        ])
        
        // Draw the glyph with proper baseline positioning
        let line = CTLineCreateWithAttributedString(attributedString)
        
        // Get font metrics to center the text properly
        let ascent = CTFontGetAscent(font) * scale
        let descent = CTFontGetDescent(font) * scale
        
        // Center the text both horizontally and vertically within the padded frame
        let horizontalPadding = CGFloat(extraWidth) * scale * 0.5 // Center horizontally
        let verticalCenter = CGFloat(highResHeight) * 0.5 // Find vertical center
        let textHeight = ascent + descent
        let baselineY = verticalCenter - (textHeight * 0.5) + descent // Center the text baseline
        
        let baselineX = horizontalPadding // Center horizontally
        
        highResContext.textPosition = CGPoint(x: baselineX, y: baselineY)
        CTLineDraw(line, highResContext)
        
        // Generate SDF from the high-resolution bitmap
        // Use the original glyph dimensions for output to maintain atlas coordinates
        guard let sdfData = generateSDFFromBitmap(
            context: highResContext,
            width: highResWidth,
            height: highResHeight,
            outputWidth: glyphWidth,
            outputHeight: glyphHeight  // Keep original dimensions for atlas consistency
        ) else {
            return false
        }
        
        // Copy SDF data to the atlas
        copySDFToAtlas(
            sdfData: sdfData,
            atlasContext: context,
            atlasX: Int(pixelX),
            atlasY: Int(pixelY),
            glyphWidth: glyphWidth,
            glyphHeight: glyphHeight,
            atlasWidth: Int(atlasSize.width)
        )
        
        return true
    }
    
    private func generateSDFFromBitmap(
        context: CGContext,
        width: Int,
        height: Int,
        outputWidth: Int,
        outputHeight: Int
    ) -> [UInt8]? {
        guard let data = context.data else { return nil }
        
        let inputData = data.bindMemory(to: UInt8.self, capacity: width * height)
        var sdfData = [UInt8](repeating: 0, count: outputWidth * outputHeight)
        
        let maxDistance: Float = 8.0 // Maximum distance to search for edges
        let spread: Float = 4.0 // SDF spread parameter
        
        for y in 0..<outputHeight {
            for x in 0..<outputWidth {
                // Map output coordinates to input coordinates
                let inputX = Float(x) * Float(width) / Float(outputWidth)
                let inputY = Float(y) * Float(height) / Float(outputHeight)
                
                // Sample the input bitmap (bilinear interpolation)
                let pixelValue = sampleBitmap(inputData, width: width, height: height, x: inputX, y: inputY)
                let isInside = pixelValue > 128 // White pixels are "inside"
                
                // Find distance to nearest edge
                var minDistance: Float = maxDistance
                let searchRadius = Int(maxDistance)
                
                for dy in -searchRadius...searchRadius {
                    for dx in -searchRadius...searchRadius {
                        let testX = inputX + Float(dx)
                        let testY = inputY + Float(dy)
                        
                        if testX >= 0 && testX < Float(width) && testY >= 0 && testY < Float(height) {
                            let testPixel = sampleBitmap(inputData, width: width, height: height, x: testX, y: testY)
                            let testIsInside = testPixel > 128
                            
                            // If we found an edge (inside/outside transition)
                            if testIsInside != isInside {
                                let distance = sqrt(Float(dx * dx + dy * dy))
                                if distance < minDistance {
                                    minDistance = distance
                                }
                            }
                        }
                    }
                }
                
                // Normalize distance and apply sign
                var normalizedDistance: Float
                if isInside {
                    // Inside: distance is positive, scaled from 0.5 to 1.0
                    normalizedDistance = 0.5 + (minDistance / spread) * 0.5
                } else {
                    // Outside: distance is negative, scaled from 0.0 to 0.5
                    normalizedDistance = 0.5 - (minDistance / spread) * 0.5
                }
                
                // Clamp to [0, 1] range
                normalizedDistance = max(0.0, min(1.0, normalizedDistance))
                
                // Convert to 8-bit value
                sdfData[y * outputWidth + x] = UInt8(normalizedDistance * 255.0)
            }
        }
        
        return sdfData
    }
    
    private func sampleBitmap(_ data: UnsafeMutablePointer<UInt8>, width: Int, height: Int, x: Float, y: Float) -> UInt8 {
        let ix = Int(x)
        let iy = Int(y)
        
        if ix >= 0 && ix < width && iy >= 0 && iy < height {
            return data[iy * width + ix]
        }
        return 0 // Outside bounds = black/outside
    }
    
    private func copySDFToAtlas(
        sdfData: [UInt8],
        atlasContext: CGContext,
        atlasX: Int,
        atlasY: Int,
        glyphWidth: Int,
        glyphHeight: Int,
        atlasWidth: Int
    ) {
        guard let atlasData = atlasContext.data else { return }
        
        let atlasBytes = atlasData.bindMemory(to: UInt8.self, capacity: atlasWidth * Int(atlasContext.height))
        
        for y in 0..<glyphHeight {
            for x in 0..<glyphWidth {
                let srcIndex = y * glyphWidth + x
                let dstX = atlasX + x
                let dstY = atlasY + y
                let dstIndex = dstY * atlasWidth + dstX
                
                if srcIndex < sdfData.count && dstIndex < atlasWidth * Int(atlasContext.height) {
                    atlasBytes[dstIndex] = sdfData[srcIndex]
                }
            }
        }
    }
    
    // MARK: - Screen Size Query
    
    public func getScreenSize() -> (width: Float, height: Float) {
        return (width: Float(viewportSize.width), height: Float(viewportSize.height))
    }
    
    // MARK: - Helper Methods
    
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
    
    // MARK: - Debug Functions
    
    private func saveAtlasImageToDocuments(_ image: UIImage, filename: String) {
        guard let documentsDirectory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first else {
            log("Could not access documents directory", level: .error)
            return
        }
        
        let fileURL = documentsDirectory.appendingPathComponent(filename)
        
        if let imageData = image.pngData() {
            do {
                try imageData.write(to: fileURL)
                log("Atlas saved to: \(fileURL.path)", level: .debug)
            } catch {
                log("Failed to save atlas image: \(error)", level: .error)
            }
        }
    }
    
    // MARK: - Font Loading
}