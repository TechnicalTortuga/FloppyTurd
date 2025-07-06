#import "MetalRenderer.h"
#import <simd/simd.h>

#if defined(__APPLE__) && TARGET_OS_IOS

// Global renderer instance
MetalRenderer* g_metalRenderer = nullptr;

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
    m_currentMatrix = matrix_identity_float4x4;
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
            NSLog(@"Failed to get Metal device");
            return false;
        }
        
        // Create command queue
        m_commandQueue = [m_device newCommandQueue];
        if (!m_commandQueue) {
            NSLog(@"Failed to create command queue");
            return false;
        }
        
        // Set up view properties
        view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
        view.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        
        // Initialize frame resources for triple buffering
        if (!m_frameResources.Initialize(m_device)) {
            NSLog(@"Failed to initialize frame resources");
            return false;
        }
        
        // Create render pipelines
        CreatePipelines();
        
        // Create buffers
        CreateBuffers();
        
        // Create sampler state
        MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
        samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.mipFilter = MTLSamplerMipFilterLinear;
        samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
        m_samplerState = [m_device newSamplerStateWithDescriptor:samplerDesc];
        
        // Set up initial projection matrix
        CGSize size = view.drawableSize;
        SetProjectionMatrix(size.width, size.height);
        
        // Optimize for current device
        OptimizeForDevice();
        
        NSLog(@"MetalRenderer initialized successfully");
        return true;
    }
}

void MetalRenderer::Shutdown() {
    FlushBatch();
    
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
    m_matrixStack.clear();
}

void MetalRenderer::CreatePipelines() {
    @autoreleasepool {
        NSError* error = nil;
        
        // Load shader library
        NSString* shaderPath = [[NSBundle mainBundle] pathForResource:@"Shaders2D" ofType:@"metal"];
        NSString* shaderSource = [NSString stringWithContentsOfFile:shaderPath encoding:NSUTF8StringEncoding error:&error];
        
        id<MTLLibrary> library;
        if (shaderSource) {
            library = [m_device newLibraryWithSource:shaderSource options:nil error:&error];
        } else {
            // Try to load default library
            library = [m_device newDefaultLibrary];
        }
        
        if (!library) {
            NSLog(@"Failed to load shader library: %@", error.localizedDescription);
            return;
        }
        
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_shader_2d"];
        id<MTLFunction> fragmentTexturedFunction = [library newFunctionWithName:@"fragment_shader_textured"];
        id<MTLFunction> fragmentColorFunction = [library newFunctionWithName:@"fragment_shader_color"];
        
        // Load instanced vertex shader
        id<MTLFunction> vertexInstancedFunction = [library newFunctionWithName:@"vertex_shader_2d"];
        id<MTLFunction> vertexSimpleFunction = [library newFunctionWithName:@"vertex_shader_2d_simple"];
        
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
        
        m_texturePipeline = [m_device newRenderPipelineStateWithDescriptor:texturedPipelineDesc error:&error];
        if (error) {
            NSLog(@"Failed to create textured pipeline: %@", error.localizedDescription);
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
            NSLog(@"Failed to create color pipeline: %@", error.localizedDescription);
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
            NSLog(@"Failed to create instanced textured pipeline: %@", error.localizedDescription);
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
            NSLog(@"Failed to create instanced color pipeline: %@", error.localizedDescription);
        }
        
        // Create depth stencil state
        MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthDesc.depthCompareFunction = MTLCompareFunctionLessEqual;
        depthDesc.depthWriteEnabled = YES;
        m_depthStencilState = [m_device newDepthStencilStateWithDescriptor:depthDesc];
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
        return;
    }
    
    // Create render encoder
    m_currentEncoder = [m_currentCommandBuffer renderCommandEncoderWithDescriptor:m_currentRenderPass];
    m_currentEncoder.label = @"Main Render Encoder";
    
    // Set initial pipeline state
    [m_currentEncoder setDepthStencilState:m_depthStencilState];
    [m_currentEncoder setFragmentSamplerState:m_samplerState atIndex:0];
    
    // Update uniforms for this frame
    UpdateUniforms();
    
    // Validate render state if debugging
    if (m_debugVisualization) {
        ValidateRenderState();
    }
}

