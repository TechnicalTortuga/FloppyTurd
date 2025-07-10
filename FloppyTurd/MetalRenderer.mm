#import "MetalRenderer.h"
#import "MetalTextRenderer.h"
#import "UICoordinateSystem.h"
#import <simd/simd.h>

#if defined(__APPLE__) && TARGET_OS_IOS

// Global renderer instance
MetalRenderer* g_metalRenderer = nullptr;
extern MetalTextRenderer* g_textRenderer;

MetalRenderer::MetalRenderer() 
    : m_view(nullptr)
    , m_device(nullptr)
    , m_commandQueue(nullptr)
    , m_texturePipeline(nullptr)
    , m_colorPipeline(nullptr)
    , m_instancedTexturePipeline(nullptr)
    , m_instancedColorPipeline(nullptr)
    , m_depthStencilState(nullptr)
    , m_samplerState(nullptr)
    , m_currentCommandBuffer(nullptr)
    , m_currentEncoder(nullptr)
    , m_currentRenderPass(nullptr)
    , m_currentVertexBufferOffset(0)
    , m_currentInstanceBufferOffset(0)
    , m_debugVisualization(false)
    , m_debugVertexBuffer(nullptr)
    , m_debugPipeline(nullptr)
    , m_frameStartTime(0)
    , m_targetFrameTime(1.0f/60.0f)
{
    m_projectionMatrix = matrix_identity_float4x4;
}

MetalRenderer::~MetalRenderer() {
    Shutdown();
}

bool MetalRenderer::Initialize(MTKView* view) {
    @autoreleasepool {
        m_view = view;
        m_device = view.device;
        
        if (!m_device) {
            TraceLog(LOG_ERROR, "[METAL ERROR] Failed to get Metal device");
            return false;
        }
        
        // Initialize global MetalTextRenderer if needed
        if (!g_textRenderer) {
            g_textRenderer = new MetalTextRenderer();
            g_textRenderer->Initialize(m_device);
        }
        
        // Create command queue
        m_commandQueue = [m_device newCommandQueue];
        if (!m_commandQueue) {
            TraceLog(LOG_ERROR, "[METAL ERROR] Failed to create command queue");
            return false;
        }
        
        // Set up view properties
        view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
        view.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        
        // Initialize frame resources for triple buffering
        if (!m_frameResources.Initialize(m_device)) {
            TraceLog(LOG_ERROR, "[METAL ERROR] Failed to initialize frame resources");
            return false;
        }
        
        // Create render pipelines
        CreatePipelines();
        
        // Create buffers
        CreateBuffers();
        
        // Create sampler state - use nearest-neighbor for crisp pixel art
        MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
        samplerDesc.minFilter = MTLSamplerMinMagFilterNearest;
        samplerDesc.magFilter = MTLSamplerMinMagFilterNearest;
        samplerDesc.mipFilter = MTLSamplerMipFilterNearest;
        samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
        m_samplerState = [m_device newSamplerStateWithDescriptor:samplerDesc];
        
        // Set up initial projection matrix using UICoordinateSystem
        Rectangle pixelScreenRect = UICoordinateSystem::GetPixelScreenRect();
        SetProjectionMatrix(pixelScreenRect.width, pixelScreenRect.height);
        
        // Optimize for current device
        OptimizeForDevice();
        
        TraceLog(LOG_INFO, "[METAL DEBUG] MetalRenderer initialized successfully");
        return true;
    }
}

void MetalRenderer::Shutdown() {
    FlushBatch();
    
    // Clean up global text renderer
    if (g_textRenderer) {
        g_textRenderer->Shutdown();
        delete g_textRenderer;
        g_textRenderer = nullptr;
        TraceLog(LOG_INFO, "[SHUTDOWN] Global text renderer cleaned up");
    }
    
    m_view = nullptr;
    m_device = nullptr;
    m_commandQueue = nullptr;
    m_texturePipeline = nullptr;
    m_colorPipeline = nullptr;
    m_instancedTexturePipeline = nullptr;
    m_instancedColorPipeline = nullptr;
    m_depthStencilState = nullptr;
    m_samplerState = nullptr;
    m_currentCommandBuffer = nullptr;
    m_currentEncoder = nullptr;
    m_currentRenderPass = nullptr;
    m_debugVertexBuffer = nullptr;
    m_debugPipeline = nullptr;
    
    m_vertices.clear();
    m_drawCommands.clear();
    m_instanceData.clear();
    // Matrix stack removed - using CPU vertex transformation
}

