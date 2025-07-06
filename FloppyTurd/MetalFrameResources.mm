#include "MetalFrameResources.h"

#if defined(__APPLE__) && TARGET_OS_IOS

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

// Initial buffer sizes
constexpr size_t kInitialUniformBufferSize = 16 * 1024;    // 16KB
constexpr size_t kInitialVertexBufferSize = 256 * 1024;    // 256KB

MetalFrameResources::MetalFrameResources()
    : m_currentFrameIndex(0)
    , m_uniformBufferSize(kInitialUniformBufferSize)
    , m_vertexBufferSize(kInitialVertexBufferSize)
    , m_device(nullptr)
{
}

MetalFrameResources::~MetalFrameResources() {
    // Wait for all frames to complete before releasing resources
    for (uint8_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (m_frameData[i].inFlightSemaphore) {
            dispatch_semaphore_wait(m_frameData[i].inFlightSemaphore, DISPATCH_TIME_FOREVER);
            // ARC will automatically release the semaphore
        }
        // Metal objects will be released via ARC
    }
}

bool MetalFrameResources::Initialize(id<MTLDevice> device) {
    if (!device) return false;
    
    m_device = device;
    
    // Initialize each frame's resources
    for (uint8_t i = 0; i < kMaxFramesInFlight; ++i) {
        // Create uniform buffer
        m_frameData[i].uniformBuffer = CreateBuffer(
            m_uniformBufferSize, 
            MTLResourceStorageModeShared,
            i, 
            false
        );
        
        // Create vertex buffer
        m_frameData[i].vertexBuffer = CreateBuffer(
            m_vertexBufferSize, 
            MTLResourceStorageModeShared,
            i, 
            true
        );
        
        // Reset offsets
        m_frameData[i].uniformBufferOffset = 0;
        m_frameData[i].vertexBufferOffset = 0;
        
        // Create semaphore for GPU synchronization
        m_frameData[i].inFlightSemaphore = dispatch_semaphore_create(1);
        if (!m_frameData[i].inFlightSemaphore) {
            NSLog(@"[ERROR] Failed to create semaphore for frame %d", i);
            return false;
        }
    }
    
    NSLog(@"[INFO] MetalFrameResources initialized with %d frames in flight", kMaxFramesInFlight);
    return true;
}

id<MTLBuffer> MetalFrameResources::GetCurrentUniformBuffer() const {
    return m_frameData[m_currentFrameIndex].uniformBuffer;
}

id<MTLBuffer> MetalFrameResources::GetCurrentVertexBuffer() const {
    return m_frameData[m_currentFrameIndex].vertexBuffer;
}

void MetalFrameResources::BeginFrame() {
    // Wait for the next frame resources to be available (GPU finished using them)
    dispatch_semaphore_wait(
        m_frameData[m_currentFrameIndex].inFlightSemaphore,
        DISPATCH_TIME_FOREVER
    );
    
    // Reset buffer offsets for this frame
    m_frameData[m_currentFrameIndex].uniformBufferOffset = 0;
    m_frameData[m_currentFrameIndex].vertexBufferOffset = 0;
}

void MetalFrameResources::EndFrame(id<MTLCommandBuffer> commandBuffer) {
    // When command buffer completes, signal the semaphore
    __block dispatch_semaphore_t semaphore = m_frameData[m_currentFrameIndex].inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
        dispatch_semaphore_signal(semaphore);
    }];
    
    // Move to next frame in the triple buffer
    m_currentFrameIndex = (m_currentFrameIndex + 1) % kMaxFramesInFlight;
}

void* MetalFrameResources::AllocateVertexBuffer(size_t size, size_t* outOffset) {
    FrameData& frame = m_frameData[m_currentFrameIndex];
    
    // Check if we need to resize
    if (frame.vertexBufferOffset + size > m_vertexBufferSize) {
        size_t newSize = m_vertexBufferSize * 2;
        while (frame.vertexBufferOffset + size > newSize) {
            newSize *= 2;
        }
        ResizeVertexBuffer(newSize);
    }
    
    // Allocate from current offset
    *outOffset = frame.vertexBufferOffset;
    frame.vertexBufferOffset += size;
    
    // Return pointer to the mapped memory
    return static_cast<uint8_t*>([frame.vertexBuffer contents]) + *outOffset;
}

void* MetalFrameResources::AllocateUniformBuffer(size_t size, size_t* outOffset) {
    FrameData& frame = m_frameData[m_currentFrameIndex];
    
    // Check if we need to resize
    if (frame.uniformBufferOffset + size > m_uniformBufferSize) {
        size_t newSize = m_uniformBufferSize * 2;
        while (frame.uniformBufferOffset + size > newSize) {
            newSize *= 2;
        }
        ResizeUniformBuffer(newSize);
    }
    
    // 256-byte alignment for uniform buffers (Metal requirement)
    const size_t alignment = 256;
    frame.uniformBufferOffset = (frame.uniformBufferOffset + alignment - 1) & ~(alignment - 1);
    
    // Allocate from current offset
    *outOffset = frame.uniformBufferOffset;
    frame.uniformBufferOffset += size;
    
    // Return pointer to the mapped memory
    return static_cast<uint8_t*>([frame.uniformBuffer contents]) + *outOffset;
}

void MetalFrameResources::ResizeVertexBuffer(size_t newSize) {
    NSLog(@"[INFO] Resizing vertex buffer from %zu to %zu bytes", m_vertexBufferSize, newSize);
    
    m_vertexBufferSize = newSize;
    
    // Recreate all frame buffers with new size
    for (uint8_t i = 0; i < kMaxFramesInFlight; ++i) {
        id<MTLBuffer> newBuffer = CreateBuffer(
            m_vertexBufferSize, 
            MTLResourceStorageModeShared, 
            i, 
            true
        );
        
        // Copy existing data if there is any
        if (m_frameData[i].vertexBufferOffset > 0) {
            memcpy(
                [newBuffer contents], 
                [m_frameData[i].vertexBuffer contents], 
                m_frameData[i].vertexBufferOffset
            );
        }
        
        // Replace buffer
        m_frameData[i].vertexBuffer = newBuffer;
    }
}

void MetalFrameResources::ResizeUniformBuffer(size_t newSize) {
    NSLog(@"[INFO] Resizing uniform buffer from %zu to %zu bytes", m_uniformBufferSize, newSize);
    
    m_uniformBufferSize = newSize;
    
    // Recreate all frame buffers with new size
    for (uint8_t i = 0; i < kMaxFramesInFlight; ++i) {
        id<MTLBuffer> newBuffer = CreateBuffer(
            m_uniformBufferSize, 
            MTLResourceStorageModeShared, 
            i, 
            false
        );
        
        // Copy existing data if there is any
        if (m_frameData[i].uniformBufferOffset > 0) {
            memcpy(
                [newBuffer contents], 
                [m_frameData[i].uniformBuffer contents], 
                m_frameData[i].uniformBufferOffset
            );
        }
        
        // Replace buffer
        m_frameData[i].uniformBuffer = newBuffer;
    }
}

id<MTLBuffer> MetalFrameResources::CreateBuffer(size_t size, MTLResourceOptions options, uint8_t frameIndex, bool isVertexBuffer) {
    NSString* label = [NSString stringWithFormat:@"%s Buffer (Frame %d)",
                       isVertexBuffer ? "Vertex" : "Uniform", frameIndex];
    
    id<MTLBuffer> buffer = [m_device newBufferWithLength:size options:options];
    [buffer setLabel:label];
    
    return buffer;
}

#endif // defined(__APPLE__) && TARGET_OS_IOS
