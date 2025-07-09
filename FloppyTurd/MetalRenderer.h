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
#include "MetalFrameResources.h"
#include "RaylibCompat.h"
#include "RenderLayer.h"

// Vertex structure for 2D rendering
typedef struct {
    simd_float2 position;
    simd_float2 texCoords;
    simd_float4 color;
} MetalVertex2D;

// Uniform buffer for transformations
typedef struct {
    simd_float4x4 projectionMatrix;
    float distanceRange; // Add distance range for SDF
    float time;          // Add time for future effects
} MetalUniforms;

// Draw command for batching and sorting
typedef struct {
    MTLPrimitiveType primitiveType;
    NSUInteger vertexStart;
    NSUInteger vertexCount;
    id<MTLTexture> texture;
    bool useTexture;
    
    // Enhanced fields for sorting and state management
    uint32_t renderState;        // Combined render state hash
    uint32_t textureId;          // Texture ID for sorting
    float depth;                 // Z-depth for sorting
    uint32_t sortKey;            // Combined sort key
    
    // Instance data for instanced rendering
    uint32_t instanceCount;
    NSUInteger instanceDataOffset;
    
    // Debug info
    const char* debugName;
} DrawCommand;

// Render state flags for batching
typedef enum {
    RENDER_STATE_NONE = 0,
    RENDER_STATE_ALPHA_BLEND = 1 << 0,
    RENDER_STATE_DEPTH_TEST = 1 << 1,
    RENDER_STATE_CULL_BACK = 1 << 2,
    RENDER_STATE_WIREFRAME = 1 << 3,
    RENDER_STATE_INSTANCED = 1 << 4,
    RENDER_STATE_SDF = 1 << 5  // SDF texture rendering
} RenderStateFlags;

// Instance data for instanced rendering
typedef struct {
    simd_float4x4 modelMatrix;
    simd_float4 color;
    simd_float4 texCoordScale; // For texture atlas support
} InstanceData;

// Mobile GPU optimization settings
typedef struct {
    bool enableMipmapping;
    bool preferLowPowerGPU;
    uint32_t maxDrawCallsPerFrame;
    uint32_t maxTextureBindsPerFrame;
    uint32_t vertexBufferSize;
    uint32_t uniformBufferSize;
    bool enableEarlyZTest;
    bool enableOcclusionCulling;
} MobileGPUSettings;

// Debug and profiling stats
typedef struct {
    uint32_t drawCalls;
    uint32_t stateChanges;
    uint32_t textureBinds;
    uint32_t instancedCalls;
    uint32_t batchedVertices;
    double frameTime;
    double renderTime;
} DebugStats;

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
    void SetProjectionMatrixWithSafeArea(float screenWidth, float screenHeight, Rectangle safeArea);
    
    // Drawing primitives
    void DrawRectangle(float x, float y, float width, float height, Color color);
    void DrawRectangleRounded(float x, float y, float width, float height, float roundness, int segments, Color color);
    void DrawRectangleRoundedLines(float x, float y, float width, float height, float roundness, int segments, float lineThick, Color color);
    void DrawCircle(float x, float y, float radius, Color color);
    void DrawLine(float x1, float y1, float x2, float y2, Color color);
    void DrawLineEx(float x1, float y1, float x2, float y2, float thickness, Color color);
    
    // Texture drawing
    void DrawTexture(id<MTLTexture> texture, Rectangle source, Rectangle dest, Color tint);
    void DrawTexture(id<MTLTexture> texture, Rectangle source, Rectangle dest, Color tint, RenderLayer layer);
    void DrawTexture(id<MTLTexture> texture, Rectangle source, Rectangle dest, Color tint, RenderLayer layer, int textureFormat);
    void DrawTextureEx(id<MTLTexture> texture, Vector2 position, float rotation, float scale, Color tint);
    
    // Text rendering
    void DrawText(const char* text, float x, float y, float fontSize, Color color);
    void DrawText(const char* text, float x, float y, float fontSize, Color color, Font* font);
    
    // Batch rendering
    void FlushBatch();
    
    // Debug and profiling
    void EnableDebugVisualization(bool enable);
    const DebugStats& GetDebugStats() const { return m_debugStats; }
    void ResetDebugStats();
    
    // Device access
    id<MTLDevice> GetDevice() const { return m_device; }
    
    // Mobile GPU optimization
    void SetMobileGPUSettings(const MobileGPUSettings& settings);
    const MobileGPUSettings& GetMobileGPUSettings() const { return m_mobileSettings; }
    void OptimizeForDevice();
    
    // Instanced rendering interface
    void BeginInstancedBatch();
    void EndInstancedBatch();
    void DrawInstancedRectangles(const std::vector<Rectangle>& rects, const std::vector<Color>& colors);
    void DrawInstancedTextures(id<MTLTexture> texture, const std::vector<Rectangle>& sources, 
                              const std::vector<Rectangle>& dests, const std::vector<Color>& tints);
    
    // Getters
    id<MTLCommandQueue> GetCommandQueue() const { return m_commandQueue; }
    MTKView* GetView() const { return m_view; }
    
    void DrawTestRectangle(); // Test function to verify Metal pipeline
    
