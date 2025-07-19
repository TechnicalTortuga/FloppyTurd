//
//  MetalTextRendererSwift.swift
//  Professional Game Engine - Native Swift Text Renderer
//
//  Created by C++ Swift Interop Migration
//  Direct Swift replacement for MetalTextRenderer.mm
//


import Foundation
import simd
import Metal
import MetalKit
import CoreText

/// Professional Metal text renderer implementation in pure Swift
/// Direct replacement for MetalTextRenderer.mm with zero C++ dependencies
public class MetalTextRendererSwift {
    
    // MARK: - Metal Resources
    private var device: MTLDevice?
    private var textPipeline: MTLRenderPipelineState?
    private var textSamplerState: MTLSamplerState?
    private var view: MTKView?
    
    // Font atlas management
    private var fontAtlas: FontAtlas?
    private var defaultFont: CTFont?
    private var fontCache: [String: CTFont] = [:]
    
    // Text batching
    private var textVertices: [TextVertex] = []
    private var textDrawCommands: [TextDrawCommand] = []
    
    // MARK: - Swift Structures
    
    public struct TextVertex {
        var position: simd_float2
        var texCoords: simd_float2
        var color: simd_float4
    }
    
    public struct TextDrawCommand {
        var vertexStart: Int = 0
        var vertexCount: Int = 0
        var texture: MTLTexture?
        var fontSize: Float = 0
        var debugText: String = ""
        var isSDF: Bool = false
        var sdfParams: SDFParams = SDFParams()
    }
    
    /// SDF rendering parameters (preserving your SDF system)
    public struct SDFParams {
        var smoothing: Float = 0.1      // SDF smoothing factor
        var threshold: Float = 0.5      // SDF threshold value
        var outlineWidth: Float = 0.0   // Outline width (0 = no outline)
        var shadowOffset: simd_float2 = simd_float2(0, 0)  // Shadow offset
        var outlineColor: simd_float4 = simd_float4(0, 0, 0, 1)  // Outline color
        var shadowColor: simd_float4 = simd_float4(0, 0, 0, 0.5) // Shadow color
        
        init() {} // Default initializer
        
        init(smoothing: Float, threshold: Float, outlineWidth: Float = 0.0) {
            self.smoothing = smoothing
            self.threshold = threshold
            self.outlineWidth = outlineWidth
        }
    }
    
    public struct FontMetrics {
        var ascent: Float
        var descent: Float
        var leading: Float
        var lineHeight: Float
    }
    
    // MARK: - Font Atlas Management
    
    /// Font atlas for efficient text rendering
    private class FontAtlas {
        private var atlasTexture: MTLTexture?
        private var glyphMap: [String: GlyphInfo] = [:]  // Changed to String key for SDF variants
        var atlasSize: Int = 1024 // Made internal for access
        private var currentX: Int = 0
        private var currentY: Int = 0
        private var lineHeight: Int = 0
        
        struct GlyphInfo {
            var uvRect: simd_float4  // x, y, width, height in UV space
            var metrics: simd_float4 // bearingX, bearingY, advance, fontSize
            var isSDF: Bool = false  // Whether this glyph uses SDF rendering
        }
        
        func initialize(device: MTLDevice, font: CTFont) -> Bool {
            // Create atlas texture
            let textureDesc = MTLTextureDescriptor.texture2DDescriptor(
                pixelFormat: .r8Unorm,
                width: atlasSize,
                height: atlasSize,
                mipmapped: false
            )
            textureDesc.usage = [.shaderRead, .renderTarget]
            
            atlasTexture = device.makeTexture(descriptor: textureDesc)
            atlasTexture?.label = "Font Atlas"
            
            guard atlasTexture != nil else {
                print("[MetalTextRendererSwift] ERROR: Failed to create font atlas texture")
                return false
            }
            
            // Pre-cache common ASCII characters with SDF support
            let commonChars = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
            
            for char in commonChars {
                _ = getOrCreateGlyph(char: char, font: font, sdfEnabled: true)  // Cache as SDF by default
            }
            
            print("[MetalTextRendererSwift] ✅ Font atlas initialized with \(glyphMap.count) glyphs")
            return true
        }
        
        func getOrCreateGlyph(char: Character, font: CTFont, sdfEnabled: Bool = true) -> GlyphInfo? {
            // Check if glyph already exists
            let glyphKey = "\(char)_\(sdfEnabled ? "sdf" : "regular")"
            if let existingGlyph = glyphMap[glyphKey] {
                return existingGlyph
            }
            
            // Create glyph with SDF support
            guard let glyph = createGlyph(char: char, font: font, sdfEnabled: sdfEnabled) else {
                return nil
            }
            
            glyphMap[glyphKey] = glyph
            return glyph
        }
        
