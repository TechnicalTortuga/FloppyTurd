#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

// C++ namespace for Metal shader operations
namespace MetalShader {

// Shader compilation
id<MTLLibrary> CompileShaderFromSource(const char* source, id<MTLDevice> device);
id<MTLLibrary> LoadShaderFromFile(const char* filePath, id<MTLDevice> device);
id<MTLLibrary> LoadDefaultLibrary(id<MTLDevice> device);

// Function loading
id<MTLFunction> GetFunction(id<MTLLibrary> library, const char* functionName);

// Pipeline creation helpers
id<MTLRenderPipelineState> CreateRenderPipeline(
    id<MTLDevice> device,
    id<MTLFunction> vertexFunction,
    id<MTLFunction> fragmentFunction,
    MTLPixelFormat colorFormat,
    MTLPixelFormat depthFormat,
    MTLVertexDescriptor* vertexDescriptor
);

id<MTLComputePipelineState> CreateComputePipeline(
    id<MTLDevice> device,
    id<MTLFunction> computeFunction
);

// Utility functions
bool IsValidLibrary(id<MTLLibrary> library);
bool IsValidFunction(id<MTLFunction> function);
bool IsValidPipelineState(id<MTLRenderPipelineState> pipeline);

} // namespace MetalShader

#endif // defined(__APPLE__) && TARGET_OS_IOS 