void MetalRenderer::CreatePipelines() {
    @autoreleasepool {
        NSError* error = nil;
        
        // Load shader library
        NSString* shaderPath = [[NSBundle mainBundle] pathForResource:@"Shaders2D" ofType:@"metal"];
        NSString* shaderSource = [NSString stringWithContentsOfFile:shaderPath encoding:NSUTF8StringEncoding error:&error];
        
        id<MTLLibrary> library;
        NSLog(@"[METAL DEBUG] Attempting to load shader library");
        NSLog(@"[METAL DEBUG] Device: %@", m_device);
        
        if (shaderSource) {
            // Metal debug logs removed for cleaner output
            library = [m_device newLibraryWithSource:shaderSource options:nil error:&error];
        } else {
            // Try to load default library
            // Metal debug logs removed for cleaner output
            library = [m_device newDefaultLibrary];
        }
        
        if (!library) {
            NSLog(@"[METAL ERROR] Failed to load shader library");
            if (error) {
                NSLog(@"[METAL ERROR] Error: %@", error.localizedDescription);
            }
            
            // Try to get more information about why the library failed to load
                    // Metal debug logs removed for cleaner output
            
            return;
        }
        
        // Metal debug logs removed for cleaner output
        
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_shader_2d"];
        id<MTLFunction> fragmentTexturedFunction = [library newFunctionWithName:@"fragment_shader_textured"];
        id<MTLFunction> fragmentColorFunction = [library newFunctionWithName:@"fragment_shader_color"];
        id<MTLFunction> fragmentSdfFunction = [library newFunctionWithName:@"fragment_shader_sdf"];
        
        // Load instanced vertex shader
        id<MTLFunction> vertexInstancedFunction = [library newFunctionWithName:@"vertex_shader_2d"];
        id<MTLFunction> vertexSimpleFunction = [library newFunctionWithName:@"vertex_shader_2d_simple"];
        
            // Check shader compilation
    if (!vertexFunction) TraceLog(LOG_ERROR, "[METAL ERROR] Failed to compile vertex_shader_2d");
    if (!fragmentTexturedFunction) TraceLog(LOG_ERROR, "[METAL ERROR] Failed to compile fragment_shader_textured");
    if (!fragmentColorFunction) TraceLog(LOG_ERROR, "[METAL ERROR] Failed to compile fragment_shader_color");
    if (!fragmentSdfFunction) TraceLog(LOG_ERROR, "[METAL ERROR] Failed to compile fragment_shader_sdf");
    if (!vertexInstancedFunction) TraceLog(LOG_ERROR, "[METAL ERROR] Failed to compile vertex_shader_2d (instanced)");
    if (!vertexSimpleFunction) TraceLog(LOG_ERROR, "[METAL ERROR] Failed to compile vertex_shader_2d_simple");
    
    if (vertexFunction && fragmentTexturedFunction && fragmentColorFunction && 
        fragmentSdfFunction && vertexInstancedFunction && vertexSimpleFunction) {
        TraceLog(LOG_INFO, "[METAL DEBUG] All shaders compiled successfully");
    } else {
        TraceLog(LOG_ERROR, "[METAL ERROR] Some shaders failed to compile - rendering will not work");
        return;
    }
        
        // Create vertex descriptor
        MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
        
        // Position (float2)
        vertexDesc.attributes[0].format = MTLVertexFormatFloat2;
        vertexDesc.attributes[0].offset = 0;
        vertexDesc.attributes[0].bufferIndex = 0;
        
        // Texture coordinates (float2)
        vertexDesc.attributes[1].format = MTLVertexFormatFloat2;
        vertexDesc.attributes[1].offset = 8;
        vertexDesc.attributes[1].bufferIndex = 0;
        
        // Color (float4)
        vertexDesc.attributes[2].format = MTLVertexFormatFloat4;
        vertexDesc.attributes[2].offset = 16;
        vertexDesc.attributes[2].bufferIndex = 0;
        
        vertexDesc.layouts[0].stride = sizeof(MetalVertex2D);
        vertexDesc.layouts[0].stepRate = 1;
        vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        
        // Create textured pipeline (non-instanced)
        MTLRenderPipelineDescriptor* texturedPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        texturedPipelineDesc.label = @"Textured Pipeline";
        texturedPipelineDesc.vertexFunction = vertexSimpleFunction;
        texturedPipelineDesc.fragmentFunction = fragmentTexturedFunction;
        texturedPipelineDesc.vertexDescriptor = vertexDesc;
        texturedPipelineDesc.colorAttachments[0].pixelFormat = m_view.colorPixelFormat;
        texturedPipelineDesc.colorAttachments[0].blendingEnabled = YES;
        texturedPipelineDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        texturedPipelineDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        texturedPipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        texturedPipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        texturedPipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        texturedPipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        texturedPipelineDesc.depthAttachmentPixelFormat = m_view.depthStencilPixelFormat;
        
        TraceLog(LOG_INFO, "[METAL DEBUG] Creating textured pipeline with pixelFormat=%lu, depthFormat=%lu", 
              (unsigned long)m_view.colorPixelFormat, (unsigned long)m_view.depthStencilPixelFormat);
        
        m_texturePipeline = [m_device newRenderPipelineStateWithDescriptor:texturedPipelineDesc error:&error];
        if (error) {
            TraceLog(LOG_ERROR, "[METAL ERROR] Failed to create textured pipeline: %@", error.localizedDescription);
        } else {
            // Metal debug logs removed for cleaner output
        }
        
        // Create color-only pipeline (non-instanced)
        MTLRenderPipelineDescriptor* colorPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        colorPipelineDesc.label = @"Color Pipeline";
        colorPipelineDesc.vertexFunction = vertexSimpleFunction;
        colorPipelineDesc.fragmentFunction = fragmentColorFunction;
        colorPipelineDesc.vertexDescriptor = vertexDesc;
        colorPipelineDesc.colorAttachments[0].pixelFormat = m_view.colorPixelFormat;
        colorPipelineDesc.colorAttachments[0].blendingEnabled = YES;
        colorPipelineDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        colorPipelineDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        colorPipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        colorPipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        colorPipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        colorPipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        colorPipelineDesc.depthAttachmentPixelFormat = m_view.depthStencilPixelFormat;
        
        m_colorPipeline = [m_device newRenderPipelineStateWithDescriptor:colorPipelineDesc error:&error];
        if (error) {
            TraceLog(LOG_ERROR, "[METAL ERROR] Failed to create color pipeline: %@", error.localizedDescription);
        } else {
            // Metal debug logs removed for cleaner output
        }
        
        // Create SDF pipeline for single-channel grayscale textures
        MTLRenderPipelineDescriptor* sdfPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        sdfPipelineDesc.label = @"SDF Pipeline";
        sdfPipelineDesc.vertexFunction = vertexSimpleFunction;
        sdfPipelineDesc.fragmentFunction = fragmentSdfFunction;
        sdfPipelineDesc.vertexDescriptor = vertexDesc;
        sdfPipelineDesc.colorAttachments[0].pixelFormat = m_view.colorPixelFormat;
        sdfPipelineDesc.colorAttachments[0].blendingEnabled = YES;
        sdfPipelineDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        sdfPipelineDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        sdfPipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        sdfPipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        sdfPipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        sdfPipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        sdfPipelineDesc.depthAttachmentPixelFormat = m_view.depthStencilPixelFormat;
        
        m_sdfPipeline = [m_device newRenderPipelineStateWithDescriptor:sdfPipelineDesc error:&error];
        if (error) {
            TraceLog(LOG_ERROR, "[METAL ERROR] Failed to create SDF pipeline: %@", error.localizedDescription);
        } else {
            // Metal debug logs removed for cleaner output
        }
        
        // Create instanced textured pipeline
        MTLRenderPipelineDescriptor* instancedTexturedPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        instancedTexturedPipelineDesc.label = @"Instanced Textured Pipeline";
        instancedTexturedPipelineDesc.vertexFunction = vertexInstancedFunction;
        instancedTexturedPipelineDesc.fragmentFunction = fragmentTexturedFunction;
        instancedTexturedPipelineDesc.vertexDescriptor = vertexDesc;
        instancedTexturedPipelineDesc.colorAttachments[0].pixelFormat = m_view.colorPixelFormat;
        instancedTexturedPipelineDesc.colorAttachments[0].blendingEnabled = YES;
        instancedTexturedPipelineDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        instancedTexturedPipelineDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        instancedTexturedPipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        instancedTexturedPipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        instancedTexturedPipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        instancedTexturedPipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        instancedTexturedPipelineDesc.depthAttachmentPixelFormat = m_view.depthStencilPixelFormat;
        
        m_instancedTexturePipeline = [m_device newRenderPipelineStateWithDescriptor:instancedTexturedPipelineDesc error:&error];
        if (error) {
            TraceLog(LOG_ERROR, "[METAL ERROR] Failed to create instanced textured pipeline: %@", error.localizedDescription);
        } else {
            TraceLog(LOG_INFO, "[METAL DEBUG] Instanced textured pipeline created successfully");
        }
        
        // Create instanced color-only pipeline
        MTLRenderPipelineDescriptor* instancedColorPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        instancedColorPipelineDesc.label = @"Instanced Color Pipeline";
        instancedColorPipelineDesc.vertexFunction = vertexInstancedFunction;
        instancedColorPipelineDesc.fragmentFunction = fragmentColorFunction;
        instancedColorPipelineDesc.vertexDescriptor = vertexDesc;
        instancedColorPipelineDesc.colorAttachments[0].pixelFormat = m_view.colorPixelFormat;
        instancedColorPipelineDesc.colorAttachments[0].blendingEnabled = YES;
        instancedColorPipelineDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        instancedColorPipelineDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        instancedColorPipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        instancedColorPipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        instancedColorPipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        instancedColorPipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        instancedColorPipelineDesc.depthAttachmentPixelFormat = m_view.depthStencilPixelFormat;
        
        m_instancedColorPipeline = [m_device newRenderPipelineStateWithDescriptor:instancedColorPipelineDesc error:&error];
        if (error) {
            TraceLog(LOG_ERROR, "[METAL ERROR] Failed to create instanced color pipeline: %@", error.localizedDescription);
        } else {
            TraceLog(LOG_INFO, "[METAL DEBUG] Instanced color pipeline created successfully");
        }
        
        // Create depth stencil state for game elements
        MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthDesc.depthCompareFunction = MTLCompareFunctionLessEqual;
        depthDesc.depthWriteEnabled = YES;
        m_depthStencilState = [m_device newDepthStencilStateWithDescriptor:depthDesc];
        
        // Create depth stencil state for UI elements (no depth testing)
        MTLDepthStencilDescriptor* uiDepthDesc = [[MTLDepthStencilDescriptor alloc] init];
        uiDepthDesc.depthCompareFunction = MTLCompareFunctionAlways;
        uiDepthDesc.depthWriteEnabled = NO;
        m_uiDepthStencilState = [m_device newDepthStencilStateWithDescriptor:uiDepthDesc];
        
        if (m_depthStencilState && m_uiDepthStencilState) {
            TraceLog(LOG_INFO, "[METAL DEBUG] Depth stencil states created successfully");
        } else {
            TraceLog(LOG_ERROR, "[METAL ERROR] Failed to create depth stencil states");
        }
        
        // Log pipeline creation summary
        if (m_texturePipeline && m_colorPipeline && m_sdfPipeline && m_instancedTexturePipeline && m_instancedColorPipeline && m_depthStencilState) {
            TraceLog(LOG_INFO, "[METAL DEBUG] All Metal pipelines created successfully - rendering should work");
        } else {
            TraceLog(LOG_ERROR, "[METAL ERROR] Some Metal pipelines failed to create - rendering will not work");
        }
    }
}

void MetalRenderer::CreateBuffers() {
    // The actual buffer creation is now handled by MetalFrameResources
    // Just initialize our local storage
    m_vertices.reserve(10000);
    m_currentVertexBufferOffset = 0;
}

void MetalRenderer::BeginFrame() {
    // Record frame start time for frame pacing
    m_frameStartTime = CACurrentMediaTime();
    
    // Begin new frame in the triple buffer system
    // This will wait on the GPU if needed to avoid resource conflicts
    m_frameResources.BeginFrame();
    
    // Reset per-frame state
    m_vertices.clear();
    m_drawCommands.clear();
    m_currentVertexBufferOffset = 0;
    m_currentInstanceBufferOffset = 0;
    
    // Reset debug stats for this frame
    if (m_debugVisualization) {
        ResetDebugStats();
        UpdateDebugStats();
    }
    
    // Create command buffer
    m_currentCommandBuffer = [m_commandQueue commandBuffer];
    m_currentCommandBuffer.label = [NSString stringWithFormat:@"Frame %d Command Buffer", 
                                 m_frameResources.GetCurrentFrameIndex()];
    

    // Get render pass descriptor from view
    m_currentRenderPass = m_view.currentRenderPassDescriptor;
    if (!m_currentRenderPass) {
        TraceLog(LOG_ERROR, "[METAL ERROR] BeginFrame: No render pass descriptor available");
        return;
    }
    
    // Metal debug logs removed for cleaner output
    
    // Create render encoder
    m_currentEncoder = [m_currentCommandBuffer renderCommandEncoderWithDescriptor:m_currentRenderPass];
    m_currentEncoder.label = @"Main Render Encoder";
    
    if (!m_currentEncoder) {
        TraceLog(LOG_ERROR, "[METAL ERROR] BeginFrame: Failed to create render command encoder");
        return;
    }
    
    // Metal debug logs removed for cleaner output
    
    // Set initial pipeline state
    [m_currentEncoder setDepthStencilState:m_depthStencilState];
    [m_currentEncoder setFragmentSamplerState:m_samplerState atIndex:0];
    
    // Set viewport to full drawable size
    MTLViewport viewport = {0, 0, m_view.drawableSize.width, m_view.drawableSize.height, 0, 1};
    [m_currentEncoder setViewport:viewport];
    // Metal debug logs removed for cleaner output
    
    // Update uniforms for this frame
    UpdateUniforms();
    
    // Validate render state if debugging
    if (m_debugVisualization) {
        ValidateRenderState();
    }
}

void MetalRenderer::EndFrame() {
    // Metal debug logs removed for cleaner output
    FlushBatch();

    // Draw debug overlay if enabled
    if (m_debugVisualization) {
        DrawDebugOverlay();
        RenderDebugInfo();
    }
    
    if (m_currentEncoder) {
        // End the render command encoder
        [m_currentEncoder endEncoding];
        m_currentEncoder = nullptr;
    }
}