        private func createGlyph(char: Character, font: CTFont, sdfEnabled: Bool) -> GlyphInfo? {
            let string = String(char)
            let attributedString = NSAttributedString(string: string, attributes: [.font: font])
            
            // Create Core Text frame
            let framesetter = CTFramesetterCreateWithAttributedString(attributedString)
            let textSize = CTFramesetterSuggestFrameSizeWithConstraints(framesetter, CFRange(location: 0, length: 0), nil, CGSize(width: 1000, height: 1000), nil)
            
            // Increase glyph size for SDF generation (more resolution for distance field calculation)
            let sdfScale: CGFloat = sdfEnabled ? 4.0 : 1.0
            let glyphWidth = Int(ceil(textSize.width * sdfScale)) + 8  // Extra padding for SDF
            let glyphHeight = Int(ceil(textSize.height * sdfScale)) + 8
            
            // Check if we need to move to next line in atlas
            if currentX + glyphWidth > atlasSize {
                currentX = 0
                currentY += lineHeight
                lineHeight = 0
            }
            
            // Check if we're out of space
            if currentY + glyphHeight > atlasSize {
                print("[MetalTextRendererSwift] WARNING: Font atlas is full, glyph '\(char)' skipped")
                return nil
            }
            
            // Create bitmap context for glyph
            let colorSpace = CGColorSpaceCreateDeviceGray()
            let bitmapContext = CGContext(
                data: nil,
                width: glyphWidth,
                height: glyphHeight,
                bitsPerComponent: 8,
                bytesPerRow: glyphWidth,
                space: colorSpace,
                bitmapInfo: CGImageAlphaInfo.none.rawValue
            )
            
            guard let context = bitmapContext else {
                print("[MetalTextRendererSwift] ERROR: Failed to create bitmap context for glyph '\(char)'")
                return nil
            }
            
            // Configure context
            context.setFillColor(CGColor(gray: 1.0, alpha: 1.0))  // White text
            context.setTextDrawingMode(.fill)
            
            // Draw glyph at higher resolution for SDF
            let drawRect = CGRect(x: 4, y: 4, width: textSize.width * sdfScale, height: textSize.height * sdfScale)
            let path = CGPath(rect: drawRect, transform: nil)
            let frame = CTFramesetterCreateFrame(framesetter, CFRange(location: 0, length: 0), path, nil)
            CTFrameDraw(frame, context)
            
            // Generate SDF if enabled
            var finalData: UnsafeMutableRawPointer
            if sdfEnabled {
                finalData = generateSDFFromBitmap(context: context, width: glyphWidth, height: glyphHeight)
            } else {
                finalData = context.data!
            }
            
            // Upload to atlas texture
            let region = MTLRegion(origin: MTLOrigin(x: currentX, y: currentY, z: 0),
                                  size: MTLSize(width: glyphWidth, height: glyphHeight, depth: 1))
            
            atlasTexture?.replace(region: region, mipmapLevel: 0, withBytes: finalData, bytesPerRow: glyphWidth)
            
            // Clean up SDF data if we generated it
            if sdfEnabled && finalData != context.data {
                free(finalData)
            }
            
            // Create glyph info
            let uvRect = simd_float4(
                Float(currentX) / Float(atlasSize),           // u
                Float(currentY) / Float(atlasSize),           // v
                Float(glyphWidth) / Float(atlasSize),         // width
                Float(glyphHeight) / Float(atlasSize)         // height
            )
            
            // Get font metrics
            let ascent = Float(CTFontGetAscent(font))
            let advance = Float(textSize.width)
            let fontSize = Float(CTFontGetSize(font))
            
            let metrics = simd_float4(
                0,          // bearingX
                ascent,     // bearingY  
                advance,    // advance
                fontSize    // original font size for SDF scaling
            )
            
            let glyphInfo = GlyphInfo(uvRect: uvRect, metrics: metrics, isSDF: sdfEnabled)
            
            // Update atlas position
            currentX += glyphWidth
            lineHeight = max(lineHeight, glyphHeight)
            
            return glyphInfo
        }
        
