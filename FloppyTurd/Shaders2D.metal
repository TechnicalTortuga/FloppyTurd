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
    float distanceRange; // Add distance range for SDF
    float time;          // Add time for future effects
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
    
    // Position already transformed on CPU - just apply projection
    out.position = uniforms.projectionMatrix * localPos;
    
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
    
    // Position already transformed on CPU - just apply projection
    out.position = uniforms.projectionMatrix * float4(in.position, 0.0, 1.0);
    
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

// Fragment shader for rounded rectangle using SDF
fragment float4 fragment_shader_rounded_rect(VertexOut in [[stage_in]],
                                            constant float& roundness [[buffer(0)]],
                                            constant float2& rectSize [[buffer(1)]]) {
    // Get normalized coordinates (0,0 to 1,1)
    float2 uv = in.texCoords;
    
    // Convert to centered coordinates (-0.5 to 0.5)
    float2 centeredUV = uv - 0.5;
    
    // Scale by rectangle size to get pixel coordinates
    float2 pos = centeredUV * rectSize;
    
    // Calculate distance to rounded rectangle
    float2 absPos = abs(pos);
    float2 rectHalfSize = rectSize * 0.5;
    
    // Clamp roundness to valid range
    float clampedRoundness = clamp(roundness, 0.0, min(rectHalfSize.x, rectHalfSize.y));
    
    // Calculate distance to rounded rectangle edges
    float2 cornerOffset = absPos - (rectHalfSize - clampedRoundness);
    float cornerDistance = length(max(cornerOffset, 0.0)) - clampedRoundness;
    float edgeDistance = max(cornerOffset.x, cornerOffset.y);
    
    // Combine corner and edge distances
    float distance = max(cornerDistance, edgeDistance);
    
    // Anti-aliasing with smoothstep
    float smoothing = 1.0; // Adjust for desired edge softness
    float alpha = 1.0 - smoothstep(-smoothing, smoothing, distance);
    
    // Apply alpha to color
    float4 finalColor = in.color;
    finalColor.a *= alpha;
    
    return finalColor;
}

// Fragment shader for rounded rectangle with border
fragment float4 fragment_shader_rounded_rect_border(VertexOut in [[stage_in]],
                                                   constant float& roundness [[buffer(0)]],
                                                   constant float2& rectSize [[buffer(1)]],
                                                   constant float& borderWidth [[buffer(2)]],
                                                   constant float4& borderColor [[buffer(3)]]) {
    // Get normalized coordinates (0,0 to 1,1)
    float2 uv = in.texCoords;
    
    // Convert to centered coordinates (-0.5 to 0.5)
    float2 centeredUV = uv - 0.5;
    
    // Scale by rectangle size to get pixel coordinates
    float2 pos = centeredUV * rectSize;
    
    // Calculate distance to rounded rectangle
    float2 absPos = abs(pos);
    float2 rectHalfSize = rectSize * 0.5;
    
    // Clamp roundness to valid range
    float clampedRoundness = clamp(roundness, 0.0, min(rectHalfSize.x, rectHalfSize.y));
    
    // Calculate distance to outer rounded rectangle
    float2 cornerOffset = absPos - (rectHalfSize - clampedRoundness);
    float cornerDistance = length(max(cornerOffset, 0.0)) - clampedRoundness;
    float edgeDistance = max(cornerOffset.x, cornerOffset.y);
    float outerDistance = max(cornerDistance, edgeDistance);
    
    // Calculate distance to inner rounded rectangle (for border)
    float2 innerRectHalfSize = rectHalfSize - borderWidth;
    float innerClampedRoundness = clamp(clampedRoundness - borderWidth, 0.0, min(innerRectHalfSize.x, innerRectHalfSize.y));
    
    float2 innerCornerOffset = absPos - (innerRectHalfSize - innerClampedRoundness);
    float innerCornerDistance = length(max(innerCornerOffset, 0.0)) - innerClampedRoundness;
    float innerEdgeDistance = max(innerCornerOffset.x, innerCornerOffset.y);
    float innerDistance = max(innerCornerDistance, innerEdgeDistance);
    
    // Anti-aliasing
    float smoothing = 1.0;
    float outerAlpha = 1.0 - smoothstep(-smoothing, smoothing, outerDistance);
    float innerAlpha = 1.0 - smoothstep(-smoothing, smoothing, innerDistance);
    
    // Determine if we're in border or fill area
    float borderAlpha = outerAlpha * (1.0 - innerAlpha);
    float fillAlpha = outerAlpha * innerAlpha;
    
    // Mix border and fill colors
    float4 finalColor = borderColor * borderAlpha + in.color * fillAlpha;
    
    return finalColor;
}