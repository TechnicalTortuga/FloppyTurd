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
    , m_depthStencilState(nullptr)
    , m_uniformBuffer(nullptr)
    , m_samplerState(nullptr)
    , m_currentCommandBuffer(nullptr)
    , m_currentEncoder(nullptr)
    , m_currentRenderPass(nullptr)
    , m_vertexBuffer(nullptr)
    , m_vertexBufferSize(0)
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
    m_depthStencilState = nullptr;
    m_uniformBuffer = nullptr;
    m_samplerState = nullptr;
    m_currentCommandBuffer = nullptr;
    m_currentEncoder = nullptr;
    m_currentRenderPass = nullptr;
    m_vertexBuffer = nullptr;
    
    m_vertices.clear();
    m_drawCommands.clear();
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
        
        // Create textured pipeline
        MTLRenderPipelineDescriptor* texturedPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        texturedPipelineDesc.label = @"Textured Pipeline";
        texturedPipelineDesc.vertexFunction = vertexFunction;
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
        if (!m_texturePipeline) {
            NSLog(@"Failed to create textured pipeline: %@", error.localizedDescription);
        }
        
        // Create color pipeline
        MTLRenderPipelineDescriptor* colorPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        colorPipelineDesc.label = @"Color Pipeline";
        colorPipelineDesc.vertexFunction = vertexFunction;
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
        if (!m_colorPipeline) {
            NSLog(@"Failed to create color pipeline: %@", error.localizedDescription);
        }
        
        // Create depth stencil state
        MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthDesc.depthCompareFunction = MTLCompareFunctionLessEqual;
        depthDesc.depthWriteEnabled = YES;
        m_depthStencilState = [m_device newDepthStencilStateWithDescriptor:depthDesc];
    }
}

void MetalRenderer::CreateBuffers() {
    // Create uniform buffer
    m_uniformBuffer = [m_device newBufferWithLength:sizeof(MetalUniforms) 
                                             options:MTLResourceStorageModeShared];
    
    // Create vertex buffer (start with 10000 vertices)
    m_vertexBufferSize = 10000 * sizeof(MetalVertex2D);
    m_vertexBuffer = [m_device newBufferWithLength:m_vertexBufferSize 
                                           options:MTLResourceStorageModeShared];
    
    m_vertices.reserve(10000);
}

void MetalRenderer::BeginFrame() {
    m_frameStartTime = CACurrentMediaTime();
    
    // Create command buffer
    m_currentCommandBuffer = [m_commandQueue commandBuffer];
    m_currentCommandBuffer.label = @"Frame Command Buffer";
    
    // Get render pass descriptor from view
    m_currentRenderPass = m_view.currentRenderPassDescriptor;
    if (!m_currentRenderPass) {
        return;
    }
    
    // Create render encoder
    m_currentEncoder = [m_currentCommandBuffer renderCommandEncoderWithDescriptor:m_currentRenderPass];
    m_currentEncoder.label = @"Render Encoder";
    
    [m_currentEncoder setDepthStencilState:m_depthStencilState];
    
    // Update uniforms
    UpdateUniforms();
}

void MetalRenderer::EndFrame() {
    FlushBatch();
    
    if (m_currentEncoder) {
        [m_currentEncoder endEncoding];
        m_currentEncoder = nullptr;
    }
}

void MetalRenderer::Present() {
    if (m_currentCommandBuffer && m_view.currentDrawable) {
        [m_currentCommandBuffer presentDrawable:m_view.currentDrawable];
        [m_currentCommandBuffer commit];
        m_currentCommandBuffer = nullptr;
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
    if (!m_uniformBuffer) return;
    
    MetalUniforms* uniforms = (MetalUniforms*)[m_uniformBuffer contents];
    uniforms->projectionMatrix = m_projectionMatrix;
    uniforms->modelViewMatrix = m_currentMatrix;
    
    [m_currentEncoder setVertexBuffer:m_uniformBuffer offset:0 atIndex:1];
}

void MetalRenderer::DrawRectangle(float x, float y, float width, float height, Color color) {
    AddRectangleVertices(x, y, width, height, color);
    
    DrawCommand cmd;
    cmd.primitiveType = MTLPrimitiveTypeTriangle;
    cmd.vertexStart = m_vertices.size() - 6;
    cmd.vertexCount = 6;
    cmd.texture = nullptr;
    cmd.useTexture = false;
    
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
    
    // Copy vertices to GPU buffer
    size_t dataSize = m_vertices.size() * sizeof(MetalVertex2D);
    if (dataSize > m_vertexBufferSize) {
        // Reallocate buffer if needed
        m_vertexBufferSize = dataSize * 2;
        m_vertexBuffer = [m_device newBufferWithLength:m_vertexBufferSize 
                                               options:MTLResourceStorageModeShared];
    }
    
    memcpy([m_vertexBuffer contents], m_vertices.data(), dataSize);
    
    [m_currentEncoder setVertexBuffer:m_vertexBuffer offset:0 atIndex:0];
    
    // Execute draw commands
    ExecuteDrawCommands();
    
    // Clear for next batch
    m_vertices.clear();
    m_drawCommands.clear();
}

void MetalRenderer::ExecuteDrawCommands() {
    for (const auto& cmd : m_drawCommands) {
        if (cmd.useTexture && cmd.texture) {
            [m_currentEncoder setRenderPipelineState:m_texturePipeline];
            [m_currentEncoder setFragmentTexture:cmd.texture atIndex:0];
            [m_currentEncoder setFragmentSamplerState:m_samplerState atIndex:0];
        } else {
            [m_currentEncoder setRenderPipelineState:m_colorPipeline];
        }
        
        [m_currentEncoder drawPrimitives:cmd.primitiveType
                             vertexStart:cmd.vertexStart
                             vertexCount:cmd.vertexCount];
    }
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

#endif // defined(__APPLE__) && TARGET_OS_IOS 