        /// Generate SDF (Signed Distance Field) from bitmap (preserving your SDF system)
        private func generateSDFFromBitmap(context: CGContext, width: Int, height: Int) -> UnsafeMutableRawPointer {
            guard let inputData = context.data else {
                print("[MetalTextRendererSwift] ERROR: No bitmap data for SDF generation")
                return context.data!
            }
            
            // Allocate output buffer for SDF
            let outputData = malloc(width * height)!
            let input = inputData.bindMemory(to: UInt8.self, capacity: width * height)
            let output = outputData.bindMemory(to: UInt8.self, capacity: width * height)
            
            // SDF generation parameters
            let spread: Float = 4.0  // Distance field spread in pixels
            let maxDistance = spread
            
            // Generate SDF using distance transform
            for y in 0..<height {
                for x in 0..<width {
                    let currentIndex = y * width + x
                    let currentPixel = input[currentIndex]
                    let isInside = currentPixel > 127  // Threshold for inside/outside
                    
                    var minDistance: Float = maxDistance
                    
                    // Search in a square around the current pixel
                    let searchRadius = Int(ceil(spread))
                    for dy in -searchRadius...searchRadius {
                        for dx in -searchRadius...searchRadius {
                            let nx = x + dx
                            let ny = y + dy
                            
                            // Check bounds
                            if nx >= 0 && nx < width && ny >= 0 && ny < height {
                                let neighborIndex = ny * width + nx
                                let neighborPixel = input[neighborIndex]
                                let neighborIsInside = neighborPixel > 127
                                
                                // If we found a pixel with different inside/outside state
                                if isInside != neighborIsInside {
                                    let distance = sqrtf(Float(dx * dx + dy * dy))
                                    minDistance = min(minDistance, distance)
                                }
                            }
                        }
                    }
                    
                    // Convert distance to SDF value
                    var sdfValue: Float
                    if isInside {
                        sdfValue = 0.5 + (minDistance / (2.0 * spread))  // Inside: 0.5 to 1.0
                    } else {
                        sdfValue = 0.5 - (minDistance / (2.0 * spread))  // Outside: 0.0 to 0.5
                    }
                    
                    // Clamp and convert to byte
                    sdfValue = max(0.0, min(1.0, sdfValue))
                    output[currentIndex] = UInt8(sdfValue * 255.0)
                }
            }
            
            print("[MetalTextRendererSwift] ✅ Generated SDF for glyph: \(width)x\(height)")
            return outputData
        }
        
        func getTexture() -> MTLTexture? {
            return atlasTexture
        }
    }
    
    // MARK: - Initialization
    
    public init() {
        print("[MetalTextRendererSwift] Initializing Swift text renderer")
    }
    
    deinit {
        print("[MetalTextRendererSwift] Destroying Swift text renderer")
        shutdown()
    }
    
    /// Initialize the text renderer
    @MainActor public func initialize(device: MTLDevice, view: MTKView) -> Bool {
        print("[MetalTextRendererSwift] Initialize START")
        
        self.device = device
        self.view = view
        
        // Load default font
        if !loadDefaultFont() {
            print("[MetalTextRendererSwift] ERROR: Failed to load default font")
            return false
        }
        
        // Create font atlas
        guard let font = defaultFont else {
            print("[MetalTextRendererSwift] ERROR: No default font available")
            return false
        }
        
        fontAtlas = FontAtlas()
        if !fontAtlas!.initialize(device: device, font: font) {
            print("[MetalTextRendererSwift] ERROR: Failed to initialize font atlas")
            return false
        }
        
        // Create text rendering pipeline
        if !createTextPipeline() {
            print("[MetalTextRendererSwift] ERROR: Failed to create text pipeline")
            return false
        }
        
        // Create sampler state
        createTextSamplerState()
        
        print("[MetalTextRendererSwift] ✅ Text renderer initialized successfully")
        return true
    }
    
    /// Shutdown the text renderer
    public func shutdown() {
        print("[MetalTextRendererSwift] Shutting down text renderer")
        
        // Release resources
        textPipeline = nil
        textSamplerState = nil
        fontAtlas = nil
        defaultFont = nil
        fontCache.removeAll()
        
        // Clear batching data
        textVertices.removeAll()
        textDrawCommands.removeAll()
        
        print("[MetalTextRendererSwift] ✅ Text renderer shutdown complete")
    }
    
    // MARK: - Text Rendering API
    
    /// Draw text at the specified position with SDF support
    public func drawText(_ text: String, x: Float, y: Float, fontSize: Float, color: RaylibColor) {
        drawText(text, x: x, y: y, fontSize: fontSize, color: color, sdfEnabled: true)
    }
    
