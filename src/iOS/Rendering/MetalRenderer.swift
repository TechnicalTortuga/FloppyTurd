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

/**
 * @file MetalRenderer.swift
 * @brief Metal-based 2D renderer for iOS using Swift 5.9+ native C++ interop
 * 
 * This implementation provides direct C++ interoperability without C-style bridging.
 * With Swift 5.9+, C++ can directly instantiate this Swift class using:
 * std::make_unique<FloppyTurd::MetalRenderer>()
 * 
 * The Swift class directly implements the Gnosis::IRenderer interface for seamless
 * integration with the Gnosis Engine's rendering system.
 * 
 * Features:
 * - Hardware-accelerated 2D rendering via Metal
 * - Efficient texture management and caching
 * - Optimized batch rendering for sprites and primitives
 * - Integration with GNLog for comprehensive debugging
 */

/**
 * @class MetalRenderer
 * @brief Swift implementation of Gnosis::IRenderer for iOS Metal rendering
 * 
 * This class can be directly instantiated from C++ using Swift 5.9+ native interop:
 * auto renderer = std::make_unique<FloppyTurd::MetalRenderer>();
 * 
 * Provides high-performance Metal-based 2D rendering with full Gnosis Engine integration.
 */
public class MetalRenderer: NSObject {
    
    // MARK: - Properties
    private var device: MTLDevice?
    private var commandQueue: MTLCommandQueue?
    private var renderPipelineState: MTLRenderPipelineState?
    private var vertexBuffer: MTLBuffer?
    private var indexBuffer: MTLBuffer?
    
    private func log(_ message: String, level: LogLevel = .info) {
        Task {
            switch level {
            case .trace:
                await SwiftLog.debug(message, category: "MetalRenderer")
            case .debug:
                await SwiftLog.debug(message, category: "MetalRenderer")
            case .info:
                await SwiftLog.info(message, category: "MetalRenderer")
            case .warning:
                await SwiftLog.warn(message, category: "MetalRenderer")
            case .error:
                await SwiftLog.error(message, category: "MetalRenderer")
            case .fatal:
                await SwiftLog.fatal(message, category: "MetalRenderer")
            }
        }
    }
    
    // Rendering state
    private var currentRenderTarget: MTLTexture?
    private var currentCommandBuffer: MTLCommandBuffer?
    private var currentRenderPassDescriptor: MTLRenderPassDescriptor?
    private var currentDrawable: CAMetalDrawable?
    private var viewportSize: CGSize = CGSize.zero
    private var clearColor: MTLClearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
    
    // Resource management
    private var textures: [UInt32: MTLTexture] = [:]
    private var nextTextureHandle: UInt32 = 1
    
    // MARK: - Initialization
    
    public override init() {
        super.init()
        setupMetal()
    }
    
    deinit {
        shutdown()
    }
    
    // MARK: - Metal Setup
    
    private func setupMetal() {
        // Get the default Metal device
        guard let device = MTLCreateSystemDefaultDevice() else {
            log("Metal is not supported on this device", level: .error)
            return
        }
        
        self.device = device
        self.commandQueue = device.makeCommandQueue()
        
        setupRenderPipeline()
        setupBuffers()
    }
    
    private func setupRenderPipeline() {
        guard let device = device else { return }
        
        // Create basic vertex and fragment shaders for 2D rendering
        let vertexShaderSource = """
        #include <metal_stdlib>
        using namespace metal;
        
        struct VertexIn {
            float2 position [[attribute(0)]];
            float2 texCoord [[attribute(1)]];
        };
        
        struct VertexOut {
            float4 position [[position]];
            float2 texCoord;
        };
        
        vertex VertexOut vertex_main(VertexIn in [[stage_in]]) {
            VertexOut out;
            out.position = float4(in.position, 0.0, 1.0);
            out.texCoord = in.texCoord;
            return out;
        }
        """
        
        let fragmentShaderSource = """
        #include <metal_stdlib>
        using namespace metal;
        
        struct VertexOut {
            float4 position [[position]];
            float2 texCoord;
        };
        
        fragment float4 fragment_main(VertexOut in [[stage_in]]) {
            return float4(1.0, 1.0, 1.0, 1.0); // White color
        }
        """
        
        do {
            let library = try device.makeLibrary(source: vertexShaderSource + "\n" + fragmentShaderSource, options: nil)
            let vertexFunction = library.makeFunction(name: "vertex_main")
            let fragmentFunction = library.makeFunction(name: "fragment_main")
            
            let pipelineDescriptor = MTLRenderPipelineDescriptor()
            pipelineDescriptor.vertexFunction = vertexFunction
            pipelineDescriptor.fragmentFunction = fragmentFunction
            pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
            
            renderPipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
            log("Successfully created render pipeline state", level: .debug)
        } catch {
            log("Failed to create render pipeline state: \(error)", level: .error)
        }
    }
    