void MetalRenderer::EndFrame() {
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
    if (m_currentCommandBuffer && m_view.currentDrawable) {
        // Schedule presentation of the drawable
        [m_currentCommandBuffer presentDrawable:m_view.currentDrawable];
        
        // Signal the completion of this frame
        m_frameResources.EndFrame(m_currentCommandBuffer);
        
        // Submit the command buffer to the GPU
        [m_currentCommandBuffer commit];
        m_currentCommandBuffer = nullptr;
        
        // Implement frame pacing if needed
        double frameEndTime = CACurrentMediaTime();
        double frameTime = frameEndTime - m_frameStartTime;
        if (frameTime < m_targetFrameTime) {
            // Simple frame pacing - sleep if we're ahead of schedule
            usleep((useconds_t)((m_targetFrameTime - frameTime) * 1000000));
        }
    }
}

void MetalRenderer::Clear(Color color) {
    if (m_currentRenderPass) {
        m_currentRenderPass.colorAttachments[0].clearColor = MTLClearColorMake(
            color.r / 255.0f,
            color.g / 255.0f, 
            color.b / 255.0f,
            color.a / 255.0f
        );
    }
}

void MetalRenderer::SetProjectionMatrix(float width, float height) {
    // Create orthographic projection matrix for 2D rendering
    // Metal uses NDC from -1 to 1, but we want 0 to width/height
    m_projectionMatrix = MakeOrthoMatrix(0, width, height, 0, -1.0f, 1.0f);
}

void MetalRenderer::UpdateUniforms() {
    // Allocate uniform buffer from the current frame's resources
    size_t offset = 0;
    MetalUniforms* uniforms = static_cast<MetalUniforms*>(
        m_frameResources.AllocateUniformBuffer(sizeof(MetalUniforms), &offset)
    );
    
    if (!uniforms) {
        NSLog(@"[ERROR] Failed to allocate uniform buffer");
        return;
    }
    
    // Update the uniform data
    uniforms->projectionMatrix = m_projectionMatrix;
    uniforms->modelViewMatrix = m_currentMatrix;
    
    // Set the uniform buffer for rendering
    [m_currentEncoder setVertexBuffer:m_frameResources.GetCurrentUniformBuffer() 
                              offset:offset 
                             atIndex:1];
}

void MetalRenderer::DrawRectangle(float x, float y, float width, float height, Color color) {
    AddRectangleVertices(x, y, width, height, color);
    
    DrawCommand cmd;
    cmd.primitiveType = MTLPrimitiveTypeTriangle;
    cmd.vertexStart = m_vertices.size() - 6;
    cmd.vertexCount = 6;
    cmd.texture = nullptr;
    cmd.useTexture = false;
    
    // Enhanced fields for sorting and optimization
    cmd.renderState = RENDER_STATE_ALPHA_BLEND;
    cmd.textureId = 0; // No texture
    cmd.depth = 0.0f; // Default depth
    cmd.sortKey = 0; // Will be generated during sorting
    cmd.instanceCount = 1;
    cmd.instanceDataOffset = 0;
    cmd.debugName = "Rectangle";
    
    m_drawCommands.push_back(cmd);
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
    if (m_vertices.empty() || !m_currentEncoder) {
        return;
    }
    
    // Calculate size of vertex data
    size_t dataSize = m_vertices.size() * sizeof(MetalVertex2D);
    
    // Allocate space in the current frame's vertex buffer
    size_t bufferOffset = 0;
    void* destinationBuffer = m_frameResources.AllocateVertexBuffer(dataSize, &bufferOffset);
    
    if (!destinationBuffer) {
        NSLog(@"[ERROR] Failed to allocate vertex buffer space");
        return;
    }
    
    // Copy vertex data to the frame's vertex buffer
    memcpy(destinationBuffer, m_vertices.data(), dataSize);
    
    // Set the vertex buffer for the render command encoder
    [m_currentEncoder setVertexBuffer:m_frameResources.GetCurrentVertexBuffer() 
                              offset:bufferOffset 
                             atIndex:0];
    
    // Store the current offset for later reference (when drawing)
    size_t vertexStartOffset = m_currentVertexBufferOffset;
    m_currentVertexBufferOffset += m_vertices.size();
    
    // Update draw commands to use buffer-relative offsets
    for (auto& cmd : m_drawCommands) {
        cmd.vertexStart += vertexStartOffset;
    }
    
    // Execute optimized draw commands with sorting and state management
    ExecuteOptimizedDrawCommands();
    
    // Clear batch data for the next batch
    m_vertices.clear();
    m_drawCommands.clear();
}