    /// Draw text with explicit SDF control (preserving your SDF system)
    public func drawText(_ text: String, x: Float, y: Float, fontSize: Float, color: RaylibColor, sdfEnabled: Bool) {
        print("[MetalTextRendererSwift] DrawText: '\(text)' at (\(x),\(y)), size=\(fontSize), SDF=\(sdfEnabled)")
        
        guard let font = getFont(size: fontSize) else {
            print("[MetalTextRendererSwift] ERROR: Failed to get font for size \(fontSize)")
            return
        }
        
        let colorVec = simd_float4(Float(color.r)/255.0, Float(color.g)/255.0, Float(color.b)/255.0, Float(color.a)/255.0)
        
        var currentX = x
        let startVertexCount = textVertices.count
        
        // Generate vertices for each character
        for char in text {
            guard let glyphInfo = fontAtlas?.getOrCreateGlyph(char: char, font: font, sdfEnabled: sdfEnabled) else {
                print("[MetalTextRendererSwift] WARNING: Failed to get glyph for '\(char)'")
                continue
            }
            
            let glyphWidth = glyphInfo.uvRect.z * Float(fontAtlas?.atlasSize ?? 1024)
            let glyphHeight = glyphInfo.uvRect.w * Float(fontAtlas?.atlasSize ?? 1024)
            
            // Scale glyph based on font size for SDF rendering
            let scaleFactor = sdfEnabled ? fontSize / glyphInfo.metrics.w : 1.0
            let scaledWidth = glyphWidth * scaleFactor
            let scaledHeight = glyphHeight * scaleFactor
            
            // Create quad for this character (triangle strip)
            let vertices = [
                TextVertex(position: simd_float2(currentX, y), 
                          texCoords: simd_float2(glyphInfo.uvRect.x, glyphInfo.uvRect.y), 
                          color: colorVec),
                TextVertex(position: simd_float2(currentX + scaledWidth, y), 
                          texCoords: simd_float2(glyphInfo.uvRect.x + glyphInfo.uvRect.z, glyphInfo.uvRect.y), 
                          color: colorVec),
                TextVertex(position: simd_float2(currentX, y + scaledHeight), 
                          texCoords: simd_float2(glyphInfo.uvRect.x, glyphInfo.uvRect.y + glyphInfo.uvRect.w), 
                          color: colorVec),
                TextVertex(position: simd_float2(currentX + scaledWidth, y + scaledHeight), 
                          texCoords: simd_float2(glyphInfo.uvRect.x + glyphInfo.uvRect.z, glyphInfo.uvRect.y + glyphInfo.uvRect.w), 
                          color: colorVec)
            ]
            
            textVertices.append(contentsOf: vertices)
            currentX += glyphInfo.metrics.z * scaleFactor // Advance with scale
        }
        
        // Create draw command for this text
        let vertexCount = textVertices.count - startVertexCount
        if vertexCount > 0 {
            let cmd = TextDrawCommand(
                vertexStart: startVertexCount,
                vertexCount: vertexCount,
                texture: fontAtlas?.getTexture(),
                fontSize: fontSize,
                debugText: text,
                isSDF: sdfEnabled,
                sdfParams: sdfEnabled ? calculateSDFParams(fontSize: fontSize) : SDFParams()
            )
            textDrawCommands.append(cmd)
        }
    }
    
    /// Calculate SDF parameters based on font size (preserving your SDF system)
    private func calculateSDFParams(fontSize: Float) -> SDFParams {
        // Adaptive SDF parameters based on font size
        let baseSize: Float = 24.0
        let sizeRatio = fontSize / baseSize
        
        var params = SDFParams()
        
        // Adjust smoothing based on size - larger fonts need less smoothing
        params.smoothing = max(0.05, 0.15 / sizeRatio)
        
        // Threshold stays consistent for crisp rendering
        params.threshold = 0.5
        
        // Optional outline width (could be configurable)
        params.outlineWidth = 0.0
        
        return params
    }
    