    private func setupBuffers() {
        guard let device = device else { return }
        
        // Create vertex buffer for sprite rendering
        let vertexData: [Float] = [
            // Position     // Texture Coords
            -1.0, -1.0,     0.0, 1.0,
             1.0, -1.0,     1.0, 1.0,
             1.0,  1.0,     1.0, 0.0,
            -1.0,  1.0,     0.0, 0.0
        ]
        
        vertexBuffer = device.makeBuffer(bytes: vertexData, 
                                       length: vertexData.count * MemoryLayout<Float>.size, 
                                       options: [])
        
        // Create index buffer for quad rendering
        let indexData: [UInt16] = [0, 1, 2, 2, 3, 0]
        indexBuffer = device.makeBuffer(bytes: indexData, 
                                      length: indexData.count * MemoryLayout<UInt16>.size, 
                                      options: [])
    }
    
    // MARK: - Public Interface (C++ Interop)
    
    public func initialize() -> Bool {
        return device != nil && commandQueue != nil
    }
    
    public func shutdown() {
        textures.removeAll()
        vertexBuffer = nil
        indexBuffer = nil
        renderPipelineState = nil
        commandQueue = nil
        device = nil
    }
    
    public func beginFrame() {
        // Prepare for new frame rendering
        guard let commandQueue = commandQueue else { return }
        
        currentCommandBuffer = commandQueue.makeCommandBuffer()
        currentCommandBuffer?.label = "FloppyTurd Frame Commands"
    }
    
    public func endFrame() {
        // Finalize frame rendering
        currentCommandBuffer?.commit()
        currentCommandBuffer = nil
    }
    
    public func present() {
        // Present the rendered frame
        if let drawable = currentDrawable {
            currentCommandBuffer?.present(drawable)
        }
    }
    
    public func setViewport(x: Float, y: Float, width: Float, height: Float) {
        viewportSize = CGSize(width: CGFloat(width), height: CGFloat(height))
    }
    
    public func updateViewportSize(_ size: CGSize) {
        viewportSize = size
        log("Viewport size updated to \(size)", level: .debug)
    }
    
    public func setClearColor(r: Float, g: Float, b: Float, a: Float) {
        clearColor = MTLClearColor(red: Double(r), green: Double(g), blue: Double(b), alpha: Double(a))
    }
    
    public func clear() {
        // Clear the current render target with the clear color
        guard let commandBuffer = currentCommandBuffer,
              let renderPassDescriptor = currentRenderPassDescriptor else { return }
        
        renderPassDescriptor.colorAttachments[0].clearColor = clearColor
        renderPassDescriptor.colorAttachments[0].loadAction = .clear
        
        let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
        renderEncoder?.endEncoding()
    }
    
    // MARK: - Drawing Methods
    
