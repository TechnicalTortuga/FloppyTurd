#ifndef METAL_FRAME_RESOURCES_H
#define METAL_FRAME_RESOURCES_H

#import <Metal/Metal.h>
#include <array>

// Forward declarations for Metal types
@protocol MTLDevice;
@protocol MTLBuffer;
@protocol MTLCommandBuffer;
@protocol MTLRenderCommandEncoder;

// Number of frames in flight (triple buffering)
constexpr int kMaxFramesInFlight = 3;

/**
 * MetalFrameResource - Manages per-frame resources with triple buffering
 * This class handles synchronization of resources across multiple frames
 * to avoid GPU-CPU contention issues.
 */
class MetalFrameResources {
public:
    MetalFrameResources();
    ~MetalFrameResources();
    
    // Initialize with a Metal device
    bool Initialize(id<MTLDevice> device);
    
    // Get resources for current frame
    id<MTLBuffer> GetCurrentUniformBuffer() const;
    id<MTLBuffer> GetCurrentVertexBuffer() const;
    
    // Get the index of the current frame
    uint8_t GetCurrentFrameIndex() const { return m_currentFrameIndex; }
    
    // Begin a new frame - rotates the triple buffer and waits for GPU if necessary
    void BeginFrame();
    
    // End the current frame and signal the GPU fence
    void EndFrame(id<MTLCommandBuffer> commandBuffer);
    
    // Dynamic buffer allocation for the current frame
    void* AllocateVertexBuffer(size_t size, size_t* outOffset);
    void* AllocateUniformBuffer(size_t size, size_t* outOffset);
    
    // Resize dynamic buffers if needed
    void ResizeVertexBuffer(size_t newSize);
    void ResizeUniformBuffer(size_t newSize);
    
private:
    // Triple buffer resources
    struct FrameData {
        id<MTLBuffer> uniformBuffer;
        id<MTLBuffer> vertexBuffer;
        size_t vertexBufferOffset;
        size_t uniformBufferOffset;
        
        // Semaphores for GPU-CPU synchronization
        dispatch_semaphore_t inFlightSemaphore;
    };
    
    std::array<FrameData, kMaxFramesInFlight> m_frameData;
    uint8_t m_currentFrameIndex;
    
    // Buffer sizes
    size_t m_uniformBufferSize;
    size_t m_vertexBufferSize;
    
    // Metal device reference
    id<MTLDevice> m_device;
    
    // Helper to create a buffer for the specified frame
    id<MTLBuffer> CreateBuffer(size_t size, MTLResourceOptions options, uint8_t frameIndex, bool isVertexBuffer);
};

#endif // METAL_FRAME_RESOURCES_H