void MetalRenderer::Present() {
    // Metal debug logs removed for cleaner output
    
    if (m_currentCommandBuffer && m_view.currentDrawable) {
        // Schedule presentation of the drawable
        [m_currentCommandBuffer presentDrawable:m_view.currentDrawable];
        // Metal debug logs removed for cleaner output
        
        // Add a completion handler to log command buffer status
        [m_currentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
            if (buffer.error) {
                TraceLog(LOG_ERROR, "[METAL ERROR] Command buffer failed with error: %@", buffer.error);
            } else {
                TraceLog(LOG_INFO, "[METAL INFO] Command buffer completed successfully.");
            }
        }];

        // Signal the completion of this frame
        m_frameResources.EndFrame(m_currentCommandBuffer);
        
        // Submit the command buffer to the GPU
        [m_currentCommandBuffer commit];
        // Metal debug logs removed for cleaner output
        
        m_currentCommandBuffer = nullptr;
        
        // Implement frame pacing if needed
        double frameEndTime = CACurrentMediaTime();
        double frameTime = frameEndTime - m_frameStartTime;
        if (frameTime < m_targetFrameTime) {
            // Simple frame pacing - sleep if we're ahead of schedule
            usleep((useconds_t)((m_targetFrameTime - frameTime) * 1000000));
        }
    } else {
        if (!m_currentCommandBuffer) {
            TraceLog(LOG_ERROR, "[METAL ERROR] Present: No command buffer available");
        }
        if (!m_view.currentDrawable) {
            TraceLog(LOG_ERROR, "[METAL ERROR] Present: No drawable available");
        }
    }
}

void MetalRenderer::Clear(Color color) {
    // Metal debug logs removed for cleaner output
    
    if (m_currentRenderPass) {
        m_currentRenderPass.colorAttachments[0].clearColor = MTLClearColorMake(
            color.r / 255.0f,
            color.g / 255.0f, 
            color.b / 255.0f,
            color.a / 255.0f
        );
        // Metal debug logs removed for cleaner output
    } else {
        TraceLog(LOG_ERROR, "[METAL ERROR] Clear: No render pass available");
    }
}

void MetalRenderer::SetProjectionMatrix(float width, float height) {
    // Use MTKView's drawableSize for pixel-accurate rendering
    CGSize drawableSize = m_view.drawableSize;
    m_projectionMatrix = MakeOrthoMatrix(0, drawableSize.width, drawableSize.height, 0, -1.0f, 1.0f);
    // Metal debug logs removed for cleaner output
}

void MetalRenderer::SetProjectionMatrixWithSafeArea(float screenWidth, float screenHeight, Rectangle safeArea) {
    // Create orthographic projection matrix that accounts for safe area
    // This ensures content is properly positioned within the safe area
    m_projectionMatrix = MakeOrthoMatrix(safeArea.x, safeArea.x + safeArea.width, 
                                        safeArea.y + safeArea.height, safeArea.y, -1.0f, 1.0f);
    // Metal debug logs removed for cleaner output
}

void MetalRenderer::UpdateUniforms() {
    // Allocate uniform buffer from the current frame's resources
    size_t offset = 0;
    MetalUniforms* uniforms = static_cast<MetalUniforms*>(
        m_frameResources.AllocateUniformBuffer(sizeof(MetalUniforms), &offset)
    );
    
    if (!uniforms) {
        TraceLog(LOG_ERROR, "[METAL ERROR] Failed to allocate uniform buffer");
        return;
    }
    
    // Update the uniform data
    uniforms->projectionMatrix = m_projectionMatrix;
    uniforms->distanceRange = 4.0f; // Match AtlasConfig::distanceRange
    uniforms->time = 0.0f; // Initialize time for future effects
    
    // Set the uniform buffer for rendering
    [m_currentEncoder setVertexBuffer:m_frameResources.GetCurrentUniformBuffer() 
                              offset:offset 
                             atIndex:1];
}

void MetalRenderer::DrawRectangle(float x, float y, float width, float height, Color color) {
        // Drawing debug logs removed for cleaner output
    size_t verticesBefore = m_vertices.size();
    size_t commandsBefore = m_drawCommands.size();
    AddRectangleVertices(x, y, width, height, color);
    DrawCommand cmd = CreateDrawCommand(MTLPrimitiveTypeTriangle, m_vertices.size() - 6, 6, nullptr, false, 
                                       RENDER_STATE_ALPHA_BLEND, 0.0f, "Rectangle");
    m_drawCommands.push_back(cmd);
        // Drawing debug logs removed for cleaner output
}

void MetalRenderer::AddRectangleVertices(float x, float y, float width, float height, Color color) {
    simd_float4 colorVec = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
    
    // Two triangles to make a rectangle
    // Triangle 1: top-left, bottom-left, top-right
    AddVertex(x, y, 0.0f, 0.0f, color);                    // top-left
    AddVertex(x, y + height, 0.0f, 1.0f, color);          // bottom-left  
    AddVertex(x + width, y, 1.0f, 0.0f, color);           // top-right
    
    // Triangle 2: top-right, bottom-left, bottom-right
    AddVertex(x + width, y, 1.0f, 0.0f, color);           // top-right
    AddVertex(x, y + height, 0.0f, 1.0f, color);          // bottom-left
    AddVertex(x + width, y + height, 1.0f, 1.0f, color);  // bottom-right
}

void MetalRenderer::AddVertex(float x, float y, float u, float v, Color color) {
    MetalVertex2D vertex;
    vertex.position = {x, y};
    vertex.texCoords = {u, v};
    vertex.color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
    
    m_vertices.push_back(vertex);
}

void MetalRenderer::FlushBatch() {
    TraceLog(LOG_INFO, "[METAL DEBUG] FlushBatch: vertices=%zu, commands=%zu, encoder=%p", 
             m_vertices.size(), m_drawCommands.size(), m_currentEncoder);
    
    if (m_vertices.empty() || !m_currentEncoder) {
        if (m_vertices.empty()) {
            TraceLog(LOG_WARNING, "[METAL DEBUG] FlushBatch: No vertices to flush");
        }
        if (!m_currentEncoder) {
            TraceLog(LOG_ERROR, "[METAL ERROR] FlushBatch: No render encoder available");
        }
        return;
    }
    
    // Calculate size of vertex data
    size_t dataSize = m_vertices.size() * sizeof(MetalVertex2D);
    
    // Allocate space in the current frame's vertex buffer
    size_t bufferOffset = 0;
    void* destinationBuffer = m_frameResources.AllocateVertexBuffer(dataSize, &bufferOffset);
    
    if (!destinationBuffer) {
        TraceLog(LOG_ERROR, "[METAL ERROR] FlushBatch: Failed to allocate vertex buffer space");
        return;
    }
    
    TraceLog(LOG_INFO, "[METAL DEBUG] FlushBatch: Allocated %zu bytes at offset %zu", dataSize, bufferOffset);
    
    // Copy vertex data to the frame's vertex buffer
    memcpy(destinationBuffer, m_vertices.data(), dataSize);
    
    // Set the vertex buffer for the render command encoder
    [m_currentEncoder setVertexBuffer:m_frameResources.GetCurrentVertexBuffer() 
                              offset:bufferOffset 
                             atIndex:0];
    
    // Store the current offset for later reference (when drawing)
    size_t vertexStartOffset = m_currentVertexBufferOffset;
    m_currentVertexBufferOffset += m_vertices.size();
    
    TraceLog(LOG_INFO, "[METAL DEBUG] FlushBatch: Updated vertex offsets, startOffset=%zu, newOffset=%zu", 
             vertexStartOffset, m_currentVertexBufferOffset);
    
    // Update draw commands to use buffer-relative offsets
    for (auto& cmd : m_drawCommands) {
        cmd.vertexStart += vertexStartOffset;
    }
    
    // Execute optimized draw commands with sorting and state management
    ExecuteOptimizedDrawCommands();
    
    // Clear batch data for the next batch
    m_vertices.clear();
    m_drawCommands.clear();
    
    TraceLog(LOG_INFO, "[METAL DEBUG] FlushBatch: Completed successfully, cleared batch data");
}

