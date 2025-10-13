//
//  Shaders2D.metal
//  FloppyTurd
//
//  Created by Gnosis Engine
//  Copyright © 2024 Floppy Turd Studios. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;

// Vertex input structure
struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
    float4 color [[attribute(2)]];
};

// Vertex output structure
struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
    float4 color;
};

// Uniforms structure
struct Uniforms {
    float4x4 mvpMatrix;
};

// Vertex shader for 2D sprites
vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                            constant Uniforms& uniforms [[buffer(1)]]) {
    VertexOut out;
    out.position = uniforms.mvpMatrix * float4(in.position, 0.0, 1.0);
    out.texCoord = in.texCoord;
    out.color = in.color;
    return out;
}

// Fragment shader for solid color rendering (main fragment shader)
fragment float4 fragment_main(VertexOut in [[stage_in]]) {
    return in.color;
}

// Fragment shader for textured sprites
fragment float4 textured_fragment_main(VertexOut in [[stage_in]],
                                      texture2d<float> colorTexture [[texture(0)]],
                                      sampler colorSampler [[sampler(0)]]) {
    float4 texColor = colorTexture.sample(colorSampler, in.texCoord);
    
    // DEBUG: Return the raw texture color without vertex color multiplication
    return texColor;
    
    // Original code (commented out for testing)
    // return texColor * in.color;
}

// Legacy fragment shader names for compatibility
fragment float4 fragment_textured(VertexOut in [[stage_in]],
                                 texture2d<float> colorTexture [[texture(0)]],
                                 sampler colorSampler [[sampler(0)]]) {
    float4 texColor = colorTexture.sample(colorSampler, in.texCoord);
    return texColor * in.color;
}

fragment float4 fragment_solid(VertexOut in [[stage_in]]) {
    return in.color;
}

// Fragment shader for debug wireframe
fragment float4 fragment_debug(VertexOut in [[stage_in]]) {
    return float4(1.0, 0.0, 1.0, 1.0); // Magenta for debug
}



// MSDF/SDF shader paths removed — raster text is used now

// ==============================================================================
// GPU INSTANCED SPRITE RENDERING
// ==============================================================================

// Per-instance sprite data (96 bytes per sprite)
struct SpriteInstanceData {
    float4x4 modelMatrix;      // Transform (position, scale, rotation) - 64 bytes
    float4 uvRect;             // (u0, v0, u1, v1) for sprite sheets - 16 bytes
    float4 color;              // Tint/alpha (r, g, b, a) - 16 bytes
};

// Shared frame-level uniforms
struct FrameUniforms {
    float4x4 projectionMatrix;  // Orthographic projection - 64 bytes
};

// Vertex input for instanced rendering (simple quad)
struct InstancedVertexIn {
    float2 position [[attribute(0)]];   // Local quad position (0-1)
    float2 texCoord [[attribute(1)]];   // Base texture coordinates (0-1)
};

// Vertex output for instanced rendering
struct InstancedVertexOut {
    float4 position [[position]];
    float2 texCoord;
    float4 color;
};

// Instanced vertex shader
vertex InstancedVertexOut spriteVertexInstanced(
    InstancedVertexIn in [[stage_in]],
    constant FrameUniforms& frameUniforms [[buffer(1)]],
    constant SpriteInstanceData* instances [[buffer(2)]],
    uint instanceID [[instance_id]])
{
    // Get this sprite's instance data
    SpriteInstanceData instance = instances[instanceID];
    
    // Transform vertex position by instance's model matrix
    float4 worldPosition = instance.modelMatrix * float4(in.position, 0.0, 1.0);
    
    // Project to clip space
    float4 clipPosition = frameUniforms.projectionMatrix * worldPosition;
    
    // Calculate UV coordinates from sprite sheet
    // Mix between uvRect.xy (top-left) and uvRect.zw (bottom-right) based on texCoord
    float2 uv = mix(instance.uvRect.xy, instance.uvRect.zw, in.texCoord);
    
    // Output
    InstancedVertexOut out;
    out.position = clipPosition;
    out.texCoord = uv;
    out.color = instance.color;
    return out;
}

// Instanced fragment shader
fragment float4 spriteFragmentInstanced(
    InstancedVertexOut in [[stage_in]],
    texture2d<float> texture [[texture(0)]],
    sampler textureSampler [[sampler(0)]])
{
    // Sample texture
    float4 texColor = texture.sample(textureSampler, in.texCoord);
    
    // Apply tint/alpha
    return texColor * in.color;
}