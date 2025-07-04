#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Vertex structure matching MetalVertex2D
struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texCoords [[attribute(1)]];
    float4 color [[attribute(2)]];
};

// Vertex output / Fragment input
struct VertexOut {
    float4 position [[position]];
    float2 texCoords;
    float4 color;
};

// Uniform buffer structure
struct Uniforms {
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
};

// Vertex shader
vertex VertexOut vertex_shader_2d(VertexIn in [[stage_in]],
                                  constant Uniforms& uniforms [[buffer(1)]]) {
    VertexOut out;
    
    // Transform position
    float4 worldPos = uniforms.modelViewMatrix * float4(in.position, 0.0, 1.0);
    out.position = uniforms.projectionMatrix * worldPos;
    
    // Pass through texture coordinates and color
    out.texCoords = in.texCoords;
    out.color = in.color;
    
    return out;
}

// Fragment shader for textured rendering
fragment float4 fragment_shader_textured(VertexOut in [[stage_in]],
                                        texture2d<float> texture [[texture(0)]],
                                        sampler textureSampler [[sampler(0)]]) {
    // Sample texture
    float4 texColor = texture.sample(textureSampler, in.texCoords);
    
    // Apply tint color
    return texColor * in.color;
}

// Fragment shader for solid color rendering
fragment float4 fragment_shader_color(VertexOut in [[stage_in]]) {
    return in.color;
}

// Fragment shader for rounded rectangle (can be enhanced later)
fragment float4 fragment_shader_rounded_rect(VertexOut in [[stage_in]],
                                            constant float& roundness [[buffer(0)]]) {
    // For now, just return solid color
    // TODO: Implement proper rounded rectangle with distance field
    return in.color;
} 