void MetalRenderer::ExecuteOptimizedDrawCommands() {
    if (m_drawCommands.empty()) {
        TraceLog(LOG_WARNING, "[METAL DEBUG] ExecuteOptimizedDrawCommands: No draw commands to execute");
        return;
    }
    
    TraceLog(LOG_INFO, "[METAL DEBUG] ExecuteOptimizedDrawCommands: Executing %zu draw commands", m_drawCommands.size());
    
    // Track current state to minimize state changes
    id<MTLRenderPipelineState> currentPipeline = nullptr;
    id<MTLTexture> currentTexture = nullptr;
    id<MTLDepthStencilState> currentDepthStencilState = nullptr;
    
    uint32_t drawCallCount = 0;
    
    for (const auto& cmd : m_drawCommands) {

        // Validate instance count to catch memory corruption early
        if (cmd.instanceCount > 1000000) {
            TraceLog(LOG_ERROR, "[METAL ERROR] ExecuteOptimizedDrawCommands: Suspicious instance count %lu for %s, possible memory corruption!", 
                     (unsigned long)cmd.instanceCount, cmd.debugName);
            // Continue execution but log the error
        }
        
        // Determine required pipeline
        id<MTLRenderPipelineState> requiredPipeline = nullptr;
        if (cmd.useTexture) {
            if (cmd.renderState & RENDER_STATE_SDF) {
                // Use SDF pipeline for grayscale textures
                requiredPipeline = m_sdfPipeline;
                TraceLog(LOG_INFO, "[METAL DEBUG] Using SDF pipeline for texture: %p", cmd.texture);
            } else {
                // Use regular textured pipeline for RGBA textures
            requiredPipeline = cmd.instanceCount > 1 ? m_instancedTexturePipeline : m_texturePipeline;
            }
        } else {
            requiredPipeline = cmd.instanceCount > 1 ? m_instancedColorPipeline : m_colorPipeline;
        }
        
        // Set depth stencil state based on layer
        id<MTLDepthStencilState> requiredDepthStencilState = nullptr;
        if (strcmp(cmd.debugName, "Background") == 0 || strcmp(cmd.debugName, "Midground") == 0 || 
            strcmp(cmd.debugName, "Foreground") == 0 || strcmp(cmd.debugName, "Logo") == 0 || 
            strcmp(cmd.debugName, "UI") == 0 || strcmp(cmd.debugName, "Text") == 0) {
            requiredDepthStencilState = m_uiDepthStencilState;
            TraceLog(LOG_INFO, "[METAL DEBUG] Using UI depth stencil state for %s", cmd.debugName);
        } else {
            requiredDepthStencilState = m_depthStencilState;
        }
        
        // Change depth stencil state if needed
        if (requiredDepthStencilState != currentDepthStencilState) {
            [m_currentEncoder setDepthStencilState:requiredDepthStencilState];
            currentDepthStencilState = requiredDepthStencilState;
            m_debugStats.stateChanges++;
            TraceLog(LOG_INFO, "[METAL DEBUG] Changed depth stencil state for %s", cmd.debugName);
        }
        
        // Change pipeline if needed
        if (requiredPipeline != currentPipeline) {
            [m_currentEncoder setRenderPipelineState:requiredPipeline];
            currentPipeline = requiredPipeline;
            m_debugStats.stateChanges++;
            TraceLog(LOG_INFO, "[METAL DEBUG] Set pipeline: %p for %s", requiredPipeline, cmd.debugName);
        }
        
        // Bind texture if needed and changed
        if (cmd.useTexture && cmd.texture) {
            if (cmd.texture != currentTexture) {
                BindTexture(cmd.texture);
                currentTexture = cmd.texture;
                m_debugStats.textureBinds++;
                TraceLog(LOG_INFO, "[METAL DEBUG] Bound texture: %p for %s", cmd.texture, cmd.debugName);
            }
        }
        
        // Draw primitives
        if (cmd.instanceCount > 1) {
            // Instanced rendering
            [m_currentEncoder drawPrimitives:cmd.primitiveType
                                 vertexStart:cmd.vertexStart
                                 vertexCount:cmd.vertexCount
                               instanceCount:cmd.instanceCount];
            m_debugStats.instancedCalls++;
            TraceLog(LOG_INFO, "[METAL DEBUG] Instanced draw: %lu vertices, %lu instances for %s", 
                     (unsigned long)cmd.vertexCount, (unsigned long)cmd.instanceCount, cmd.debugName);
        } else {
            // Regular rendering
            [m_currentEncoder drawPrimitives:cmd.primitiveType
                                 vertexStart:cmd.vertexStart
                                 vertexCount:cmd.vertexCount];
        }
        
        m_debugStats.batchedVertices += cmd.vertexCount;
        drawCallCount++;
    }
    
    TraceLog(LOG_INFO, "[METAL DEBUG] ExecuteOptimizedDrawCommands: Completed %u draw calls", drawCallCount);
}

void MetalRenderer::SortDrawCommands() {
    // Generate sort keys for all commands
    for (auto& cmd : m_drawCommands) {
        cmd.sortKey = GenerateSortKey(cmd);
    }
    
    // Sort commands by sort key
    std::sort(m_drawCommands.begin(), m_drawCommands.end(), 
              [](const DrawCommand& a, const DrawCommand& b) {
                  return a.sortKey < b.sortKey;
              });
    
    TraceLog(LOG_INFO, "[METAL DEBUG] SortDrawCommands: Sorted %zu draw commands", m_drawCommands.size());
    
    // Log the sorted order for debugging
    for (size_t i = 0; i < m_drawCommands.size(); ++i) {
        const auto& cmd = m_drawCommands[i];
        TraceLog(LOG_INFO, "[METAL DEBUG] Draw command %zu: %s, layer=%d, depth=%.2f, sortKey=0x%08x", 
                 i, cmd.debugName, (int)(cmd.depth / 0.1f), cmd.sortKey);
    }
}

uint32_t MetalRenderer::GenerateSortKey(const DrawCommand& cmd) {
    // Sort key format (32-bit):
    // Bits 31-24: Render state (8 bits)
    // Bits 23-16: Texture ID (8 bits)
    // Bits 15-8:  Primitive type (8 bits)
    // Bits 7-0:   Depth (8 bits, inverted for front-to-back)
    
    uint32_t sortKey = 0;
    
    // Render state (highest priority)
    sortKey |= (cmd.renderState & 0xFF) << 24;
    
    // Texture ID (second priority)
    uint32_t textureId = cmd.useTexture ? GetTextureHash(cmd.texture) : 0;
    sortKey |= (textureId & 0xFF) << 16;
    
    // Primitive type
    sortKey |= (static_cast<uint32_t>(cmd.primitiveType) & 0xFF) << 8;
    
    // Depth (inverted for front-to-back rendering)
    uint8_t depthByte = static_cast<uint8_t>(255 - std::min(255.0f, cmd.depth * 255.0f));
    sortKey |= depthByte;
    
    return sortKey;
}

void MetalRenderer::OptimizeDrawCommands() {
    if (m_drawCommands.size() < 2) return;
    
    // Try to batch adjacent commands with the same state
    std::vector<DrawCommand> optimizedCommands;
    optimizedCommands.reserve(m_drawCommands.size());
    
    for (size_t i = 0; i < m_drawCommands.size(); ++i) {
        DrawCommand& cmd = m_drawCommands[i];
        
        // Check if we can batch with the previous command
        if (!optimizedCommands.empty() && ShouldBatchCommands(optimizedCommands.back(), cmd)) {
            // Extend the previous command
            optimizedCommands.back().vertexCount += cmd.vertexCount;
        } else {
            // Add as new command
            optimizedCommands.push_back(cmd);
        }
    }
    
    // Replace original commands with optimized ones
    m_drawCommands = std::move(optimizedCommands);
}

bool MetalRenderer::ShouldBatchCommands(const DrawCommand& cmd1, const DrawCommand& cmd2) {
    // Can batch if:
    // 1. Same primitive type
    // 2. Same texture
    // 3. Same render state
    // 4. Sequential vertices
    // 5. Both non-instanced
    
    return cmd1.primitiveType == cmd2.primitiveType &&
           cmd1.texture == cmd2.texture &&
           cmd1.useTexture == cmd2.useTexture &&
           cmd1.renderState == cmd2.renderState &&
           cmd1.instanceCount <= 1 && cmd2.instanceCount <= 1 &&
           (cmd1.vertexStart + cmd1.vertexCount) == cmd2.vertexStart;
}

void MetalRenderer::SetRenderState(uint32_t renderState) {
    // Apply render state changes
    // Note: This is a simplified implementation
    // In a full implementation, you'd set depth state, blend state, etc.
    
    if (renderState & RENDER_STATE_ALPHA_BLEND) {
        // Alpha blending is already set up in pipeline creation
    }
    
    if (renderState & RENDER_STATE_DEPTH_TEST) {
        // Depth testing would be configured here
    }
    
    // Other render states would be handled here
}

void MetalRenderer::BindTexture(id<MTLTexture> texture) {
    if (texture) {
        [m_currentEncoder setFragmentTexture:texture atIndex:0];
        [m_currentEncoder setFragmentSamplerState:m_samplerState atIndex:0];
        m_currentTexture = texture; // Track current texture for UV normalization
    }
}

uint32_t MetalRenderer::GetTextureHash(id<MTLTexture> texture) {
    if (!texture) return 0;
    
    // Simple hash based on texture pointer
    // In a more sophisticated implementation, you might use texture properties
    uintptr_t ptr = reinterpret_cast<uintptr_t>((__bridge void*)texture);
    return static_cast<uint32_t>(ptr ^ (ptr >> 32));
}

// Matrix helper functions
simd_float4x4 MetalRenderer::MakeOrthoMatrix(float left, float right, float bottom, float top, float near, float far) {
    float width = right - left;
    float height = top - bottom;
    float depth = far - near;
    
    simd_float4x4 result = matrix_identity_float4x4;
    result.columns[0][0] = 2.0f / width;
    result.columns[1][1] = 2.0f / height;
    result.columns[2][2] = -2.0f / depth;
    result.columns[3][0] = -(right + left) / width;
    result.columns[3][1] = -(top + bottom) / height;
    result.columns[3][2] = -(far + near) / depth;
    
    return result;
}

simd_float4x4 MetalRenderer::MakeTranslationMatrix(float x, float y) {
    simd_float4x4 result = matrix_identity_float4x4;
    result.columns[3][0] = x;
    result.columns[3][1] = y;
    return result;
}

simd_float4x4 MetalRenderer::MakeRotationMatrix(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    
    simd_float4x4 result = matrix_identity_float4x4;
    // For screen coordinates (Y increases downward), we need to flip the Y component
    result.columns[0][0] = c;
    result.columns[0][1] = -s;  // Flip Y component for screen coordinates
    result.columns[1][0] = s;
    result.columns[1][1] = c;
    
    return result;
}

simd_float4x4 MetalRenderer::MakeScaleMatrix(float x, float y) {
    simd_float4x4 result = matrix_identity_float4x4;
    result.columns[0][0] = x;
    result.columns[1][1] = y;
    return result;
}

// Stub implementations for remaining functions
void MetalRenderer::DrawRectangleRounded(float x, float y, float width, float height, float roundness, int segments, Color color) {
    if (roundness <= 0 || segments <= 0) {
        DrawRectangle(x, y, width, height, color);
        return;
    }
    
    // Clamp roundness to reasonable values
    float maxRadius = fminf(width, height) * 0.5f;
    float radius = fminf(roundness, maxRadius);
    
    if (radius <= 0) {
        DrawRectangle(x, y, width, height, color);
        return;
    }
    
    // Calculate corner centers
    float left = x + radius;
    float right = x + width - radius;
    float top = y + radius;
    float bottom = y + height - radius;
    
    // Draw center rectangle
    DrawRectangle(left, y, right - left, height, color);
    
    // Draw left and right rectangles
    DrawRectangle(x, top, radius, bottom - top, color);
    DrawRectangle(right, top, radius, bottom - top, color);
    
    // Draw four rounded corners
    DrawRoundedCorner(left, top, radius, segments, 0, color);     // top-left
    DrawRoundedCorner(right, top, radius, segments, 1, color);    // top-right
    DrawRoundedCorner(right, bottom, radius, segments, 2, color); // bottom-right
    DrawRoundedCorner(left, bottom, radius, segments, 3, color);  // bottom-left
}

