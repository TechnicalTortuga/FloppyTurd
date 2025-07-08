#import "MetalShader.h"
#import <Foundation/Foundation.h>
#include <string>
#include "ResourceManager.h"

#if defined(__APPLE__) && TARGET_OS_IOS

namespace MetalShader {

id<MTLLibrary> CompileShaderFromSource(const char* source, id<MTLDevice> device) {
    if (!source || !device) {
        return nil;
    }
    
    @autoreleasepool {
        NSString* sourceString = [NSString stringWithUTF8String:source];
        NSError* error = nil;
        
        MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
        options.languageVersion = MTLLanguageVersion2_0;
        
        id<MTLLibrary> library = [device newLibraryWithSource:sourceString 
                                                      options:options 
                                                        error:&error];
        
        if (!library) {
            NSLog(@"Failed to compile shader: %@", error.localizedDescription);
            return nil;
        }
        
        return library;
    }
}

id<MTLLibrary> LoadShaderFromFile(const char* filePath, id<MTLDevice> device) {
    if (!filePath || !device) {
        return nil;
    }
    
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:filePath];
        
        std::string cppPath = [path UTF8String];
        ResourcePathParts parts = ResourceManager::ParseResourcePath(cppPath);
        NSString* fileName = [NSString stringWithUTF8String:parts.baseName.c_str()];
        // Try to load from bundle first
        NSString* bundlePath = [[NSBundle mainBundle] pathForResource:fileName ofType:parts.extension.length() ? [NSString stringWithUTF8String:parts.extension.c_str()] : nil];
        if (!bundlePath) {
            bundlePath = path;
        }
        
        NSError* error = nil;
        NSString* shaderSource = [NSString stringWithContentsOfFile:bundlePath 
                                                           encoding:NSUTF8StringEncoding 
                                                              error:&error];
        
        if (!shaderSource) {
            NSLog(@"Failed to load shader file %@: %@", path, error.localizedDescription);
            return nil;
        }
        
        return CompileShaderFromSource([shaderSource UTF8String], device);
    }
}

id<MTLLibrary> LoadDefaultLibrary(id<MTLDevice> device) {
    if (!device) {
        return nil;
    }
    
    return [device newDefaultLibrary];
}

id<MTLFunction> GetFunction(id<MTLLibrary> library, const char* functionName) {
    if (!library || !functionName) {
        return nil;
    }
    
    @autoreleasepool {
        NSString* name = [NSString stringWithUTF8String:functionName];
        id<MTLFunction> function = [library newFunctionWithName:name];
        
        if (!function) {
            NSLog(@"Failed to find function: %@", name);
        }
        
        return function;
    }
}

id<MTLRenderPipelineState> CreateRenderPipeline(
    id<MTLDevice> device,
    id<MTLFunction> vertexFunction,
    id<MTLFunction> fragmentFunction,
    MTLPixelFormat colorFormat,
    MTLPixelFormat depthFormat,
    MTLVertexDescriptor* vertexDescriptor) {
    
    if (!device || !vertexFunction || !fragmentFunction) {
        return nil;
    }
    
    @autoreleasepool {
        MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDescriptor.label = @"Render Pipeline";
        pipelineDescriptor.vertexFunction = vertexFunction;
        pipelineDescriptor.fragmentFunction = fragmentFunction;
        pipelineDescriptor.colorAttachments[0].pixelFormat = colorFormat;
        
        if (depthFormat != MTLPixelFormatInvalid) {
            pipelineDescriptor.depthAttachmentPixelFormat = depthFormat;
        }
        
        if (vertexDescriptor) {
            pipelineDescriptor.vertexDescriptor = vertexDescriptor;
        }
        
        // Set up alpha blending
        pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
        pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        
        NSError* error = nil;
        id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor 
                                                                                           error:&error];
        
        if (!pipelineState) {
            NSLog(@"Failed to create render pipeline: %@", error.localizedDescription);
        }
        
        return pipelineState;
    }
}

id<MTLComputePipelineState> CreateComputePipeline(id<MTLDevice> device, id<MTLFunction> computeFunction) {
    if (!device || !computeFunction) {
        return nil;
    }
    
    @autoreleasepool {
        NSError* error = nil;
        id<MTLComputePipelineState> pipelineState = [device newComputePipelineStateWithFunction:computeFunction 
                                                                                          error:&error];
        
        if (!pipelineState) {
            NSLog(@"Failed to create compute pipeline: %@", error.localizedDescription);
        }
        
        return pipelineState;
    }
}

bool IsValidLibrary(id<MTLLibrary> library) {
    return library != nil;
}

bool IsValidFunction(id<MTLFunction> function) {
    return function != nil;
}

bool IsValidPipelineState(id<MTLRenderPipelineState> pipeline) {
    return pipeline != nil;
}

} // namespace MetalShader

#endif // defined(__APPLE__) && TARGET_OS_IOS 