void MetalRenderer::ExecuteOptimizedDrawCommands() {
    if (m_drawCommands.empty()) return;
    
    // Sort draw commands for optimal rendering
    SortDrawCommands();
    
    // Validate against mobile GPU limits
    if (m_drawCommands.size() > m_mobileSettings.maxDrawCallsPerFrame) {
        NSLog(@"[WARNING] Draw calls (%lu) exceed mobile GPU limit (%u)", 
              m_drawCommands.size(), m_mobileSettings.maxDrawCallsPerFrame);
    }
    
    // Execute commands with state optimization
    id<MTLRenderPipelineState> currentPipeline = nullptr;
    id<MTLTexture> currentTexture = nullptr;
    uint32_t currentRenderState = RENDER_STATE_NONE;
    uint32_t textureBindCount = 0;
    
    for (const auto& cmd : m_drawCommands) {
        m_debugStats.drawCalls++;
        
        // Check texture bind limits
        if (cmd.useTexture && cmd.texture && cmd.texture != currentTexture) {
            textureBindCount++;
            if (textureBindCount > m_mobileSettings.maxTextureBindsPerFrame) {
                NSLog(@"[WARNING] Texture binds (%u) exceed mobile GPU limit (%u)", 
                      textureBindCount, m_mobileSettings.maxTextureBindsPerFrame);
            }
        }
        
        // Set render state if changed
        if (cmd.renderState != currentRenderState) {
            SetRenderState(cmd.renderState);
            currentRenderState = cmd.renderState;
            m_debugStats.stateChanges++;
        }
        
        // Set pipeline state if changed
        id<MTLRenderPipelineState> requiredPipeline;
        if (cmd.renderState & RENDER_STATE_INSTANCED) {
            // Use instanced pipeline
            requiredPipeline = cmd.useTexture ? m_instancedTexturePipeline : m_instancedColorPipeline;
        } else {
            // Use regular pipeline
            requiredPipeline = cmd.useTexture ? m_texturePipeline : m_colorPipeline;
        }
        
        if (requiredPipeline != currentPipeline) {
            [m_currentEncoder setRenderPipelineState:requiredPipeline];
            currentPipeline = requiredPipeline;
            m_debugStats.stateChanges++;
        }
        
        // Bind texture if needed and changed
        if (cmd.useTexture && cmd.texture) {
            if (cmd.texture != currentTexture) {
                BindTexture(cmd.texture);
                currentTexture = cmd.texture;
                m_debugStats.textureBinds++;
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
        } else {
            // Regular rendering
            [m_currentEncoder drawPrimitives:cmd.primitiveType
                                 vertexStart:cmd.vertexStart
                                 vertexCount:cmd.vertexCount];
        }
        
        m_debugStats.batchedVertices += cmd.vertexCount;
    }
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
    result.columns[0][0] = c;
    result.columns[0][1] = s;
    result.columns[1][0] = -s;
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
    // Simple line implementation using two triangles to form a thin rectangle
    const float thickness = 1.0f;
    
    // Calculate perpendicular vector for thickness
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);
    
    if (length == 0) return;
    
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
    
    AddVertex(x1_top, y1_top, 0.5f, 0.5f, color);
    AddVertex(x1_bot, y1_bot, 0.5f, 0.5f, color);
    AddVertex(x2_top, y2_top, 0.5f, 0.5f, color);
    AddVertex(x2_bot, y2_bot, 0.5f, 0.5f, color);
    
    // First triangle: top-left, bottom-left, top-right
    DrawCommand cmd1;
    cmd1.primitiveType = MTLPrimitiveTypeTriangle;
    cmd1.vertexStart = startIdx;
    cmd1.vertexCount = 3;
    cmd1.texture = nullptr;
    cmd1.useTexture = false;
    m_drawCommands.push_back(cmd1);
    
    // Second triangle: bottom-left, bottom-right, top-right
    DrawCommand cmd2;
    cmd2.primitiveType = MTLPrimitiveTypeTriangle;
    cmd2.vertexStart = startIdx + 1;
    cmd2.vertexCount = 3;
    cmd2.texture = nullptr;
    cmd2.useTexture = false;
    m_drawCommands.push_back(cmd2);
}