private:
    // Metal objects
    MTKView* m_view;
    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_commandQueue;
    id<MTLRenderPipelineState> m_texturePipeline;
    id<MTLRenderPipelineState> m_colorPipeline;
    id<MTLRenderPipelineState> m_sdfPipeline;  // SDF-specific pipeline for grayscale textures
    id<MTLRenderPipelineState> m_instancedTexturePipeline;
    id<MTLRenderPipelineState> m_instancedColorPipeline;
    id<MTLDepthStencilState> m_depthStencilState;
    id<MTLDepthStencilState> m_uiDepthStencilState;
    id<MTLSamplerState> m_samplerState;
    
    // Current frame resources
    id<MTLCommandBuffer> m_currentCommandBuffer;
    id<MTLRenderCommandEncoder> m_currentEncoder;
    MTLRenderPassDescriptor* m_currentRenderPass;
    
    // Current texture for UV normalization
    id<MTLTexture> m_currentTexture;
    
    // Triple-buffered frame resources manager
    MetalFrameResources m_frameResources;
    
    // Vertex batching
    std::vector<MetalVertex2D> m_vertices;
    std::vector<DrawCommand> m_drawCommands;
    size_t m_currentVertexBufferOffset; // Offset within current frame's vertex buffer
    
    // Enhanced batching and sorting
    std::vector<InstanceData> m_instanceData;
    size_t m_currentInstanceBufferOffset;
    
    // Debug and profiling
    DebugStats m_debugStats;
    
    bool m_debugVisualization;
    id<MTLBuffer> m_debugVertexBuffer;
    id<MTLRenderPipelineState> m_debugPipeline;
    
    // Mobile GPU optimization settings
    MobileGPUSettings m_mobileSettings;
    
    // Projection matrix
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
    void AddTransformedTexturedQuad(const simd_float2 vertices[4], id<MTLTexture> texture, Color tint);
    void DrawRoundedCorner(float centerX, float centerY, float radius, int segments, int corner, Color color);
    void DrawRoundedCornerLines(float centerX, float centerY, float radius, int segments, int corner, float lineThick, Color color);
    void ExecuteDrawCommands();
    simd_float4x4 MakeOrthoMatrix(float left, float right, float bottom, float top, float near, float far);
    simd_float4x4 MakeTranslationMatrix(float x, float y);
    simd_float4x4 MakeRotationMatrix(float angle);
    simd_float4x4 MakeScaleMatrix(float x, float y);
    
    // Enhanced rendering methods
    void SortDrawCommands();
    uint32_t GenerateSortKey(const DrawCommand& cmd);
    void OptimizeDrawCommands();
    void ExecuteOptimizedDrawCommands();
    
    // Instanced rendering
    void AddInstanceData(const InstanceData& instanceData);
    void FlushInstancedBatch();
    void DrawInstanced(id<MTLTexture> texture, uint32_t instanceCount, uint32_t renderState);
    
    // Debug visualization
    void InitializeDebugVisualization();
    void DrawDebugOverlay();
    void UpdateDebugStats();
    void RenderDebugInfo();
    
    // State management
    void SetRenderState(uint32_t renderState);
    void BindTexture(id<MTLTexture> texture);
    void ValidateRenderState();
    
    // Utility methods
    uint32_t GetTextureHash(id<MTLTexture> texture);
    bool ShouldBatchCommands(const DrawCommand& cmd1, const DrawCommand& cmd2);
    DrawCommand CreateDrawCommand(MTLPrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount, 
                                 id<MTLTexture> texture, bool useTexture, uint32_t renderState, 
                                 float depth, const char* debugName, uint32_t instanceCount = 1);
};

// Global renderer instance (managed by MetalRaylibCompat)
extern MetalRenderer* g_metalRenderer;

#endif // defined(__APPLE__) && TARGET_OS_IOS 