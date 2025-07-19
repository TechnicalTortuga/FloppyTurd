//
//  MetalRendererSwift.swift
//  Professional Game Engine - Native Metal Renderer
//
//  Created by C++ Swift Interop Migration
//  Direct Swift replacement for MetalRenderer.mm with zero overhead
//

import Foundation
import Metal
import MetalKit
import simd

/// Professional Metal renderer implementation in pure Swift
/// Direct replacement for MetalRenderer.mm with native C++ interop
public class MetalRendererSwift: @unchecked Sendable {
    
    // MARK: - Shared Instance
    nonisolated(unsafe) public static let shared = MetalRendererSwift()
    
    /// Update the viewport and projection matrix for a new drawable size
    public func updateViewport(size: CGSize) {
        print("[MetalRendererSwift] updateViewport: size=\(size)")
        setProjectionMatrix(width: Float(size.width), height: Float(size.height))
    }
    
    // MARK: - Metal Resources
    private var device: MTLDevice?
    private var commandQueue: MTLCommandQueue?
    private var view: MTKView?
    
    // Pipeline states
    private var texturePipeline: MTLRenderPipelineState?
    private var colorPipeline: MTLRenderPipelineState?
    private var sdfPipeline: MTLRenderPipelineState?
    private var instancedTexturePipeline: MTLRenderPipelineState?
    private var instancedColorPipeline: MTLRenderPipelineState?
    
    // Depth and sampler states
    private var depthStencilState: MTLDepthStencilState?
    private var uiDepthStencilState: MTLDepthStencilState?
    private var samplerState: MTLSamplerState?
    
    // Buffers
    private var vertexBuffer: MTLBuffer?
    private var uniformBuffer: MTLBuffer?
    
    // Rendering state
    private var currentCommandBuffer: MTLCommandBuffer?
    private var currentEncoder: MTLRenderCommandEncoder?
    private var currentTexture: MTLTexture?
    
    // Vertex batching
    private var vertices: [MetalVertex2D] = []
    private var drawCommands: [DrawCommand] = []
    
    // Performance tracking
    private var debugStats = DebugStats()
    private var frameStartTime: CFTimeInterval = 0
    private let targetFrameTime: Float = 1.0/60.0
    
    // Frame resources (we'll need to create a Swift equivalent)
    private var frameResources: MetalFrameResourcesSwift?
    
    // State tracking
    private var projectionMatrix = matrix_identity_float4x4
    private var currentVertexBufferOffset: Int = 0
    private var isPaused: Bool = false
    
    // MARK: - Swift Structs (equivalent to C++ structs)
    
    public struct MetalVertex2D {
        var position: simd_float2
        var texCoords: simd_float2
        var color: simd_float4
    }
    
    public struct DrawCommand {
        var primitiveType: MTLPrimitiveType = .triangle
        var vertexStart: Int = 0
        var vertexCount: Int = 0
        var texture: MTLTexture?
        var useTexture: Bool = false
        var renderState: UInt32 = 0
        var textureId: UInt32 = 0
        var depth: Float = 0.0
        var sortKey: UInt32 = 0
        var instanceCount: Int = 1
        var instanceDataOffset: Int = 0
        var debugName: String = ""
    }
    
    public struct DebugStats {
        var batchedVertices: Int = 0
        var stateChanges: Int = 0
        var textureBinds: Int = 0
        var instancedCalls: Int = 0
    }
    
    public enum RenderLayer: Int {
        case background = 0
        case midground = 1
        case foreground = 2
        case ui = 3
        case text = 4
    }
    
    // Render state constants
    public static let RENDER_STATE_ALPHA_BLEND: UInt32 = 0x01
    public static let RENDER_STATE_DEPTH_TEST: UInt32 = 0x02
    public static let RENDER_STATE_SDF: UInt32 = 0x04
    
    // MARK: - Initialization
    
    public init() {
        print("[MetalRendererSwift] Initializing Swift Metal renderer")
    }
    
    deinit {
        print("[MetalRendererSwift] Destroying Swift Metal renderer")
        shutdown()
    }
    
    /// Initialize the Metal renderer with an MTKView
    /// Direct equivalent of MetalRenderer::Initialize
    @MainActor
    public func initialize(view: MTKView) -> Bool {
        print("[MetalRendererSwift] Initialize START")
        
        self.view = view
        self.device = view.device
        
        guard let device = self.device else {
            print("[MetalRendererSwift] ERROR: Failed to get Metal device")
            return false
        }
        
        print("[MetalRendererSwift] Device: \(device)")
        
        // Create command queue
        commandQueue = device.makeCommandQueue()
        guard commandQueue != nil else {
            print("[MetalRendererSwift] ERROR: Failed to create command queue")
            return false
        }
        
        // Configure view
        view.colorPixelFormat = .bgra8Unorm
        view.depthStencilPixelFormat = .depth32Float
        view.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0)
        
        // Initialize frame resources
        frameResources = MetalFrameResourcesSwift()
        guard let frameResources = frameResources else {
            print("[MetalRendererSwift] ERROR: Failed to create frame resources")
            return false
        }
        
        if !frameResources.initialize(device: device) {
            print("[MetalRendererSwift] ERROR: Failed to initialize frame resources")
            return false
        }
        
        // Create pipelines
        if !createPipelines() {
            print("[MetalRendererSwift] ERROR: Failed to create pipelines")
            return false
        }
        