void MetalRenderer::DrawCircle(float x, float y, float radius, Color color) {
    // Implement circle using triangle fan
    const int segments = 32;
    
    // Center vertex
    AddVertex(x, y, 0.5f, 0.5f, color);
    
    // Create triangles around the circle
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i * 2.0f * M_PI / segments;
        float px = x + cosf(angle) * radius;
        float py = y + sinf(angle) * radius;
        
        AddVertex(px, py, 0.5f, 0.5f, color);
        
        if (i > 0) {
            // Add triangle: center, previous point, current point
            size_t centerIndex = m_vertices.size() - segments - 2;
            size_t prevIndex = m_vertices.size() - 2;
            size_t currIndex = m_vertices.size() - 1;
            
            DrawCommand cmd;
            cmd.primitiveType = MTLPrimitiveTypeTriangle;
            cmd.vertexStart = centerIndex;
            cmd.vertexCount = 3;
            cmd.texture = nullptr;
            cmd.useTexture = false;
            
            m_drawCommands.push_back(cmd);
        }
    }
}

void MetalRenderer::DrawLine(float x1, float y1, float x2, float y2, Color color) {
    // Use default thickness of 1.0f
    DrawLineEx(x1, y1, x2, y2, 1.0f, color);
}

void MetalRenderer::DrawLineEx(float x1, float y1, float x2, float y2, float thickness, Color color) {
    // Metal debug logs removed for cleaner output
    
    // Calculate perpendicular vector for thickness
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);
    
    if (length == 0) {
        // Metal debug logs removed for cleaner output
        return;
    }
    
    // Normalize and get perpendicular
    float nx = -dy / length * thickness * 0.5f;
    float ny = dx / length * thickness * 0.5f;
    
    // Four corners of the line rectangle
    float x1_top = x1 + nx;
    float y1_top = y1 + ny;
    float x1_bot = x1 - nx;
    float y1_bot = y1 - ny;
    float x2_top = x2 + nx;
    float y2_top = y2 + ny;
    float x2_bot = x2 - nx;
    float y2_bot = y2 - ny;
    
    // Add vertices for two triangles
    size_t startIdx = m_vertices.size();
    // Metal debug logs removed for cleaner output
    
    AddVertex(x1_top, y1_top, 0.5f, 0.5f, color);
    AddVertex(x1_bot, y1_bot, 0.5f, 0.5f, color);
    AddVertex(x2_top, y2_top, 0.5f, 0.5f, color);
    AddVertex(x2_bot, y2_bot, 0.5f, 0.5f, color);
    
    // Metal debug logs removed for cleaner output
    
    // First triangle: top-left, bottom-left, top-right
    DrawCommand cmd1 = CreateDrawCommand(MTLPrimitiveTypeTriangle, startIdx, 3, nullptr, false, 
                                        RENDER_STATE_ALPHA_BLEND, 0.0f, "Line");
    m_drawCommands.push_back(cmd1);
    
    // Second triangle: bottom-left, bottom-right, top-right
    DrawCommand cmd2 = CreateDrawCommand(MTLPrimitiveTypeTriangle, startIdx + 1, 3, nullptr, false, 
                                        RENDER_STATE_ALPHA_BLEND, 0.0f, "Line");
    m_drawCommands.push_back(cmd2);
    
    // Metal debug logs removed for cleaner output
}

void MetalRenderer::DrawTexture(id<MTLTexture> texture, Rectangle source, Rectangle dest, Color tint) {
    // Default to UI layer for backward compatibility
    DrawTexture(texture, source, dest, tint, RenderLayer::UI);
}

void MetalRenderer::DrawTexture(id<MTLTexture> texture, Rectangle source, Rectangle dest, Color tint, RenderLayer layer) {
        // Drawing debug logs removed for cleaner output
    
    if (!texture) {
        TraceLog(LOG_WARNING, "[METAL WARNING] DrawTexture called with a null texture.");
        return;
    }
    
    // Set current texture for UV normalization
    m_currentTexture = texture;
    
    AddTexturedRectangleVertices(dest, source, tint);
    
    DrawCommand cmd;
    cmd.primitiveType = MTLPrimitiveTypeTriangle;
    cmd.vertexStart = m_vertices.size() - 6;
    cmd.vertexCount = 6;
    cmd.texture = texture;
    cmd.useTexture = true;
    
    // Enhanced fields for sorting and optimization
    cmd.renderState = RENDER_STATE_ALPHA_BLEND;
    cmd.textureId = GetTextureHash(texture);
    cmd.depth = static_cast<float>(layer) * 0.1f; // Layer-based depth
    cmd.sortKey = 0; // Will be generated during sorting
    cmd.instanceCount = 1;
    cmd.instanceDataOffset = 0;
    
    // Set debug name based on layer
    switch (layer) {
        case RenderLayer::Background:
            cmd.debugName = "Background";
            break;
        case RenderLayer::Midground:
            cmd.debugName = "Midground";
            break;
        case RenderLayer::Foreground:
            cmd.debugName = "Foreground";
            break;
        case RenderLayer::Logo:
            cmd.debugName = "Logo";
            break;
        case RenderLayer::UI:
            cmd.debugName = "UI";
            break;
        case RenderLayer::Text:
            cmd.debugName = "Text";
            break;
    }
    
    // Drawing debug logs removed for cleaner output
    
    m_drawCommands.push_back(cmd);
}

void MetalRenderer::DrawTexture(id<MTLTexture> texture, Rectangle source, Rectangle dest, Color tint, RenderLayer layer, int textureFormat) {
    // Metal debug logs removed for cleaner output
    
    if (!texture) {
        TraceLog(LOG_WARNING, "[METAL WARNING] DrawTexture called with a null texture.");
        return;
    }
    
    // Set current texture for UV normalization
    m_currentTexture = texture;
    
    AddTexturedRectangleVertices(dest, source, tint);
    
    DrawCommand cmd;
    cmd.primitiveType = MTLPrimitiveTypeTriangle;
    cmd.vertexStart = m_vertices.size() - 6;
    cmd.vertexCount = 6;
    cmd.texture = texture;
    cmd.useTexture = true;
    
    // Enhanced fields for sorting and optimization
    cmd.renderState = RENDER_STATE_ALPHA_BLEND;
    cmd.textureId = GetTextureHash(texture);
    cmd.depth = static_cast<float>(layer) * 0.1f; // Layer-based depth
    cmd.sortKey = 0; // Will be generated during sorting
    cmd.instanceCount = 1;
    cmd.instanceDataOffset = 0;
    
    // Check if this is an SDF texture (grayscale format)
    bool isSdfTexture = (textureFormat == 1); // IOS_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    if (isSdfTexture) {
        cmd.renderState |= RENDER_STATE_SDF; // Add SDF flag to render state
        // Metal debug logs removed for cleaner output
    }
    
    // Set debug name based on layer
    switch (layer) {
        case RenderLayer::Background:
            cmd.debugName = "Background";
            break;
        case RenderLayer::Midground:
            cmd.debugName = "Midground";
            break;
        case RenderLayer::Foreground:
            cmd.debugName = "Foreground";
            break;
        case RenderLayer::Logo:
            cmd.debugName = "Logo";
            break;
        case RenderLayer::UI:
            cmd.debugName = "UI";
            break;
        case RenderLayer::Text:
            cmd.debugName = "Text";
            break;
    }
    
    // Metal debug logs removed for cleaner output
    
    m_drawCommands.push_back(cmd);
}

void MetalRenderer::AddTexturedRectangleVertices(Rectangle dest, Rectangle source, Color tint) {
    if (!m_currentTexture) {
        TraceLog(LOG_ERROR, "[METAL ERROR] No current texture set for UV normalization");
        return;
    }
    
    float texWidth = (float)m_currentTexture.width;
    float texHeight = (float)m_currentTexture.height;
    // Metal debug logs removed for cleaner output
    
    // Normalize texture coordinates by dividing by texture dimensions
    float u1 = source.x / texWidth;
    float v1 = source.y / texHeight;
    float u2 = (source.x + source.width) / texWidth;
    float v2 = (source.y + source.height) / texHeight;
    
    // Clamp UV coordinates to [0,1] range to prevent texture sampling issues
    u1 = std::max(0.0f, std::min(1.0f, u1));
    v1 = std::max(0.0f, std::min(1.0f, v1));
    u2 = std::max(0.0f, std::min(1.0f, u2));
    v2 = std::max(0.0f, std::min(1.0f, v2));
    
    TraceLog(LOG_INFO, "[METAL DEBUG] UVs: u1=%.2f, v1=%.2f, u2=%.2f, v2=%.2f", u1, v1, u2, v2);
    TraceLog(LOG_INFO, "[METAL DEBUG] Vertices: x1=%.1f, y1=%.1f, x2=%.1f, y2=%.1f", dest.x, dest.y, dest.x + dest.width, dest.y + dest.height);
    
    // Log coordinate system info for debugging
    Rectangle pixelScreenRect = UICoordinateSystem::GetPixelScreenRect();
    Rectangle safeAreaPx = UICoordinateSystem::GetSafeAreaRect(true);
    // Metal debug logs removed for cleaner output
    
    // Two triangles to make a rectangle (6 unique vertices)
    // Triangle 1: top-left, bottom-left, top-right
    AddVertex(dest.x, dest.y, u1, v1, tint);                           // top-left
    AddVertex(dest.x, dest.y + dest.height, u1, v2, tint);            // bottom-left  
    AddVertex(dest.x + dest.width, dest.y, u2, v1, tint);             // top-right
    
    // Triangle 2: top-right, bottom-left, bottom-right
    AddVertex(dest.x + dest.width, dest.y, u2, v1, tint);             // top-right
    AddVertex(dest.x, dest.y + dest.height, u1, v2, tint);            // bottom-left
    AddVertex(dest.x + dest.width, dest.y + dest.height, u2, v2, tint); // bottom-right
}

