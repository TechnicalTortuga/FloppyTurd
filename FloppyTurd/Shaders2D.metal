#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Vertex structure matching MetalVertex2D
struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texCoords [[attribute(1)]];
    float4 color [[attribute(2)]];
};

// Instance data structure matching InstanceData
struct InstanceData {
    float4x4 modelMatrix;
    float4 color;
    float4 texCoordScale; // xy = scale, zw = offset
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
    float distanceRange; // Add distance range for SDF
};

// Enhanced vertex shader with instancing support
vertex VertexOut vertex_shader_2d(VertexIn in [[stage_in]],
                                  constant Uniforms& uniforms [[buffer(1)]],
                                  constant InstanceData* instanceData [[buffer(2)]],
                                  uint instanceID [[instance_id]]) {
    VertexOut out;
    
    // Transform position
    float4 localPos = float4(in.position, 0.0, 1.0);
    
    // Apply instance transformation if available
    if (instanceData != nullptr) {
        localPos = instanceData[instanceID].modelMatrix * localPos;
    }
    
    // Apply model-view and projection transformations
    float4 worldPos = uniforms.modelViewMatrix * localPos;
    out.position = uniforms.projectionMatrix * worldPos;
    
    // Handle texture coordinates with instance scaling if available
    if (instanceData != nullptr) {
        // Apply texture coordinate scaling and offset
        out.texCoords = in.texCoords * instanceData[instanceID].texCoordScale.xy + instanceData[instanceID].texCoordScale.zw;
        // Apply instance color tinting
        out.color = in.color * instanceData[instanceID].color;
    } else {
        // Use regular texture coordinates and color
        out.texCoords = in.texCoords;
        out.color = in.color;
    }
    
    return out;
}

// Regular vertex shader (for non-instanced rendering)
vertex VertexOut vertex_shader_2d_simple(VertexIn in [[stage_in]],
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

// Fragment shader for SDF (Signed Distance Field) text rendering
fragment float4 fragment_shader_sdf(VertexOut in [[stage_in]],
                                   texture2d<float> sdfTexture [[texture(0)]],
                                   sampler textureSampler [[sampler(0)]],
                                   constant Uniforms& uniforms [[buffer(1)]]) {
    // Sample the SDF texture (single-channel grayscale)
    float distance = sdfTexture.sample(textureSampler, in.texCoords).r;
    
    // SDF thresholding with dynamic smoothing
    // Since our SDF is normalized to [0,1], use threshold around 0.5
    float threshold = 0.5; // Center of distance field
    float smoothing = 0.15; // Increased smoothing for better visibility
    float alpha = smoothstep(threshold - smoothing, threshold + smoothing, distance);
    
    // Apply tint color
    float4 finalColor = in.color;
    finalColor.a *= alpha;
    return finalColor;
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