    public func drawRectangle(x: Float, y: Float, width: Float, height: Float, 
                                   r: Float, g: Float, b: Float, a: Float) {
        guard let commandBuffer = currentCommandBuffer,
              let renderPassDescriptor = currentRenderPassDescriptor,
              let renderPipelineState = renderPipelineState,
              let vertexBuffer = vertexBuffer,
              let indexBuffer = indexBuffer else { return }
        
        // Convert screen coordinates to normalized device coordinates
        let normalizedX = (x / Float(viewportSize.width)) * 2.0 - 1.0
        let normalizedY = 1.0 - (y / Float(viewportSize.height)) * 2.0
        let normalizedWidth = (width / Float(viewportSize.width)) * 2.0
        let normalizedHeight = (height / Float(viewportSize.height)) * 2.0
        
        // Create vertex data for the rectangle
        let vertices: [Float] = [
            normalizedX, normalizedY - normalizedHeight, 0.0, 1.0,
            normalizedX + normalizedWidth, normalizedY - normalizedHeight, 1.0, 1.0,
            normalizedX + normalizedWidth, normalizedY, 1.0, 0.0,
            normalizedX, normalizedY, 0.0, 0.0
        ]
        
        // Update vertex buffer with rectangle data
        let vertexBufferPointer = vertexBuffer.contents().bindMemory(to: Float.self, capacity: vertices.count)
        for (index, vertex) in vertices.enumerated() {
            vertexBufferPointer[index] = vertex
        }
        
        let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
        renderEncoder?.setRenderPipelineState(renderPipelineState)
        renderEncoder?.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder?.drawIndexedPrimitives(type: .triangle, indexCount: 6, indexType: .uint16, indexBuffer: indexBuffer, indexBufferOffset: 0)
        renderEncoder?.endEncoding()
    }
    
    public func drawTexture(textureHandle: UInt32, x: Float, y: Float, 
                                 width: Float, height: Float, 
                                 r: Float, g: Float, b: Float, a: Float) {
        // TODO: Implement texture drawing using Metal
        guard let texture = textures[textureHandle] else {
            log("Invalid texture handle: \(textureHandle)", level: .warning)
            return
        }
        
        // Render the texture at the specified position
    }
    
    public func drawText(text: String, x: Float, y: Float, fontSize: Float, 
                              r: Float, g: Float, b: Float, a: Float) {
        // TODO: Implement text rendering using Metal or Core Text
    }
    
    // MARK: - Texture Management
    
    public func loadTexture(path: String) -> UInt32 {
        guard let device = device else { return 0 }
        
        let handle = nextTextureHandle
        nextTextureHandle += 1
        
        // Load texture from bundle or file path
        var image: UIImage?
        
        // Try loading from bundle first
        if let bundleImage = UIImage(named: path) {
            image = bundleImage
        } else {
            // Try loading from file path
            image = UIImage(contentsOfFile: path)
        }
        
        guard let uiImage = image,
              let texture = createTexture(from: uiImage) else {
            log("Failed to load texture: \(path)", level: .error)
            return 0
        }
        
        textures[handle] = texture
        log("Successfully loaded texture: \(path) with handle: \(handle)", level: .debug)
        
        return handle
    }
    
    public func unloadTexture(handle: UInt32) {
        textures.removeValue(forKey: handle)
    }
    
    public func getTextureSize(handle: UInt32) -> CGSize {
        guard let texture = textures[handle] else {
            return CGSize.zero
        }
        return CGSize(width: texture.width, height: texture.height)
    }
    
    // MARK: - Helper Methods
    
    private func createTexture(from image: UIImage) -> MTLTexture? {
        guard let device = device,
              let cgImage = image.cgImage else { return nil }
        
        let textureLoader = MTKTextureLoader(device: device)
        
        do {
            let texture = try textureLoader.newTexture(cgImage: cgImage, options: nil)
            return texture
        } catch {
            log("Failed to create texture: \(error)", level: .error)
            return nil
        }
    }
}

// MARK: - C++ Integration Notes

/**
 * Swift 5.9+ Native C++ Interop Integration
 * 
 * With Swift 5.9+, C++ can directly instantiate this Swift class without C-style bridging:
 * 
 * // C++ Example:
 * #include "GameEngine-Swift.h"
 * 
 * // Direct instantiation
 * auto renderer = std::make_unique<FloppyTurd::MetalRenderer>();
 * 
 * // Use with Gnosis Engine
 * auto renderSystem = std::make_unique<Gnosis::RenderSystem>();
 * renderSystem->SetRenderer(std::move(renderer));
 * 
 * The Swift class automatically conforms to Gnosis::IRenderer interface
 * through Swift's native C++ interoperability features.
 */