void MetalRenderer::DrawTexture(id<MTLTexture> texture, Rectangle source, Rectangle dest, Color tint) {
    if (!texture) return;
    
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
    cmd.depth = 0.0f; // Default depth
    cmd.sortKey = 0; // Will be generated during sorting
    cmd.instanceCount = 1;
    cmd.instanceDataOffset = 0;
    cmd.debugName = "Texture";
    
    m_drawCommands.push_back(cmd);
}

void MetalRenderer::AddTexturedRectangleVertices(Rectangle dest, Rectangle source, Color tint) {
    // Calculate texture coordinates based on source rectangle
    // Assume source is in normalized coordinates (0-1 range)
    float u1 = source.x;
    float v1 = source.y;
    float u2 = source.x + source.width;
    float v2 = source.y + source.height;
    
    // Two triangles to make a rectangle
    // Triangle 1: top-left, bottom-left, top-right
    AddVertex(dest.x, dest.y, u1, v1, tint);                           // top-left
    AddVertex(dest.x, dest.y + dest.height, u1, v2, tint);            // bottom-left  
    AddVertex(dest.x + dest.width, dest.y, u2, v1, tint);             // top-right
    
    // Triangle 2: top-right, bottom-left, bottom-right
    AddVertex(dest.x + dest.width, dest.y, u2, v1, tint);             // top-right
    AddVertex(dest.x, dest.y + dest.height, u1, v2, tint);            // bottom-left
    AddVertex(dest.x + dest.width, dest.y + dest.height, u2, v2, tint); // bottom-right
}

void MetalRenderer::DrawTextureEx(id<MTLTexture> texture, Vector2 position, float rotation, float scale, Color tint) {
    if (!texture) return;
    
    // Get texture dimensions
    float width = texture.width * scale;
    float height = texture.height * scale;
    
    // Save current matrix
    PushMatrix();
    
    // Apply transformations
    TranslateMatrix(position.x + width * 0.5f, position.y + height * 0.5f);
    if (rotation != 0) {
        RotateMatrix(rotation);
    }
    ScaleMatrix(scale, scale);
    TranslateMatrix(-texture.width * 0.5f, -texture.height * 0.5f);
    
    // Draw texture
    Rectangle source = {0, 0, 1, 1}; // Full texture
    Rectangle dest = {0, 0, (float)texture.width, (float)texture.height};
    DrawTexture(texture, source, dest, tint);
    
    // Restore matrix
    PopMatrix();
}