void MetalRenderer::AddTransformedTexturedQuad(const simd_float2 vertices[4], id<MTLTexture> texture, Color tint) {
    if (!texture) {
        TraceLog(LOG_ERROR, "[METAL ERROR] AddTransformedTexturedQuad: Null texture");
        return;
    }
    
    // Set current texture for UV normalization
    m_currentTexture = texture;
    
    // Use full texture coordinates (0,0) to (1,1)
    float u1 = 0.0f, v1 = 0.0f;
    float u2 = 1.0f, v2 = 1.0f;
    
    // Metal debug logs removed for cleaner output
    
    // Two triangles to make a quad using the pre-transformed vertices
    // Triangle 1: vertices[0], vertices[1], vertices[2] (top-left, top-right, bottom-left)
    AddVertex(vertices[0].x, vertices[0].y, u1, v1, tint);  // top-left
    AddVertex(vertices[1].x, vertices[1].y, u2, v1, tint);  // top-right
    AddVertex(vertices[2].x, vertices[2].y, u1, v2, tint);  // bottom-left
    
    // Triangle 2: vertices[1], vertices[2], vertices[3] (top-right, bottom-left, bottom-right)
    AddVertex(vertices[1].x, vertices[1].y, u2, v1, tint);  // top-right
    AddVertex(vertices[2].x, vertices[2].y, u1, v2, tint);  // bottom-left
    AddVertex(vertices[3].x, vertices[3].y, u2, v2, tint);  // bottom-right
}

void MetalRenderer::DrawTextureEx(id<MTLTexture> texture, Vector2 position, float rotation, float scale, Color tint) {
    if (!texture) {
        // Metal debug logs removed for cleaner output
        return;
    }
    
    // Get texture dimensions
    float width = texture.width;
    float height = texture.height;
    
    // Metal debug logs removed for cleaner output
    
    // Calculate transform matrix on CPU
    // Apply transformations in the correct order for center pivot rotation:
    // 1. Translate to the desired position (center of the scaled texture)
    float centerX = position.x + (width * scale) * 0.5f;
    float centerY = position.y + (height * scale) * 0.5f;
    
    simd_float4x4 transform = MakeTranslationMatrix(centerX, centerY);
    
    // 2. Rotate around the center
    if (rotation != 0) {
        transform = simd_mul(transform, MakeRotationMatrix(rotation));
        // Metal debug logs removed for cleaner output
    }
    
    // 3. Scale the texture
    if (scale != 1.0f) {
        transform = simd_mul(transform, MakeScaleMatrix(scale, scale));
        // Metal debug logs removed for cleaner output
    }
    
    // 4. Translate back so the texture is centered at origin before scaling
    float offsetX = -width * 0.5f;
    float offsetY = -height * 0.5f;
    transform = simd_mul(transform, MakeTranslationMatrix(offsetX, offsetY));
    
    // Metal debug logs removed for cleaner output
    
    // Transform vertices on CPU
    simd_float4 localVertices[4] = {
        simd_make_float4(0, 0, 0, 1),           // top-left
        simd_make_float4(width, 0, 0, 1),       // top-right
        simd_make_float4(0, height, 0, 1),      // bottom-left
        simd_make_float4(width, height, 0, 1)   // bottom-right
    };
    
    simd_float2 transformedVertices[4];
    for (int i = 0; i < 4; i++) {
        simd_float4 transformed = simd_mul(transform, localVertices[i]);
        transformedVertices[i] = simd_make_float2(transformed.x, transformed.y);
    }
    
    // Metal debug logs removed for cleaner output
    
    // Submit pre-transformed vertices to GPU
    AddTransformedTexturedQuad(transformedVertices, texture, tint);
    
    // Create draw command
    DrawCommand cmd;
    cmd.primitiveType = MTLPrimitiveTypeTriangle;
    cmd.vertexStart = m_vertices.size() - 6;
    cmd.vertexCount = 6;
    cmd.texture = texture;
    cmd.useTexture = true;
    cmd.renderState = RENDER_STATE_ALPHA_BLEND;
    cmd.textureId = GetTextureHash(texture);
    cmd.depth = 0.0f;
    cmd.sortKey = 0;
    cmd.instanceCount = 1;
    cmd.instanceDataOffset = 0;
    cmd.debugName = "DrawTextureEx (CPU)";
    
    m_drawCommands.push_back(cmd);
    
    // Metal debug logs removed for cleaner output
}

void MetalRenderer::DrawText(const char* text, float x, float y, float fontSize, Color color) {
    // TraceLog(LOG_INFO, "[METAL DEBUG] DrawText called: text='%s', x=%.2f, y=%.2f, fontSize=%.2f, color=(%d,%d,%d,%d)", text, x, y, fontSize, color.r, color.g, color.b, color.a);
    
    // Log coordinate system info for text positioning
    Rectangle pixelScreenRect = UICoordinateSystem::GetPixelScreenRect();
    Rectangle safeAreaPx = UICoordinateSystem::GetSafeAreaRect(true);
    // Metal debug logs removed for cleaner output
    
    if (!g_textRenderer) {
        TraceLog(LOG_ERROR, "[METAL ERROR] g_textRenderer is not initialized!");
        return;
    }
    
    Font font = g_textRenderer->GetDefaultFont();
    // TraceLog(LOG_INFO, "[METAL DEBUG] Default font pointer: %p, ctFont: %p", &font, font.ctFont);
    
    id<MTLTexture> textTexture = g_textRenderer->RenderTextToTexture(text, (int)fontSize, color);
    if (!textTexture) {
        TraceLog(LOG_ERROR, "[METAL ERROR] Failed to render text to texture for '%s' (font.ctFont=%p)", text, font.ctFont);
        return;
    }
    
    float width = textTexture.width;
    float height = textTexture.height;
    
    // Ensure source rectangle matches actual texture dimensions to prevent UV coordinate issues
    Rectangle source = {0, 0, (float)textTexture.width, (float)textTexture.height};
    Rectangle dest = {x, y, (float)width, (float)height};
    
    // Metal debug logs removed for cleaner output
    
    // Log text texture details
    // Metal debug logs removed for cleaner output
    
    DrawTexture(textTexture, source, dest, WHITE, RenderLayer::Text);
}

void MetalRenderer::DrawText(const char* text, float x, float y, float fontSize, Color color, Font* font) {
    // TraceLog(LOG_INFO, "[METAL DEBUG] DrawText with font called: text='%s', x=%.2f, y=%.2f, fontSize=%.2f, color=(%d,%d,%d,%d), font=%p", 
    //          text, x, y, fontSize, color.r, color.g, color.b, color.a, font);
    
    // Log coordinate system info for text positioning
    Rectangle pixelScreenRect = UICoordinateSystem::GetPixelScreenRect();
    Rectangle safeAreaPx = UICoordinateSystem::GetSafeAreaRect(true);
    // TraceLog(LOG_INFO, "[METAL DEBUG] Text Coordinate System: screen=%.1fx%.1f, safeArea=(%.1f,%.1f,%.1f,%.1f)", 
    //          pixelScreenRect.width, pixelScreenRect.height, safeAreaPx.x, safeAreaPx.y, safeAreaPx.width, safeAreaPx.height);
    
    if (!g_textRenderer) {
        TraceLog(LOG_ERROR, "[METAL ERROR] g_textRenderer is not initialized!");
        return;
    }
    
    if (!font) {
        TraceLog(LOG_WARNING, "[METAL WARNING] Font is null, falling back to default font");
        DrawText(text, x, y, fontSize, color);
        return;
    }
    
    // TraceLog(LOG_INFO, "[METAL DEBUG] Using provided font: ctFont=%p, glyphCount=%d, baseSize=%d", 
    //          font->ctFont, font->glyphCount, font->baseSize);
    
    id<MTLTexture> textTexture = g_textRenderer->RenderTextToTexture(text, (int)fontSize, color, font);
    if (!textTexture) {
        TraceLog(LOG_ERROR, "[METAL ERROR] Failed to render text to texture for '%s' with provided font (font.ctFont=%p)", text, font->ctFont);
        return;
    }
    
    float width = textTexture.width;
    float height = textTexture.height;
    
    // Ensure source rectangle matches actual texture dimensions to prevent UV coordinate issues
    Rectangle source = {0, 0, (float)textTexture.width, (float)textTexture.height};
    Rectangle dest = {x, y, (float)width, (float)height};
    
    // TraceLog(LOG_INFO, "[METAL DEBUG] DrawText with font: source=(%.1f,%.1f,%.1f,%.1f), dest=(%.1f,%.1f,%.1f,%.1f)", 
    //          source.x, source.y, source.width, source.height, dest.x, dest.y, dest.width, dest.height);
    
    // Log text texture details
    // TraceLog(LOG_INFO, "[METAL DEBUG] Text texture with font: %p, size=%fx%f, text='%s'", textTexture, width, height, text);
    
    DrawTexture(textTexture, source, dest, WHITE, RenderLayer::Text);
}

// Matrix stack methods removed - using CPU vertex transformation instead

