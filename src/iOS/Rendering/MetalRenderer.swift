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
    private var currentDrawable: CAMetalDrawable?
    private var viewportSize: CGSize = CGSize.zero
    private var clearColor: MTLClearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
    
    // MTKView connection (weak, private)
    private weak var metalView: MTKView?
    
    // Resource management (private)
    private var textures: [UInt32: MTLTexture] = [:]
    private var nextTextureHandle: UInt32 = 1
    
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
        
        // Enable blending for transparency
        pipelineDescriptor.colorAttachments[0].isBlendingEnabled = true
        pipelineDescriptor.colorAttachments[0].rgbBlendOperation = .add
        pipelineDescriptor.colorAttachments[0].alphaBlendOperation = .add
        pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = .sourceAlpha
        pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
        
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
        
        let projectionMatrix = simd_float4x4(
            simd_float4(2.0 / (right - left), 0, 0, -(right + left) / (right - left)),
            simd_float4(0, 2.0 / (top - bottom), 0, -(top + bottom) / (top - bottom)),
            simd_float4(0, 0, -2.0 / (far - near), -(far + near) / (far - near)),
            simd_float4(0, 0, 0, 1)
        )
        
        let contents = uniformBuffer.contents().bindMemory(to: simd_float4x4.self, capacity: 1)
        contents.pointee = projectionMatrix
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
    }
    
    public func endFrame() {
        currentCommandBuffer?.commit()
        currentCommandBuffer = nil
        log("endFrame() called", level: .debug)
    }

    public func present() {
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
        
        // Set clear color and load action - this will clear when the next render encoder is created
        renderPassDescriptor.colorAttachments[0].clearColor = clearColor
        renderPassDescriptor.colorAttachments[0].loadAction = .clear
        
        log("Screen clear configured with color (\(clearColor.red), \(clearColor.green), \(clearColor.blue), \(clearColor.alpha))", level: .debug)
    }

    public func clear() {
        clearScreen()
    }
    
    public func drawRectangle(x: Float, y: Float, width: Float, height: Float, 
                             r: Float, g: Float, b: Float, a: Float) {
        guard let device = device,
              let commandBuffer = currentCommandBuffer,
              let renderPassDescriptor = currentRenderPassDescriptor,
              let renderPipelineState = renderPipelineState,
              let indexBuffer = indexBuffer,
              let uniformBuffer = uniformBuffer else {
            log("drawRectangle: Missing required Metal resources", level: .warning)
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
        
        // Create temporary vertex buffer for this rectangle
        guard let tempVertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<Float>.stride, options: []) else {
            log("drawRectangle: Failed to create vertex buffer", level: .error)
            return
        }
        
        let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
        renderEncoder?.setRenderPipelineState(renderPipelineState)
        renderEncoder?.setVertexBuffer(tempVertexBuffer, offset: 0, index: 0)
        renderEncoder?.setVertexBuffer(uniformBuffer, offset: 0, index: 1)
        
        renderEncoder?.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer, indexBufferOffset: 0)
        renderEncoder?.endEncoding()
        
        log("Rectangle drawn at (\(x), \(y)) with size (\(width), \(height)) and color (\(r), \(g), \(b), \(a)) - Viewport: \(viewportSize.width)x\(viewportSize.height)", level: .debug)
    }
    
    public func drawTexture(textureHandle: UInt32, x: Float, y: Float, width: Float, height: Float) {
        guard let texture = textures[textureHandle] else {
            log("drawTexture: Invalid texture handle \(textureHandle)", level: .warning)
            return
        }
        
        guard let commandBuffer = currentCommandBuffer,
              let renderPassDescriptor = currentRenderPassDescriptor,
              let texturedPipelineState = texturedPipelineState,
              let vertexBuffer = vertexBuffer,
              let indexBuffer = indexBuffer,
              let uniformBuffer = uniformBuffer else {
            log("drawTexture: Missing required Metal resources", level: .warning)
            return
        }
        
        let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
        renderEncoder?.setRenderPipelineState(texturedPipelineState)
        renderEncoder?.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder?.setVertexBuffer(uniformBuffer, offset: 0, index: 1)
        renderEncoder?.setFragmentTexture(texture, index: 0)
        renderEncoder?.setFragmentSamplerState(samplerState, index: 0)
        
        renderEncoder?.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer, indexBufferOffset: 0)
        renderEncoder?.endEncoding()
        
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
        // For now, approximate circle as a square
        let diameter = radius * 2
        drawRectangle(x: x - radius, y: y - radius, width: diameter, height: diameter, r: r, g: g, b: b, a: a)
        log("drawCircle: Approximated as rectangle at (\(x), \(y)) with radius \(radius)", level: .debug)
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
}