    /// Measure text size with SDF support
    public func measureText(_ text: String, fontSize: Float, sdfEnabled: Bool = true) -> Vector2 {
        guard let font = getFont(size: fontSize) else {
            return Vector2(x: 0, y: 0)
        }
        
        var totalWidth: Float = 0
        var maxHeight: Float = 0
        
        for char in text {
            guard let glyphInfo = fontAtlas?.getOrCreateGlyph(char: char, font: font, sdfEnabled: sdfEnabled) else {
                continue
            }
            
            let scaleFactor = sdfEnabled ? fontSize / glyphInfo.metrics.w : 1.0
            totalWidth += glyphInfo.metrics.z * scaleFactor // Advance with scale
            
            let glyphHeight = glyphInfo.uvRect.w * Float(fontAtlas?.atlasSize ?? 1024) * scaleFactor
            maxHeight = max(maxHeight, glyphHeight)
        }
        
        return Vector2(x: totalWidth, y: maxHeight)
    }
    
    /// Flush all batched text to the GPU with SDF support
    public func flushTextBatch(encoder: MTLRenderCommandEncoder) {
        guard !textVertices.isEmpty, !textDrawCommands.isEmpty else { return }
        
        print("[MetalTextRendererSwift] FlushTextBatch: vertices=\(textVertices.count), commands=\(textDrawCommands.count)")
        
        // Set text pipeline
        encoder.setRenderPipelineState(textPipeline!)
        
        // Bind font atlas texture
        if let atlasTexture = fontAtlas?.getTexture() {
            encoder.setFragmentTexture(atlasTexture, index: 0)
            encoder.setFragmentSamplerState(textSamplerState, index: 0)
        }
        
        // Create temporary vertex buffer for this batch
        let vertexDataSize = textVertices.count * MemoryLayout<TextVertex>.stride
        guard let vertexBuffer = device?.makeBuffer(bytes: textVertices, length: vertexDataSize, options: .storageModeShared) else {
            print("[MetalTextRendererSwift] ERROR: Failed to create vertex buffer")
            return
        }
        
        encoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        
        // Execute draw commands with SDF parameter support
        for cmd in textDrawCommands {
            // Set SDF parameters if this is an SDF text command
            if cmd.isSDF {
                // Create SDF parameter buffer
                var sdfParams = cmd.sdfParams
                let sdfParamSize = MemoryLayout<SDFParams>.stride
                guard let sdfBuffer = device?.makeBuffer(bytes: &sdfParams, length: sdfParamSize, options: .storageModeShared) else {
                    print("[MetalTextRendererSwift] ERROR: Failed to create SDF parameter buffer")
                    continue
                }
                
                // Bind SDF parameters to fragment shader
                encoder.setFragmentBuffer(sdfBuffer, offset: 0, index: 0)
                
                print("[MetalTextRendererSwift] SDF text '\(cmd.debugText)': smoothing=\(sdfParams.smoothing), threshold=\(sdfParams.threshold)")
            }
            
            // Draw the text
            encoder.drawPrimitives(type: .triangleStrip, vertexStart: cmd.vertexStart, vertexCount: cmd.vertexCount)
        }
        
        // Clear batch
        textVertices.removeAll()
        textDrawCommands.removeAll()
        
        print("[MetalTextRendererSwift] ✅ Text batch flushed successfully")
    }
    
    // MARK: - Private Implementation
    
    private func loadDefaultFont() -> Bool {
        // Try to load a good default font
        let fontNames = ["HelveticaNeue", "Helvetica", "Arial", "System"]
        
        for fontName in fontNames {
            let font = CTFontCreateWithName(fontName as CFString, 24.0, nil)
            defaultFont = font
            print("[MetalTextRendererSwift] ✅ Loaded default font: \(fontName)")
            return true
        }
        
        // Fallback to system font
        if let systemFont = CTFontCreateUIFontForLanguage(.system, 24.0, nil) {
             defaultFont = systemFont
             print("[MetalTextRendererSwift] ✅ Loaded system font as fallback")
             return true
        }
        
        print("[MetalTextRendererSwift] ERROR: Failed to load any default font")
        return false
    }
    
    private func getFont(size: Float) -> CTFont? {
        let sizeKey = String(Int(size))
        
        if let cachedFont = fontCache[sizeKey] {
            return cachedFont
        }
        
        guard let baseFont = defaultFont else { return nil }
        
        let scaledFont = CTFontCreateCopyWithAttributes(baseFont, CGFloat(size), nil, nil)
        fontCache[sizeKey] = scaledFont
        
        return scaledFont
    }
    