void MetalRenderer::DrawRoundedCorner(float centerX, float centerY, float radius, int segments, int corner, Color color) {
    TraceLog(LOG_INFO, "[METAL DEBUG] DrawRoundedCorner: center=(%.1f,%.1f), radius=%.1f, segments=%d, corner=%d, color=(%d,%d,%d,%d)", 
             centerX, centerY, radius, segments, corner, color.r, color.g, color.b, color.a);
    
    // Calculate angle range for this corner
    float startAngle, endAngle;
    switch (corner) {
        case 0: // top-left
            startAngle = M_PI;
            endAngle = M_PI * 1.5f;
            break;
        case 1: // top-right
            startAngle = M_PI * 1.5f;
            endAngle = M_PI * 2.0f;
            break;
        case 2: // bottom-right
            startAngle = 0;
            endAngle = M_PI * 0.5f;
            break;
        case 3: // bottom-left
            startAngle = M_PI * 0.5f;
            endAngle = M_PI;
            break;
        default:
            TraceLog(LOG_WARNING, "[METAL DEBUG] DrawRoundedCorner: Invalid corner %d", corner);
            return;
    }
    
    TraceLog(LOG_INFO, "[METAL DEBUG] DrawRoundedCorner: angle range %.2f to %.2f", startAngle, endAngle);
    
    // Center vertex
    size_t centerIndex = m_vertices.size();
    AddVertex(centerX, centerY, 0.5f, 0.5f, color);
    
    // Create triangles for the corner arc
    for (int i = 0; i <= segments; i++) {
        float t = (float)i / segments;
        float angle = startAngle + (endAngle - startAngle) * t;
        float px = centerX + cosf(angle) * radius;
        float py = centerY + sinf(angle) * radius;
        
        AddVertex(px, py, 0.5f, 0.5f, color);
        
        if (i > 0) {
            // Add triangle: center, previous point, current point
            size_t centerIndex = m_vertices.size() - segments - 2;
            size_t prevIndex = m_vertices.size() - 2;
            size_t currIndex = m_vertices.size() - 1;
            
            DrawCommand cmd = CreateDrawCommand(MTLPrimitiveTypeTriangle, centerIndex, 3, nullptr, false, 
                                               RENDER_STATE_ALPHA_BLEND, 0.0f, "RoundedCorner");
            m_drawCommands.push_back(cmd);
        }
    }
    
    TraceLog(LOG_INFO, "[METAL DEBUG] DrawRoundedCorner: Added %d triangles, total vertices=%zu, total commands=%zu", 
             segments, m_vertices.size(), m_drawCommands.size());
}

void MetalRenderer::DrawRoundedCornerLines(float centerX, float centerY, float radius, int segments, int corner, float lineThick, Color color) {
    // Calculate angle range for this corner
    float startAngle, endAngle;
    switch (corner) {
        case 0: // top-left
            startAngle = M_PI;
            endAngle = M_PI * 1.5f;
            break;
        case 1: // top-right
            startAngle = M_PI * 1.5f;
            endAngle = M_PI * 2.0f;
            break;
        case 2: // bottom-right
            startAngle = 0;
            endAngle = M_PI * 0.5f;
            break;
        case 3: // bottom-left
            startAngle = M_PI * 0.5f;
            endAngle = M_PI;
            break;
        default:
            return;
    }
    
    // Draw line segments along the arc
    for (int i = 0; i < segments; i++) {
        float t1 = (float)i / segments;
        float t2 = (float)(i + 1) / segments;
        
        float angle1 = startAngle + (endAngle - startAngle) * t1;
        float angle2 = startAngle + (endAngle - startAngle) * t2;
        
        float x1 = centerX + cosf(angle1) * radius;
        float y1 = centerY + sinf(angle1) * radius;
        float x2 = centerX + cosf(angle2) * radius;
        float y2 = centerY + sinf(angle2) * radius;
        
        DrawLineEx(x1, y1, x2, y2, lineThick, color);
    }
}

// Instanced rendering implementation
void MetalRenderer::BeginInstancedBatch() {
    m_instanceData.clear();
    m_currentInstanceBufferOffset = 0;
}

void MetalRenderer::EndInstancedBatch() {
    FlushInstancedBatch();
}

void MetalRenderer::AddInstanceData(const InstanceData& instanceData) {
    m_instanceData.push_back(instanceData);
}

void MetalRenderer::FlushInstancedBatch() {
    if (m_instanceData.empty()) return;
    
    // Calculate size of instance data
    size_t dataSize = m_instanceData.size() * sizeof(InstanceData);
    
    // Allocate space in the current frame's uniform buffer for instance data
    size_t bufferOffset = 0;
    void* destinationBuffer = m_frameResources.AllocateUniformBuffer(dataSize, &bufferOffset);
    
    if (!destinationBuffer) {
        TraceLog(LOG_ERROR, "[METAL ERROR] Failed to allocate instance buffer space");
        return;
    }
    
    // Copy instance data to the frame's uniform buffer
    memcpy(destinationBuffer, m_instanceData.data(), dataSize);
    
    // Set the instance buffer for the render command encoder
    [m_currentEncoder setVertexBuffer:m_frameResources.GetCurrentUniformBuffer() 
                              offset:bufferOffset 
                             atIndex:2]; // Using index 2 for instance data
    
    m_currentInstanceBufferOffset = bufferOffset;
    
    // Clear instance data for the next batch
    m_instanceData.clear();
}

void MetalRenderer::DrawInstanced(id<MTLTexture> texture, uint32_t instanceCount, uint32_t renderState) {
    TraceLog(LOG_INFO, "[METAL DEBUG] DrawInstanced: texture=%p, instances=%lu, renderState=0x%08x", 
             texture, (unsigned long)instanceCount, renderState);
    
    if (instanceCount == 0) {
        TraceLog(LOG_WARNING, "[METAL DEBUG] DrawInstanced: Zero instances, skipping");
        return;
    }
    
    // Create instanced draw command
    DrawCommand cmd = CreateDrawCommand(MTLPrimitiveTypeTriangle, m_vertices.size() - 6, 6, texture, 
                                       (texture != nullptr), renderState | RENDER_STATE_INSTANCED, 
                                       0.0f, "Instanced Draw", instanceCount);
    cmd.instanceDataOffset = m_currentInstanceBufferOffset;
    cmd.textureId = GetTextureHash(texture);
    
    m_drawCommands.push_back(cmd);
    
    TraceLog(LOG_INFO, "[METAL DEBUG] DrawInstanced: Added instanced command, total commands=%zu, instanceDataOffset=%zu", 
             m_drawCommands.size(), m_currentInstanceBufferOffset);
}

void MetalRenderer::DrawInstancedRectangles(const std::vector<Rectangle>& rects, const std::vector<Color>& colors) {
    if (rects.empty() || colors.empty()) return;
    
    BeginInstancedBatch();
    
    // Add a single rectangle to the vertex buffer
    if (!rects.empty()) {
        const Rectangle& rect = rects[0];
        Color baseColor = colors.empty() ? Color{255, 255, 255, 255} : colors[0];
        AddRectangleVertices(0, 0, rect.width, rect.height, baseColor);
    }
    
    // Add instance data for each rectangle
    for (size_t i = 0; i < rects.size(); ++i) {
        const Rectangle& rect = rects[i];
        Color color = (i < colors.size()) ? colors[i] : Color{255, 255, 255, 255};
        
        InstanceData instanceData;
        // Create translation matrix for this instance
        instanceData.modelMatrix = MakeTranslationMatrix(rect.x, rect.y);
        instanceData.color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
        instanceData.texCoordScale = {1.0f, 1.0f, 0.0f, 0.0f}; // No texture scaling
        
        AddInstanceData(instanceData);
    }
    
    // Draw all instances
    DrawInstanced(nullptr, static_cast<uint32_t>(rects.size()), RENDER_STATE_ALPHA_BLEND);
    
    EndInstancedBatch();
}

void MetalRenderer::DrawInstancedTextures(id<MTLTexture> texture, const std::vector<Rectangle>& sources, 
                                         const std::vector<Rectangle>& dests, const std::vector<Color>& tints) {
    if (dests.empty() || !texture) return;
    
    BeginInstancedBatch();
    
    // Add a single textured rectangle to the vertex buffer
    if (!dests.empty()) {
        const Rectangle& dest = dests[0];
        Rectangle source = sources.empty() ? Rectangle{0, 0, 1, 1} : sources[0];
        Color tint = tints.empty() ? Color{255, 255, 255, 255} : tints[0];
        AddTexturedRectangleVertices(Rectangle{0, 0, dest.width, dest.height}, source, tint);
    }
    
    // Add instance data for each texture
    for (size_t i = 0; i < dests.size(); ++i) {
        const Rectangle& dest = dests[i];
        Rectangle source = (i < sources.size()) ? sources[i] : Rectangle{0, 0, 1, 1};
        Color tint = (i < tints.size()) ? tints[i] : Color{255, 255, 255, 255};
        
        InstanceData instanceData;
        // Create translation matrix for this instance
        instanceData.modelMatrix = MakeTranslationMatrix(dest.x, dest.y);
        instanceData.color = {tint.r / 255.0f, tint.g / 255.0f, tint.b / 255.0f, tint.a / 255.0f};
        instanceData.texCoordScale = {source.width, source.height, source.x, source.y};
        
        AddInstanceData(instanceData);
    }
    
    // Draw all instances
    DrawInstanced(texture, static_cast<uint32_t>(dests.size()), RENDER_STATE_ALPHA_BLEND);
    
    EndInstancedBatch();
}

// Debug visualization implementation
void MetalRenderer::EnableDebugVisualization(bool enable) {
    if (enable && !m_debugVisualization) {
        InitializeDebugVisualization();
    }
    m_debugVisualization = enable;
}

void MetalRenderer::InitializeDebugVisualization() {
    if (!m_device) return;
    
    // Create debug vertex buffer
    m_debugVertexBuffer = [m_device newBufferWithLength:1024 * sizeof(MetalVertex2D) 
                                                 options:MTLResourceStorageModeShared];
    [m_debugVertexBuffer setLabel:@"Debug Vertex Buffer"];
    
    // Reset debug stats
    ResetDebugStats();
    
    TraceLog(LOG_INFO, "[METAL INFO] Debug visualization initialized");
}

void MetalRenderer::ResetDebugStats() {
    m_debugStats = DebugStats{};
}