void MetalRenderer::DrawText(const char* text, float x, float y, float fontSize, Color color) {
    if (!text) return;
    
    // This will be implemented by rendering text to a texture and then drawing that texture
    // For now, we'll draw a placeholder rectangle
    float width = strlen(text) * fontSize * 0.6f;
    float height = fontSize;
    
    DrawRectangle(x, y, width, height, {color.r, color.g, color.b, 64}); // Semi-transparent placeholder
}

void MetalRenderer::PushMatrix() {
    m_matrixStack.push_back(m_currentMatrix);
}

void MetalRenderer::PopMatrix() {
    if (!m_matrixStack.empty()) {
        m_currentMatrix = m_matrixStack.back();
        m_matrixStack.pop_back();
    }
}

void MetalRenderer::TranslateMatrix(float x, float y) {
    m_currentMatrix = simd_mul(m_currentMatrix, MakeTranslationMatrix(x, y));
}

void MetalRenderer::RotateMatrix(float angle) {
    m_currentMatrix = simd_mul(m_currentMatrix, MakeRotationMatrix(angle));
}

void MetalRenderer::ScaleMatrix(float x, float y) {
    m_currentMatrix = simd_mul(m_currentMatrix, MakeScaleMatrix(x, y));
}

void MetalRenderer::DrawRoundedCorner(float centerX, float centerY, float radius, int segments, int corner, Color color) {
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
        NSLog(@"[ERROR] Failed to allocate instance buffer space");
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
    if (instanceCount == 0) return;
    
    // Create instanced draw command
    DrawCommand cmd;
    cmd.primitiveType = MTLPrimitiveTypeTriangle;
    cmd.vertexStart = m_vertices.size() - 6; // Assume rectangle (6 vertices)
    cmd.vertexCount = 6;
    cmd.texture = texture;
    cmd.useTexture = (texture != nullptr);
    cmd.renderState = renderState | RENDER_STATE_INSTANCED;
    cmd.instanceCount = instanceCount;
    cmd.instanceDataOffset = m_currentInstanceBufferOffset;
    cmd.textureId = GetTextureHash(texture);
    cmd.depth = 0.0f; // Default depth
    cmd.debugName = "Instanced Draw";
    
    m_drawCommands.push_back(cmd);
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
    
    NSLog(@"[INFO] Debug visualization initialized");
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
    
    NSLog(@"[DEBUG] Frame Stats - Draws: %u, States: %u, Textures: %u, Instances: %u, Vertices: %u, Time: %.2fms",
          m_debugStats.drawCalls, m_debugStats.stateChanges, m_debugStats.textureBinds,
          m_debugStats.instancedCalls, m_debugStats.batchedVertices, m_debugStats.frameTime * 1000.0f);
}

void MetalRenderer::ValidateRenderState() {
    if (!m_debugVisualization) return;
    
    // Validate that the current render state is consistent
    if (!m_currentEncoder) {
        NSLog(@"[WARNING] No current render encoder set");
        return;
    }
    
    // Check for excessive state changes
    if (m_debugStats.stateChanges > 100) {
        NSLog(@"[WARNING] High number of state changes: %u", m_debugStats.stateChanges);
    }
    
    // Check for excessive draw calls
    if (m_debugStats.drawCalls > 200) {
        NSLog(@"[WARNING] High number of draw calls: %u", m_debugStats.drawCalls);
    }
    
    // Check for excessive texture binds
    if (m_debugStats.textureBinds > 100) {
        NSLog(@"[WARNING] High number of texture binds: %u", m_debugStats.textureBinds);
    }
}

// Mobile GPU optimization implementation
void MetalRenderer::SetMobileGPUSettings(const MobileGPUSettings& settings) {
    m_mobileSettings = settings;
    
    NSLog(@"[INFO] Mobile GPU settings updated - MaxDrawCalls: %u, MaxTextures: %u, Mipmaps: %s",
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
    
    NSLog(@"[INFO] Optimized for device: %@ (Modern GPU: %s)", deviceName, isA12OrLater ? "YES" : "NO");
}

#endif // defined(__APPLE__) && TARGET_OS_IOS 