        // Create buffers
        if !createBuffers() {
            print("[MetalRendererSwift] ERROR: Failed to create buffers")
            return false
        }
        
        // Create sampler state
        createSamplerState()
        
        // Set up projection matrix
        let screenRect = UICoordinateSystem.getPixelScreenRect()
        setProjectionMatrix(width: screenRect.width, height: screenRect.height)
        
        print("[MetalRendererSwift] ✅ Metal renderer initialized successfully")
        return true
    }
    
    /// Shutdown the renderer and release resources
    public func shutdown() {
        print("[MetalRendererSwift] Shutting down")
        
        // Flush any pending draw calls
        flushBatch()
        
        // Release resources
        releaseResources()
        
        print("[MetalRendererSwift] ✅ Shutdown complete")
    }
    
    // MARK: - Platform API Implementation (Raylib-style functions)
    
    /// Begin drawing a frame - equivalent to BeginDrawing()
    @MainActor public func beginDrawing() {
        beginFrame()
        print("[MetalRendererSwift] BeginDrawing: Frame started")
    }
    
    /// End drawing a frame - equivalent to EndDrawing()  
    @MainActor public func endDrawing() {
        endFrame()
        print("[MetalRendererSwift] EndDrawing: Frame ended")
    }
    
    /// Clear the background - equivalent to ClearBackground()
    @MainActor public func clearBackground(_ color: RaylibColor) {
        clear(color)
        print("[MetalRendererSwift] ClearBackground: Set clear color to (\(color.r),\(color.g),\(color.b),\(color.a))")
    }
    
    /// Draw a rectangle - equivalent to DrawRectangle()
    public func drawRectangle(x: Float, y: Float, width: Float, height: Float, color: RaylibColor) {
        print("[MetalRendererSwift] DrawRectangle: (\(x),\(y),\(width),\(height)), color=(\(color.r),\(color.g),\(color.b),\(color.a))")
        
        // Create vertices for a rectangle (two triangles)
        let vertices = [
            MetalVertex2D(position: simd_float2(x, y), 
                         texCoords: simd_float2(0, 0), 
                         color: simd_float4(Float(color.r)/255.0, Float(color.g)/255.0, Float(color.b)/255.0, Float(color.a)/255.0)),
            MetalVertex2D(position: simd_float2(x + width, y), 
                         texCoords: simd_float2(1, 0), 
                         color: simd_float4(Float(color.r)/255.0, Float(color.g)/255.0, Float(color.b)/255.0, Float(color.a)/255.0)),
            MetalVertex2D(position: simd_float2(x, y + height), 
                         texCoords: simd_float2(0, 1), 
                         color: simd_float4(Float(color.r)/255.0, Float(color.g)/255.0, Float(color.b)/255.0, Float(color.a)/255.0)),
            MetalVertex2D(position: simd_float2(x + width, y + height), 
                         texCoords: simd_float2(1, 1), 
                         color: simd_float4(Float(color.r)/255.0, Float(color.g)/255.0, Float(color.b)/255.0, Float(color.a)/255.0))
        ]
        
        // Add vertices to batch
        addVertices(vertices, texture: nil, renderState: Self.RENDER_STATE_ALPHA_BLEND)
    }

    /// Draw a rectangle with rounded corners - equivalent to DrawRectangleRounded()
    public func drawRectangleRounded(rec: Rectangle, roundness: Float, segments: Int, color: RaylibColor) {
        print("[MetalRendererSwift] TRACE: drawRectangleRounded - rec: \(rec), roundness: \(roundness), segments: \(segments)")

        let colorVec = simd_float4(Float(color.r)/255.0, Float(color.g)/255.0, Float(color.b)/255.0, Float(color.a)/255.0)
        let x = rec.x
        let y = rec.y
        let width = rec.width
        let height = rec.height

        // Ensure roundness is not larger than half the rectangle's smaller side
        let radius = min(min(abs(width), abs(height)) / 2, roundness)

        // Don't draw if the rectangle is too small for the radius
        if width < radius * 2 || height < radius * 2 {
            drawRectangle(x: x, y: y, width: width, height: height, color: color)
            return
        }

        var roundedVertices: [MetalVertex2D] = []

        // Center rectangle (unused variable removed)
        
        // Top rectangle
        roundedVertices.append(contentsOf: createQuad(x: x + radius, y: y, width: width - 2 * radius, height: radius, color: colorVec))
        // Bottom rectangle
        roundedVertices.append(contentsOf: createQuad(x: x + radius, y: y + height - radius, width: width - 2 * radius, height: radius, color: colorVec))
        // Left rectangle
        roundedVertices.append(contentsOf: createQuad(x: x, y: y + radius, width: radius, height: height - 2 * radius, color: colorVec))
        // Right rectangle
        roundedVertices.append(contentsOf: createQuad(x: x + width - radius, y: y + radius, width: radius, height: height - 2 * radius, color: colorVec))
        // Center rectangle
        roundedVertices.append(contentsOf: createQuad(x: x + radius, y: y + radius, width: width - 2 * radius, height: height - 2 * radius, color: colorVec))

        // Corners
        let segments = max(4, segments) // Minimum 4 segments for a decent circle
        // Top-Left
        roundedVertices.append(contentsOf: createTriangleFan(centerX: x + radius, centerY: y + radius, radius: radius, startAngle: .pi, endAngle: .pi * 1.5, segments: segments, color: colorVec))
        // Top-Right
        roundedVertices.append(contentsOf: createTriangleFan(centerX: x + width - radius, centerY: y + radius, radius: radius, startAngle: .pi * 1.5, endAngle: .pi * 2, segments: segments, color: colorVec))
        // Bottom-Left
        roundedVertices.append(contentsOf: createTriangleFan(centerX: x + radius, centerY: y + height - radius, radius: radius, startAngle: .pi * 0.5, endAngle: .pi, segments: segments, color: colorVec))
        // Bottom-Right
        roundedVertices.append(contentsOf: createTriangleFan(centerX: x + width - radius, centerY: y + height - radius, radius: radius, startAngle: 0, endAngle: .pi * 0.5, segments: segments, color: colorVec))

        addVertices(roundedVertices, texture: nil, renderState: Self.RENDER_STATE_ALPHA_BLEND)
    }

    /// Draw the lines of a rectangle with rounded corners
    public func drawRectangleRoundedLines(rec: Rectangle, roundness: Float, segments: Int, lineThick: Float, color: RaylibColor) {
        print("[MetalRendererSwift] TRACE: drawRectangleRoundedLines - rec: \(rec), roundness: \(roundness), segments: \(segments), lineThick: \(lineThick)")

        let x = rec.x
        let y = rec.y
        let width = rec.width
        let height = rec.height
        let radius = min(min(abs(width), abs(height)) / 2, roundness)

        if lineThick <= 0 { return }
        if width < radius * 2 || height < radius * 2 {
            // Fallback to a simple rectangle outline if it's too small for rounding
            drawRectangleLines(x: x, y: y, width: width, height: height, thickness: lineThick, color: color)
            return
        }

        let segments = max(4, segments)

        // Straight line segments
        // Top
        drawLineEx(startX: x + radius, startY: y, endX: x + width - radius, endY: y, thickness: lineThick, color: color)
        // Bottom
        drawLineEx(startX: x + radius, startY: y + height, endX: x + width - radius, endY: y + height, thickness: lineThick, color: color)
        // Left
        drawLineEx(startX: x, startY: y + radius, endX: x, endY: y + height - radius, thickness: lineThick, color: color)
        // Right
        drawLineEx(startX: x + width, startY: y + radius, endX: x + width, endY: y + height - radius, thickness: lineThick, color: color)

        // Rounded corners as line segments
        // Top-Left
        drawArcLines(centerX: x + radius, centerY: y + radius, radius: radius, startAngle: 180, endAngle: 270, segments: segments, thickness: lineThick, color: color)
        // Top-Right
        drawArcLines(centerX: x + width - radius, centerY: y + radius, radius: radius, startAngle: 270, endAngle: 360, segments: segments, thickness: lineThick, color: color)
        // Bottom-Left
        drawArcLines(centerX: x + radius, centerY: y + height - radius, radius: radius, startAngle: 90, endAngle: 180, segments: segments, thickness: lineThick, color: color)
        // Bottom-Right
        drawArcLines(centerX: x + width - radius, centerY: y + height - radius, radius: radius, startAngle: 0, endAngle: 90, segments: segments, thickness: lineThick, color: color)
    }
    
    /// Draw a circle - equivalent to DrawCircle()
    public func drawCircle(centerX: Float, centerY: Float, radius: Float, color: RaylibColor) {
        print("[MetalRendererSwift] DrawCircle: center=(\(centerX),\(centerY)), radius=\(radius)")
        
        let segments = 32
        let colorVec = simd_float4(Float(color.r)/255.0, Float(color.g)/255.0, Float(color.b)/255.0, Float(color.a)/255.0)
        
        // Center vertex
        var circleVertices: [MetalVertex2D] = []
        circleVertices.append(MetalVertex2D(position: simd_float2(centerX, centerY), 
                                          texCoords: simd_float2(0.5, 0.5), 
                                          color: colorVec))
        
        // Create vertices around the circle
        for i in 0...segments {
            let angle = Float(i) * 2.0 * Float.pi / Float(segments)
            let x = centerX + cos(angle) * radius
            let y = centerY + sin(angle) * radius
            
            circleVertices.append(MetalVertex2D(position: simd_float2(x, y), 
                                              texCoords: simd_float2(0.5, 0.5), 
                                              color: colorVec))
        }
        
        addVertices(circleVertices, texture: nil, renderState: Self.RENDER_STATE_ALPHA_BLEND)
    }
    
    /// Draw a texture with extended parameters - equivalent to DrawTexturePro()
    public func drawTexturePro(textureId: Int32, source: Rectangle, dest: Rectangle, origin: Vector2, rotation: Float, tint: RaylibColor) {
        print("[MetalRendererSwift] DrawTexturePro: textureId=\(textureId), source=\(source), dest=\(dest), origin=\(origin), rotation=\(rotation)")
        
        // For now, we'll implement a basic version that doesn't use the textureId
        // In a full implementation, you'd look up the texture by ID
        guard let texture = currentTexture else {
            print("[MetalRendererSwift] WARNING: DrawTexturePro called but no current texture set")
            return
        }
        
        // Calculate UV coordinates
        let texWidth = Float(texture.width)
        let texHeight = Float(texture.height)
        let u0 = source.x / texWidth
        let v0 = source.y / texHeight
        let u1 = (source.x + source.width) / texWidth
        let v1 = (source.y + source.height) / texHeight
        
        // Vertex positions relative to dest rect
        let ox = origin.x
        let oy = origin.y
        let w = dest.width
        let h = dest.height
        
        var positions: [simd_float2] = [
            simd_float2(0 - ox, 0 - oy),
            simd_float2(w - ox, 0 - oy),
            simd_float2(0 - ox, h - oy),
            simd_float2(w - ox, h - oy)
        ]
        
        // Apply rotation
        let cosRot = cos(rotation * Float.pi / 180.0)
        let sinRot = sin(rotation * Float.pi / 180.0)
        for i in 0..<4 {
            let x = positions[i].x
            let y = positions[i].y
            positions[i].x = x * cosRot - y * sinRot
            positions[i].y = x * sinRot + y * cosRot
            positions[i].x += dest.x
            positions[i].y += dest.y
        }
        
        let colorVec = simd_float4(Float(tint.r)/255.0, Float(tint.g)/255.0, Float(tint.b)/255.0, Float(tint.a)/255.0)
        
        let vertices = [
            MetalVertex2D(position: positions[0], texCoords: simd_float2(u0, v0), color: colorVec),
            MetalVertex2D(position: positions[1], texCoords: simd_float2(u1, v0), color: colorVec),
            MetalVertex2D(position: positions[2], texCoords: simd_float2(u0, v1), color: colorVec),
            MetalVertex2D(position: positions[3], texCoords: simd_float2(u1, v1), color: colorVec)
        ]
        
        addVertices(vertices, texture: texture, renderState: Self.RENDER_STATE_ALPHA_BLEND)
    }
    
    /// Draw a line - equivalent to DrawLine()
    public func drawLine(startX: Float, startY: Float, endX: Float, endY: Float, color: RaylibColor) {
        drawLineEx(startX: startX, startY: startY, endX: endX, endY: endY, thickness: 1.0, color: color)
    }
    
    /// Draw a line with thickness - equivalent to DrawLineEx()
    public func drawLineEx(startX: Float, startY: Float, endX: Float, endY: Float, thickness: Float, color: RaylibColor) {
        print("[MetalRendererSwift] DrawLineEx: (\(startX),\(startY)) to (\(endX),\(endY)), thickness=\(thickness)")
        
        // Calculate perpendicular vector for thickness
        let dx = endX - startX
        let dy = endY - startY
        let length = sqrt(dx * dx + dy * dy)
        
        guard length > 0 else {
            print("[MetalRendererSwift] WARNING: Zero length line, skipping")
            return
        }
        
        // Normalize and get perpendicular
        let nx = -dy / length * thickness * 0.5
        let ny = dx / length * thickness * 0.5
        
        let colorVec = simd_float4(Float(color.r)/255.0, Float(color.g)/255.0, Float(color.b)/255.0, Float(color.a)/255.0)
        
        // Four corners of the line rectangle
        let vertices = [
            MetalVertex2D(position: simd_float2(startX + nx, startY + ny), texCoords: simd_float2(0.5, 0.5), color: colorVec),
            MetalVertex2D(position: simd_float2(startX - nx, startY - ny), texCoords: simd_float2(0.5, 0.5), color: colorVec),
            MetalVertex2D(position: simd_float2(endX + nx, endY + ny), texCoords: simd_float2(0.5, 0.5), color: colorVec),
            MetalVertex2D(position: simd_float2(endX - nx, endY - ny), texCoords: simd_float2(0.5, 0.5), color: colorVec)
        ]
        
        addVertices(vertices, texture: nil, renderState: Self.RENDER_STATE_ALPHA_BLEND)
    }
    
    /// Draw a texture - equivalent to DrawTexture()
    public func drawTexture(_ texture: MTLTexture?, source: Rectangle, dest: Rectangle, tint: RaylibColor, layer: RenderLayer = .ui) {
        print("[MetalRendererSwift] DrawTexture: source=(\(source.x),\(source.y),\(source.width),\(source.height)), dest=(\(dest.x),\(dest.y),\(dest.width),\(dest.height))")
        
        guard let texture = texture else {
            print("[MetalRendererSwift] WARNING: DrawTexture called with nil texture")
            return
        }
        
        currentTexture = texture
        
        // Calculate UV coordinates from source rectangle
        let texWidth = Float(texture.width)
        let texHeight = Float(texture.height)
        let u1 = source.x / texWidth
        let v1 = source.y / texHeight
        let u2 = (source.x + source.width) / texWidth
        let v2 = (source.y + source.height) / texHeight
        
        let colorVec = simd_float4(Float(tint.r)/255.0, Float(tint.g)/255.0, Float(tint.b)/255.0, Float(tint.a)/255.0)
        
        let vertices = [
            MetalVertex2D(position: simd_float2(dest.x, dest.y), texCoords: simd_float2(u1, v1), color: colorVec),
            MetalVertex2D(position: simd_float2(dest.x + dest.width, dest.y), texCoords: simd_float2(u2, v1), color: colorVec),
            MetalVertex2D(position: simd_float2(dest.x, dest.y + dest.height), texCoords: simd_float2(u1, v2), color: colorVec),
            MetalVertex2D(position: simd_float2(dest.x + dest.width, dest.y + dest.height), texCoords: simd_float2(u2, v2), color: colorVec)
        ]
        
        addVertices(vertices, texture: texture, renderState: Self.RENDER_STATE_ALPHA_BLEND)
        
        // Create draw command
        var cmd = DrawCommand()
        cmd.primitiveType = .triangleStrip
        cmd.vertexStart = self.vertices.count - 4
        cmd.vertexCount = 4
        cmd.texture = texture
        cmd.useTexture = true
        cmd.renderState = Self.RENDER_STATE_ALPHA_BLEND
        cmd.depth = Float(layer.rawValue) * 0.1
        cmd.debugName = layerName(for: layer)
        
        drawCommands.append(cmd)
    }
    
    // MARK: - Internal Implementation
    
    @MainActor
    private func createPipelines() -> Bool {
        guard let device = device else { return false }
        
        print("[MetalRendererSwift] Creating pipelines...")
        
        // Load shader library
        guard let library = loadShaderLibrary() else {
            print("[MetalRendererSwift] ERROR: Failed to load shader library")
            return false
        }
        
        // Get shader functions
        guard let vertexFunction = library.makeFunction(name: "vertex_shader_2d_simple"),
              let fragmentTexturedFunction = library.makeFunction(name: "fragment_shader_textured"),
              let fragmentColorFunction = library.makeFunction(name: "fragment_shader_color"),
              let fragmentSdfFunction = library.makeFunction(name: "fragment_shader_sdf") else {
            print("[MetalRendererSwift] ERROR: Failed to load shader functions")
            return false
        }
        
        // Create vertex descriptor
        let vertexDescriptor = MTLVertexDescriptor()
        
        // Position (float2)
        vertexDescriptor.attributes[0].format = .float2
        vertexDescriptor.attributes[0].offset = 0
        vertexDescriptor.attributes[0].bufferIndex = 0
        
        // Texture coordinates (float2)
        vertexDescriptor.attributes[1].format = .float2
        vertexDescriptor.attributes[1].offset = 8
        vertexDescriptor.attributes[1].bufferIndex = 0
        
        // Color (float4)
        vertexDescriptor.attributes[2].format = .float4
        vertexDescriptor.attributes[2].offset = 16
        vertexDescriptor.attributes[2].bufferIndex = 0
        
        vertexDescriptor.layouts[0].stride = MemoryLayout<MetalVertex2D>.stride
        vertexDescriptor.layouts[0].stepRate = 1
        vertexDescriptor.layouts[0].stepFunction = .perVertex
        
        // Create textured pipeline
        let texturedPipelineDesc = MTLRenderPipelineDescriptor()
        texturedPipelineDesc.label = "Textured Pipeline"
        texturedPipelineDesc.vertexFunction = vertexFunction
        texturedPipelineDesc.fragmentFunction = fragmentTexturedFunction
        texturedPipelineDesc.vertexDescriptor = vertexDescriptor
        texturedPipelineDesc.colorAttachments[0].pixelFormat = view?.colorPixelFormat ?? .bgra8Unorm
        
        // Set up blending
        setupBlending(attachment: texturedPipelineDesc.colorAttachments[0]!)
        texturedPipelineDesc.depthAttachmentPixelFormat = view?.depthStencilPixelFormat ?? .depth32Float
        
        do {
            texturePipeline = try device.makeRenderPipelineState(descriptor: texturedPipelineDesc)
        } catch {
            print("[MetalRendererSwift] ERROR: Failed to create textured pipeline: \(error)")
            return false
        }
        
        // Create color pipeline
        let colorPipelineDesc = MTLRenderPipelineDescriptor()
        colorPipelineDesc.label = "Color Pipeline"
        colorPipelineDesc.vertexFunction = vertexFunction
        colorPipelineDesc.fragmentFunction = fragmentColorFunction
        colorPipelineDesc.vertexDescriptor = vertexDescriptor
        colorPipelineDesc.colorAttachments[0].pixelFormat = view?.colorPixelFormat ?? .bgra8Unorm
        setupBlending(attachment: colorPipelineDesc.colorAttachments[0]!)
        colorPipelineDesc.depthAttachmentPixelFormat = view?.depthStencilPixelFormat ?? .depth32Float
        
        do {
            colorPipeline = try device.makeRenderPipelineState(descriptor: colorPipelineDesc)
        } catch {
            print("[MetalRendererSwift] ERROR: Failed to create color pipeline: \(error)")
            return false
        }
        
        // Create SDF pipeline
        let sdfPipelineDesc = MTLRenderPipelineDescriptor()
        sdfPipelineDesc.label = "SDF Pipeline"
        sdfPipelineDesc.vertexFunction = vertexFunction
        sdfPipelineDesc.fragmentFunction = fragmentSdfFunction
        sdfPipelineDesc.vertexDescriptor = vertexDescriptor
        sdfPipelineDesc.colorAttachments[0].pixelFormat = view?.colorPixelFormat ?? .bgra8Unorm
        setupBlending(attachment: sdfPipelineDesc.colorAttachments[0]!)
        sdfPipelineDesc.depthAttachmentPixelFormat = view?.depthStencilPixelFormat ?? .depth32Float
        
        do {
            sdfPipeline = try device.makeRenderPipelineState(descriptor: sdfPipelineDesc)
        } catch {
            print("[MetalRendererSwift] ERROR: Failed to create SDF pipeline: \(error)")
            return false
        }
        
        // Create depth stencil states
        let depthDesc = MTLDepthStencilDescriptor()
        depthDesc.depthCompareFunction = .lessEqual
        depthDesc.isDepthWriteEnabled = true
        depthStencilState = device.makeDepthStencilState(descriptor: depthDesc)
        
        let uiDepthDesc = MTLDepthStencilDescriptor()
        uiDepthDesc.depthCompareFunction = .always
        uiDepthDesc.isDepthWriteEnabled = false
        uiDepthStencilState = device.makeDepthStencilState(descriptor: uiDepthDesc)
        
        print("[MetalRendererSwift] ✅ Pipelines created successfully")
        return true
    }
    
    private func setupBlending(attachment: MTLRenderPipelineColorAttachmentDescriptor) {
        attachment.isBlendingEnabled = true
        attachment.rgbBlendOperation = .add
        attachment.alphaBlendOperation = .add
        attachment.sourceRGBBlendFactor = .sourceAlpha
        attachment.sourceAlphaBlendFactor = .sourceAlpha
        attachment.destinationRGBBlendFactor = .oneMinusSourceAlpha
        attachment.destinationAlphaBlendFactor = .oneMinusSourceAlpha
    }
    
    private func loadShaderLibrary() -> MTLLibrary? {
        guard let device = device else { return nil }
        
        // Try default library first
        if let library = device.makeDefaultLibrary() {
            print("[MetalRendererSwift] Loaded default shader library")
            return library
        }
        
        // Try loading from bundle
        let possiblePaths = [
            "Shaders2D",
            "FloppyTurd/Shaders2D",
            "FloppyTurd/resources/Shaders2D",
            "resources/Shaders2D"
        ]
        
        for path in possiblePaths {
            if let shaderPath = Bundle.main.path(forResource: path, ofType: "metal"),
               let shaderSource = try? String(contentsOfFile: shaderPath) {
                
                print("[MetalRendererSwift] Found shader file at: \(shaderPath)")
                
                let options = MTLCompileOptions()
                options.fastMathEnabled = true
                if #available(iOS 15.0, *) {
                    options.languageVersion = .version2_4
                }
                
                do {
                    let library = try device.makeLibrary(source: shaderSource, options: options)
                    print("[MetalRendererSwift] Successfully compiled shader library from source")
                    return library
                } catch {
                    print("[MetalRendererSwift] ERROR: Failed to compile shader library: \(error)")
                }
            }
        }
        
        print("[MetalRendererSwift] ERROR: Shaders2D.metal file not found")
        return nil
    }
    
    private func createBuffers() -> Bool {
        guard let device = device else { return false }
        
        print("[MetalRendererSwift] Creating buffers...")
        
        // Create vertex buffer
        let initialVertexBufferSize = 1024 * MemoryLayout<MetalVertex2D>.stride
        vertexBuffer = device.makeBuffer(length: initialVertexBufferSize, options: .storageModeShared)
        vertexBuffer?.label = "Vertex Buffer"
        
        guard vertexBuffer != nil else {
            print("[MetalRendererSwift] ERROR: Failed to create vertex buffer")
            return false
        }
        
        // Create uniform buffer
        uniformBuffer = device.makeBuffer(length: MemoryLayout<simd_float4x4>.stride, options: .storageModeShared)
        uniformBuffer?.label = "Uniform Buffer"
        
        guard let uniformBuffer = uniformBuffer else {
            print("[MetalRendererSwift] ERROR: Failed to create uniform buffer")
            return false
        }
        
        // Initialize uniform buffer with identity matrix
        let uniforms = uniformBuffer.contents().bindMemory(to: simd_float4x4.self, capacity: 1)
        uniforms.pointee = matrix_identity_float4x4
        
        print("[MetalRendererSwift] ✅ Buffers created successfully")
        return true
    }
    
    private func createSamplerState() {
        guard let device = device else { return }
        
        let samplerDesc = MTLSamplerDescriptor()
        samplerDesc.minFilter = .nearest
        samplerDesc.magFilter = .nearest
        samplerDesc.mipFilter = .nearest
        samplerDesc.sAddressMode = .clampToEdge
        samplerDesc.tAddressMode = .clampToEdge
        
        samplerState = device.makeSamplerState(descriptor: samplerDesc)
        print("[MetalRendererSwift] ✅ Sampler state created")
    }
    
    // MARK: - Frame Management
    
    @MainActor
    private func beginFrame() {
        frameStartTime = CACurrentMediaTime()
        
        guard let view = view,
              let drawable = view.currentDrawable,
              let commandQueue = commandQueue else {
            print("[MetalRendererSwift] ERROR: Missing rendering resources")
            return
        }
        
        currentCommandBuffer = commandQueue.makeCommandBuffer()
        currentCommandBuffer?.label = "Frame Command Buffer"
        
        let renderPassDescriptor = view.currentRenderPassDescriptor
        renderPassDescriptor?.colorAttachments[0].clearColor = view.clearColor
        
        currentEncoder = currentCommandBuffer?.makeRenderCommandEncoder(descriptor: renderPassDescriptor!)
        currentEncoder?.label = "Main Render Encoder"
        
        // Set uniform buffer
        currentEncoder?.setVertexBuffer(uniformBuffer, offset: 0, index: 1)
    }
    
    public func getCurrentEncoder() -> MTLRenderCommandEncoder? {
        return currentEncoder
    }
    
    @MainActor
    private func endFrame() {
        // Flush any remaining batched commands
        flushBatch()
        
        currentEncoder?.endEncoding()
        
        if let drawable = view?.currentDrawable {
            currentCommandBuffer?.present(drawable)
        }
        
        currentCommandBuffer?.commit()
        
        // Reset state
        currentCommandBuffer = nil
        currentEncoder = nil
        currentTexture = nil
        
        // Reset debug stats
        debugStats = DebugStats()
    }
    
    @MainActor
    private func clear(_ color: RaylibColor) {
        // The clear color is set in the render pass descriptor
        // We just need to update the view's clear color for next frame
        view?.clearColor = MTLClearColorMake(
            Double(color.r) / 255.0,
            Double(color.g) / 255.0,
            Double(color.b) / 255.0,
            Double(color.a) / 255.0
        )
    }
    
    // MARK: - Vertex Batching
    
    private func addVertices(_ newVertices: [MetalVertex2D], texture: MTLTexture?, renderState: UInt32) {
        guard !newVertices.isEmpty else { return }
        
        // Check if we need to start a new draw command
        if drawCommands.isEmpty ||
           drawCommands.last?.texture !== texture ||
           drawCommands.last?.renderState != renderState {
            
            var cmd = DrawCommand()
            cmd.primitiveType = .triangleStrip
            cmd.vertexStart = vertices.count
            cmd.vertexCount = 0
            cmd.texture = texture
            cmd.useTexture = (texture != nil)
            cmd.renderState = renderState
            cmd.debugName = "BatchedVertices"
            
            drawCommands.append(cmd)
        }
        
        // Add vertices to the batch
        vertices.append(contentsOf: newVertices)
        
        // Update the current draw command
        if !drawCommands.isEmpty {
            drawCommands[drawCommands.count - 1].vertexCount += newVertices.count
        }
        
        debugStats.batchedVertices += newVertices.count
    }
    
    private func flushBatch() {
        guard !vertices.isEmpty, let encoder = currentEncoder else { return }
        
        print("[MetalRendererSwift] FlushBatch: vertices=\(vertices.count), commands=\(drawCommands.count)")
        
        // Get vertex data size
        let dataSize = vertices.count * MemoryLayout<MetalVertex2D>.stride
        
        // Allocate space in frame's vertex buffer
        guard let frameResources = frameResources,
              let (destinationBuffer, bufferOffset) = frameResources.allocateVertexBuffer(size: dataSize) else {
            print("[MetalRendererSwift] ERROR: Failed to allocate vertex buffer space")
            return
        }
        
        // Copy vertex data
        let vertexData = UnsafeMutableRawPointer(destinationBuffer)
        vertices.withUnsafeBytes { bytes in
            vertexData.copyMemory(from: bytes.baseAddress!, byteCount: dataSize)
        }
        
        // Set vertex buffer
        encoder.setVertexBuffer(frameResources.getCurrentVertexBuffer(), offset: bufferOffset, index: 0)
        
        // Execute draw commands
        executeDrawCommands()
        
        // Clear batch data
        vertices.removeAll()
        drawCommands.removeAll()
        
        print("[MetalRendererSwift] ✅ FlushBatch completed")
    }
    
    private func executeDrawCommands() {
        guard let encoder = currentEncoder else { return }
        
        var currentPipeline: MTLRenderPipelineState?
        var currentTexture: MTLTexture?
        
        for cmd in drawCommands {
            // Determine required pipeline
            let requiredPipeline: MTLRenderPipelineState?
            if cmd.useTexture {
                if cmd.renderState & Self.RENDER_STATE_SDF != 0 {
                    requiredPipeline = sdfPipeline
                } else {
                    requiredPipeline = texturePipeline
                }
            } else {
                requiredPipeline = colorPipeline
            }
            
            // Change pipeline if needed
            if requiredPipeline !== currentPipeline {
                encoder.setRenderPipelineState(requiredPipeline!)
                currentPipeline = requiredPipeline
                debugStats.stateChanges += 1
            }
            
            // Bind texture if needed
            if cmd.useTexture, let texture = cmd.texture {
                if texture !== currentTexture {
                    encoder.setFragmentTexture(texture, index: 0)
                    encoder.setFragmentSamplerState(samplerState, index: 0)
                    currentTexture = texture
                    debugStats.textureBinds += 1
                }
            }
            
            // Draw primitives
            encoder.drawPrimitives(type: cmd.primitiveType,
                                 vertexStart: cmd.vertexStart,
                                 vertexCount: cmd.vertexCount)
        }
    }
    
    // MARK: - Utility Functions
    
    private func setProjectionMatrix(width: Float, height: Float) {
        projectionMatrix = makeOrthoMatrix(left: 0, right: width, bottom: height, top: 0, near: -1, far: 1)
        
        // Update uniform buffer
        guard let uniformBuffer = uniformBuffer else { return }
        let uniforms = uniformBuffer.contents().bindMemory(to: simd_float4x4.self, capacity: 1)
        uniforms.pointee = projectionMatrix
    }
    
    private func makeOrthoMatrix(left: Float, right: Float, bottom: Float, top: Float, near: Float, far: Float) -> simd_float4x4 {
        let width = right - left
        let height = top - bottom
        let depth = far - near
        
        var matrix = matrix_identity_float4x4
        matrix.columns.0.x = 2.0 / width
        matrix.columns.1.y = 2.0 / height
        matrix.columns.2.z = -1.0 / depth
        matrix.columns.3.x = -(right + left) / width
        matrix.columns.3.y = -(top + bottom) / height
        matrix.columns.3.z = -near / depth
        
        return matrix
    }
    
    private func layerName(for layer: RenderLayer) -> String {
        switch layer {
        case .background: return "Background"
        case .midground: return "Midground"
        case .foreground: return "Foreground"
        case .ui: return "UI"
        case .text: return "Text"
        }
    }
    
    private func releaseResources() {
        // Release Metal resources
        vertexBuffer = nil
        uniformBuffer = nil
        texturePipeline = nil
        colorPipeline = nil
        sdfPipeline = nil
        depthStencilState = nil
        uiDepthStencilState = nil
        samplerState = nil
        commandQueue = nil
        currentCommandBuffer = nil
        currentEncoder = nil
        currentTexture = nil
        
        // Clear data
        vertices.removeAll()
        drawCommands.removeAll()
        currentVertexBufferOffset = 0
        
        frameResources = nil
    }

    // MARK: - Geometry Generation Helpers

    private func drawRectangleLines(x: Float, y: Float, width: Float, height: Float, thickness: Float, color: RaylibColor) {
        drawLineEx(startX: x, startY: y, endX: x + width, endY: y, thickness: thickness, color: color)
        drawLineEx(startX: x + width, startY: y, endX: x + width, endY: y + height, thickness: thickness, color: color)
        drawLineEx(startX: x + width, startY: y + height, endX: x, endY: y + height, thickness: thickness, color: color)
        drawLineEx(startX: x, startY: y + height, endX: x, endY: y, thickness: thickness, color: color)
    }

    private func drawArcLines(centerX: Float, centerY: Float, radius: Float, startAngle: Float, endAngle: Float, segments: Int, thickness: Float, color: RaylibColor) {
        let angleStep = (endAngle - startAngle) * (.pi / 180.0) / Float(segments)
        let startRad = startAngle * (.pi / 180.0)

        for i in 0..<segments {
            let angle1 = startRad + Float(i) * angleStep
            let angle2 = startRad + Float(i + 1) * angleStep

            let startX = centerX + cos(angle1) * radius
            let startY = centerY + sin(angle1) * radius
            let endX = centerX + cos(angle2) * radius
            let endY = centerY + sin(angle2) * radius

            drawLineEx(startX: startX, startY: startY, endX: endX, endY: endY, thickness: thickness, color: color)
        }
    }

    private func createQuad(x: Float, y: Float, width: Float, height: Float, color: simd_float4) -> [MetalVertex2D] {
        return [
            MetalVertex2D(position: simd_float2(x, y), texCoords: simd_float2(0, 0), color: color),
            MetalVertex2D(position: simd_float2(x + width, y), texCoords: simd_float2(1, 0), color: color),
            MetalVertex2D(position: simd_float2(x, y + height), texCoords: simd_float2(0, 1), color: color),

            MetalVertex2D(position: simd_float2(x + width, y), texCoords: simd_float2(1, 0), color: color),
            MetalVertex2D(position: simd_float2(x + width, y + height), texCoords: simd_float2(1, 1), color: color),
            MetalVertex2D(position: simd_float2(x, y + height), texCoords: simd_float2(0, 1), color: color)
        ]
    }

    private func createTriangleFan(centerX: Float, centerY: Float, radius: Float, startAngle: Float, endAngle: Float, segments: Int, color: simd_float4) -> [MetalVertex2D] {
        var fanVertices: [MetalVertex2D] = []
        let angleStep = (endAngle - startAngle) / Float(segments)

        for i in 0...segments {
            let angle1 = startAngle + Float(i) * angleStep
            let angle2 = startAngle + Float(i + 1) * angleStep

            fanVertices.append(MetalVertex2D(position: simd_float2(centerX, centerY), texCoords: simd_float2(0.5, 0.5), color: color))
            fanVertices.append(MetalVertex2D(position: simd_float2(centerX + cos(angle1) * radius, centerY + sin(angle1) * radius), texCoords: simd_float2(0.5, 0.5), color: color))
            fanVertices.append(MetalVertex2D(position: simd_float2(centerX + cos(angle2) * radius, centerY + sin(angle2) * radius), texCoords: simd_float2(0.5, 0.5), color: color))
        }
        return fanVertices
    }
}

// MARK: - Frame Resources Helper

/// Swift equivalent of MetalFrameResources
private class MetalFrameResourcesSwift {
    private var device: MTLDevice?
    private var vertexBuffer: MTLBuffer?
    private var currentOffset: Int = 0
    private let bufferSize = 1024 * 1024 // 1MB
    
    func initialize(device: MTLDevice) -> Bool {
        self.device = device
        
        vertexBuffer = device.makeBuffer(length: bufferSize, options: .storageModeShared)
        vertexBuffer?.label = "Frame Vertex Buffer"
        
        return vertexBuffer != nil
    }
    
    func allocateVertexBuffer(size: Int) -> (UnsafeMutableRawPointer, Int)? {
        guard let buffer = vertexBuffer else { return nil }
        
        // Simple linear allocation (reset each frame)
        if currentOffset + size > bufferSize {
            currentOffset = 0 // Reset for next frame
        }
        
        let offset = currentOffset
        currentOffset += size
        
        let pointer = buffer.contents().advanced(by: offset)
        return (pointer, offset)
    }
    
    func getCurrentVertexBuffer() -> MTLBuffer? {
        return vertexBuffer
    }
    
    func reset() {
        currentOffset = 0
    }
}
