#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS

// IMPORTANT: This header contains Objective-C++ code and should ONLY be included in .mm files.
// Do NOT include this in any header files that might be included by .cpp files to prevent build errors.
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>
#include <vector>
#include <queue>
#include "RaylibCompat.h"

// Vertex structure for 2D rendering
typedef struct {
    simd_float2 position;
    simd_float2 texCoords;
    simd_float4 color;
} MetalVertex2D;

// Uniform buffer for transformations
typedef struct {
    simd_float4x4 projectionMatrix;
    simd_float4x4 modelViewMatrix;
} MetalUniforms;

// Draw command for batching
typedef struct {
    MTLPrimitiveType primitiveType;
    NSUInteger vertexStart;
    NSUInteger vertexCount;
    id<MTLTexture> texture;
    bool useTexture;
} DrawCommand;

class MetalRenderer {
public:
    MetalRenderer();
    ~MetalRenderer();
    
    // Initialize with MTKView
    bool Initialize(MTKView* view);
    void Shutdown();
    
    // Frame management
    void BeginFrame();
    void EndFrame();
    void Present();
    
    // Clear operations
    void Clear(Color color);
    
    // State management
    void SetProjectionMatrix(float width, float height);
    void PushMatrix();
    void PopMatrix();
    void TranslateMatrix(float x, float y);
    void RotateMatrix(float angle);
    void ScaleMatrix(float x, float y);
    
    // Drawing primitives
    void DrawRectangle(float x, float y, float width, float height, Color color);
    void DrawRectangleRounded(float x, float y, float width, float height, float roundness, int segments, Color color);
    void DrawCircle(float x, float y, float radius, Color color);
    void DrawLine(float x1, float y1, float x2, float y2, Color color);
    
    // Texture drawing
    void DrawTexture(id<MTLTexture> texture, Rectangle source, Rectangle dest, Color tint);
    void DrawTextureEx(id<MTLTexture> texture, Vector2 position, float rotation, float scale, Color tint);
    
    // Text rendering
    void DrawText(const char* text, float x, float y, float fontSize, Color color);
    
    // Batch rendering
    void FlushBatch();
    
    // Getters
    id<MTLDevice> GetDevice() const { return m_device; }
    id<MTLCommandQueue> GetCommandQueue() const { return m_commandQueue; }
    MTKView* GetView() const { return m_view; }
    
private:
    // Metal objects
    MTKView* m_view;
    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_commandQueue;
    id<MTLRenderPipelineState> m_texturePipeline;
    id<MTLRenderPipelineState> m_colorPipeline;
    id<MTLDepthStencilState> m_depthStencilState;
    id<MTLBuffer> m_uniformBuffer;
    id<MTLSamplerState> m_samplerState;
    
    // Current frame resources
    id<MTLCommandBuffer> m_currentCommandBuffer;
    id<MTLRenderCommandEncoder> m_currentEncoder;
    MTLRenderPassDescriptor* m_currentRenderPass;
    
    // Vertex batching
    std::vector<MetalVertex2D> m_vertices;
    std::vector<DrawCommand> m_drawCommands;
    id<MTLBuffer> m_vertexBuffer;
    size_t m_vertexBufferSize;
    
    // Matrix stack
    std::vector<simd_float4x4> m_matrixStack;
    simd_float4x4 m_currentMatrix;
    simd_float4x4 m_projectionMatrix;
    
    // Frame timing
    double m_frameStartTime;
    float m_targetFrameTime;
    
    // Helper methods
    void CreatePipelines();
    void CreateBuffers();
    void UpdateUniforms();
    void AddVertex(float x, float y, float u, float v, Color color);
    void AddRectangleVertices(float x, float y, float width, float height, Color color);
    void AddTexturedRectangleVertices(Rectangle dest, Rectangle source, Color tint);
    void DrawRoundedCorner(float centerX, float centerY, float radius, int segments, int corner, Color color);
    void ExecuteDrawCommands();
    simd_float4x4 MakeOrthoMatrix(float left, float right, float bottom, float top, float near, float far);
    simd_float4x4 MakeTranslationMatrix(float x, float y);
    simd_float4x4 MakeRotationMatrix(float angle);
    simd_float4x4 MakeScaleMatrix(float x, float y);
};

// Global renderer instance (managed by MetalRaylibCompat)
extern MetalRenderer* g_metalRenderer;

#endif // defined(__APPLE__) && TARGET_OS_IOS 