void MetalRenderer::UpdateDebugStats() {
    // Update frame timing
    static double lastFrameTime = CACurrentMediaTime();
    double currentTime = CACurrentMediaTime();
    m_debugStats.frameTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;
}

void MetalRenderer::DrawDebugOverlay() {
    if (!m_debugVisualization || !m_currentEncoder) return;
    
    // Draw performance stats as colored rectangles and text
    float y = 10.0f;
    float lineHeight = 20.0f;
    
    // Background for stats
    DrawRectangle(10, y, 300, 160, {0, 0, 0, 128});
    y += 10;
    
    // Draw call count indicator
    Color drawCallColor = m_debugStats.drawCalls > 100 ? Color{255, 0, 0, 255} : Color{0, 255, 0, 255};
    DrawRectangle(20, y, 10, 10, drawCallColor);
    y += lineHeight;
    
    // State change indicator
    Color stateChangeColor = m_debugStats.stateChanges > 50 ? Color{255, 255, 0, 255} : Color{0, 255, 0, 255};
    DrawRectangle(20, y, 10, 10, stateChangeColor);
    y += lineHeight;
    
    // Texture bind indicator
    Color textureBindColor = m_debugStats.textureBinds > 50 ? Color{255, 128, 0, 255} : Color{0, 255, 0, 255};
    DrawRectangle(20, y, 10, 10, textureBindColor);
    y += lineHeight;
    
    // Frame time bar (target 16.67ms for 60fps)
    float frameTimeMs = m_debugStats.frameTime * 1000.0f;
    float barWidth = std::min(200.0f, frameTimeMs * 12.0f); // Scale factor
    Color frameTimeColor = frameTimeMs > 16.67f ? Color{255, 0, 0, 255} : Color{0, 255, 0, 255};
    DrawRectangle(20, y, barWidth, 10, frameTimeColor);
    y += lineHeight;
    
    // Memory usage indicator (simplified)
    DrawRectangle(20, y, 150, 10, {0, 0, 255, 255});
    
    // Draw safe area visualization
    Rectangle safeArea = UICoordinateSystem::GetSafeAreaRect(true);
    DrawRectangleRoundedLines(safeArea.x, safeArea.y, safeArea.width, safeArea.height, 5.0f, 8, 2.0f, RED);
    
    // Draw screen bounds
    Rectangle screenRect = UICoordinateSystem::GetPixelScreenRect();
    DrawRectangleRoundedLines(0, 0, screenRect.width, screenRect.height, 0, 8, 1.0f, GREEN);
    
    // Draw viewport info
    char viewportText[128];
    snprintf(viewportText, sizeof(viewportText), 
             "Viewport: %.0fx%.0f\nDrawable: %.0fx%.0f", 
             m_view.bounds.size.width, m_view.bounds.size.height,
             m_view.drawableSize.width, m_view.drawableSize.height);
    DrawText(viewportText, 10, 180, 14, YELLOW);
}

void MetalRenderer::RenderDebugInfo() {
    if (!m_debugVisualization) return;
    
    // This would typically render text overlays with detailed stats
    // For now, we'll just use the visual indicators
    
    // In a full implementation, you would:
    // 1. Render text showing exact numbers
    // 2. Show frame time graphs
    // 3. Display memory usage
    // 4. Show GPU utilization
    // 5. Display texture atlas usage
    
    TraceLog(LOG_INFO, "[METAL DEBUG] Frame Stats - Draws: %u, States: %u, Textures: %u, Instances: %u, Vertices: %u, Time: %.2fms",
          m_debugStats.drawCalls, m_debugStats.stateChanges, m_debugStats.textureBinds,
          m_debugStats.instancedCalls, m_debugStats.batchedVertices, m_debugStats.frameTime * 1000.0f);
}

void MetalRenderer::ValidateRenderState() {
    if (!m_debugVisualization) return;
    
    // Validate that the current render state is consistent
    if (!m_currentEncoder) {
        TraceLog(LOG_WARNING, "[METAL WARNING] No current render encoder set");
        return;
    }
    
    // Check for excessive state changes
    if (m_debugStats.stateChanges > 100) {
        TraceLog(LOG_WARNING, "[METAL WARNING] High number of state changes: %u", m_debugStats.stateChanges);
    }
    
    // Check for excessive draw calls
    if (m_debugStats.drawCalls > 200) {
        TraceLog(LOG_WARNING, "[METAL WARNING] High number of draw calls: %u", m_debugStats.drawCalls);
    }
    
    // Check for excessive texture binds
    if (m_debugStats.textureBinds > 100) {
        TraceLog(LOG_WARNING, "[METAL WARNING] High number of texture binds: %u", m_debugStats.textureBinds);
    }
}

// Mobile GPU optimization implementation
void MetalRenderer::SetMobileGPUSettings(const MobileGPUSettings& settings) {
    m_mobileSettings = settings;
    
    TraceLog(LOG_INFO, "[METAL INFO] Mobile GPU settings updated - MaxDrawCalls: %u, MaxTextures: %u, Mipmaps: %s",
          settings.maxDrawCallsPerFrame, settings.maxTextureBindsPerFrame,
          settings.enableMipmapping ? "YES" : "NO");
}

void MetalRenderer::OptimizeForDevice() {
    // Get device capabilities
    NSString* deviceName = m_device.name;
    bool isA12OrLater = [deviceName containsString:@"A12"] || [deviceName containsString:@"A13"] || 
                        [deviceName containsString:@"A14"] || [deviceName containsString:@"A15"] ||
                        [deviceName containsString:@"A16"] || [deviceName containsString:@"A17"] ||
                        [deviceName containsString:@"M1"] || [deviceName containsString:@"M2"];
    
    // Configure settings based on device capabilities
    MobileGPUSettings settings;
    
    if (isA12OrLater) {
        // Modern devices can handle more draw calls and features
        settings.enableMipmapping = true;
        settings.preferLowPowerGPU = false;
        settings.maxDrawCallsPerFrame = 500;
        settings.maxTextureBindsPerFrame = 200;
        settings.vertexBufferSize = 512 * 1024; // 512KB
        settings.uniformBufferSize = 64 * 1024;  // 64KB
        settings.enableEarlyZTest = true;
        settings.enableOcclusionCulling = true;
    } else {
        // Older devices need more conservative settings
        settings.enableMipmapping = false;
        settings.preferLowPowerGPU = true;
        settings.maxDrawCallsPerFrame = 200;
        settings.maxTextureBindsPerFrame = 100;
        settings.vertexBufferSize = 256 * 1024; // 256KB
        settings.uniformBufferSize = 32 * 1024;  // 32KB
        settings.enableEarlyZTest = false;
        settings.enableOcclusionCulling = false;
    }
    
    SetMobileGPUSettings(settings);
    
    TraceLog(LOG_INFO, "[METAL INFO] Optimized for device: %@ (Modern GPU: %s)", deviceName, isA12OrLater ? "YES" : "NO");
}

void MetalRenderer::DrawRectangleRoundedLines(float x, float y, float width, float height, float roundness, int segments, float lineThick, Color color) {
    TraceLog(LOG_INFO, "[METAL DEBUG] DrawRectangleRoundedLines: rect=(%.1f,%.1f,%.1f,%.1f), roundness=%.1f, lineThick=%.1f, color=(%d,%d,%d,%d)", 
          x, y, width, height, roundness, lineThick, color.r, color.g, color.b, color.a);
    
    // Clamp roundness to reasonable values
    float maxRadius = fminf(width, height) * 0.5f;
    float radius = fminf(roundness, maxRadius);
    
    if (radius <= 0) {
        // Draw regular rectangle outline
        DrawLineEx(x, y, x + width, y, lineThick, color);                    // Top
        DrawLineEx(x + width, y, x + width, y + height, lineThick, color);   // Right
        DrawLineEx(x + width, y + height, x, y + height, lineThick, color);  // Bottom
        DrawLineEx(x, y + height, x, y, lineThick, color);                   // Left
        return;
    }
    
    // Calculate corner centers
    float left = x + radius;
    float right = x + width - radius;
    float top = y + radius;
    float bottom = y + height - radius;
    
    // Draw straight edges
    DrawLineEx(left, y, right, y, lineThick, color);                    // Top
    DrawLineEx(right, top, right, bottom, lineThick, color);            // Right
    DrawLineEx(right, y + height, left, y + height, lineThick, color);  // Bottom
    DrawLineEx(left, bottom, left, top, lineThick, color);              // Left
    
    // Draw rounded corners using line segments
    DrawRoundedCornerLines(left, top, radius, segments, 0, lineThick, color);     // top-left
    DrawRoundedCornerLines(right, top, radius, segments, 1, lineThick, color);    // top-right
    DrawRoundedCornerLines(right, bottom, radius, segments, 2, lineThick, color); // bottom-right
    DrawRoundedCornerLines(left, bottom, radius, segments, 3, lineThick, color);  // bottom-left
}

// Helper function to properly initialize DrawCommand objects
DrawCommand MetalRenderer::CreateDrawCommand(MTLPrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount, 
                                            id<MTLTexture> texture, bool useTexture, uint32_t renderState, 
                                            float depth, const char* debugName, uint32_t instanceCount) {
    DrawCommand cmd;
    cmd.primitiveType = primitiveType;
    cmd.vertexStart = vertexStart;
    cmd.vertexCount = vertexCount;
    cmd.texture = texture;
    cmd.useTexture = useTexture;
    cmd.renderState = renderState;
    cmd.textureId = useTexture ? GetTextureHash(texture) : 0;
    cmd.depth = depth;
    cmd.sortKey = 0; // Will be generated during sorting
    cmd.instanceCount = instanceCount;
    cmd.instanceDataOffset = 0;
    cmd.debugName = debugName;
    

    
    return cmd;
}

#endif // defined(__APPLE__) && TARGET_OS_IOS 