    @MainActor
    private func createTextPipeline() -> Bool {
        guard let device = device else { return false }
        
        // Create shader library (using same shader library as main renderer)
        guard let library = device.makeDefaultLibrary() else {
            print("[MetalTextRendererSwift] ERROR: Failed to load shader library")
            return false
        }
        
        guard let vertexFunction = library.makeFunction(name: "vertex_shader_text"),
              let fragmentFunction = library.makeFunction(name: "fragment_shader_text") else {
            print("[MetalTextRendererSwift] ERROR: Failed to load text shader functions")
            return false
        }
        
        // Create vertex descriptor for text
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
        
        vertexDescriptor.layouts[0].stride = MemoryLayout<TextVertex>.stride
        vertexDescriptor.layouts[0].stepRate = 1
        vertexDescriptor.layouts[0].stepFunction = .perVertex
        
        // Create pipeline descriptor
        let pipelineDesc = MTLRenderPipelineDescriptor()
        pipelineDesc.label = "Text Pipeline"
        pipelineDesc.vertexFunction = vertexFunction
        pipelineDesc.fragmentFunction = fragmentFunction
        pipelineDesc.vertexDescriptor = vertexDescriptor
        pipelineDesc.colorAttachments[0].pixelFormat = view?.colorPixelFormat ?? .bgra8Unorm
        pipelineDesc.depthAttachmentPixelFormat = view?.depthStencilPixelFormat ?? .depth32Float
        
        // Set up alpha blending for text
        pipelineDesc.colorAttachments[0].isBlendingEnabled = true
        pipelineDesc.colorAttachments[0].rgbBlendOperation = .add
        pipelineDesc.colorAttachments[0].alphaBlendOperation = .add
        pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = .sourceAlpha
        pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
        
        do {
            textPipeline = try device.makeRenderPipelineState(descriptor: pipelineDesc)
            print("[MetalTextRendererSwift] ✅ Text pipeline created successfully")
            return true
        } catch {
            print("[MetalTextRendererSwift] ERROR: Failed to create text pipeline: \(error)")
            return false
        }
    }
    
    private func createTextSamplerState() {
        guard let device = device else { return }
        
        let samplerDesc = MTLSamplerDescriptor()
        samplerDesc.minFilter = .linear
        samplerDesc.magFilter = .linear
        samplerDesc.mipFilter = .notMipmapped
        samplerDesc.sAddressMode = .clampToEdge
        samplerDesc.tAddressMode = .clampToEdge
        
        textSamplerState = device.makeSamplerState(descriptor: samplerDesc)
        print("[MetalTextRendererSwift] ✅ Text sampler state created")
    }
    
    // MARK: - Font Management API for C++ Interop
    
    /// Load font by name and return font ID for C++ compatibility
    public func loadFont(name: String, size: Float = 24.0) -> Int32 {
        let fontKey = "\(name)_\(size)"
        
        // Check if already cached
        if fontCache[fontKey] != nil {
            print("[MetalTextRendererSwift] Font '\(name)' already loaded")
            return hashFontName(fontKey)
        }
        
        // Try to load the font
        var font: CTFont?
        if let customFont = CTFontCreateWithName(name as CFString, CGFloat(size), nil) {
            font = customFont
        } else {
            // Fallback to default font with specified size
            if let baseFont = defaultFont {
                font = CTFontCreateCopyWithAttributes(baseFont, CGFloat(size), nil, nil)
            }
        }
        
        guard let loadedFont = font else {
            print("[MetalTextRendererSwift] Failed to load font '\(name)'")
            return 0
        }
        
        fontCache[fontKey] = loadedFont
        print("[MetalTextRendererSwift] ✅ Loaded font '\(name)' with size \(size)")
        return hashFontName(fontKey)
    }
    
    /// Unload font by ID
    public func unloadFont(id: Int32) {
        // Find font by ID and remove from cache
        for (key, _) in fontCache {
            if hashFontName(key) == id {
                fontCache.removeValue(forKey: key)
                print("[MetalTextRendererSwift] Unloaded font with ID \(id)")
                return
            }
        }
        print("[MetalTextRendererSwift] Font with ID \(id) not found")
    }
    
    /// Get default font ID
    public func getDefaultFontId() -> Int32 {
        return loadFont(name: "system", size: 24.0)
    }
    
    /// Measure text for C++ interop
    public func measureTextForInterop(_ text: String, fontSize: Float) -> (width: Float, height: Float) {
        let size = measureText(text, fontSize: fontSize)
        return (size.x, size.y)
    }
    
    /// Simple hash function to convert font name to Int32 ID
    private func hashFontName(_ name: String) -> Int32 {
        var hash: Int32 = 5381
        for char in name {
            hash = ((hash << 5) &+ hash) &+ Int32(char.asciiValue ?? 0)
        }
        return abs(hash)
    }
}
