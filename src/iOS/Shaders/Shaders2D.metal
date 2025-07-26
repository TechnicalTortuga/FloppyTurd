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
    return texColor * in.color;
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

// Fragment shader for SDF text rendering
fragment float4 sdf_text_fragment(VertexOut in [[stage_in]],
                                  texture2d<float> sdfTexture [[texture(0)]],
                                  sampler sdfSampler [[sampler(0)]]) {
    // Sample the SDF texture
    float distance = sdfTexture.sample(sdfSampler, in.texCoord).r;
    
    // Convert from 0-255 range back to signed distance (-1 to +1)
    // Our SDF generation uses 128 (0.5) as the edge, values > 128 are inside
    float normalizedDistance = (distance - 0.5) * 2.0;
    
    // Calculate the width of the antialiased edge
    float edgeWidth = 0.7 * length(float2(dfdx(normalizedDistance), dfdy(normalizedDistance)));
    
    // Use 0.0 as the threshold for the normalized distance
    float edgeDistance = 0.0;
    
    // Apply antialiasing using smoothstep
    float alpha = smoothstep(edgeDistance - edgeWidth, edgeDistance + edgeWidth, normalizedDistance);
    
    // Apply the text color with computed alpha
    float4 textColor = in.color;
    textColor.a *= alpha;
    
    return textColor;
}

// Fragment shader for SDF text with outline
fragment float4 sdf_text_outline_fragment(VertexOut in [[stage_in]],
                                          texture2d<float> sdfTexture [[texture(0)]],
                                          sampler sdfSampler [[sampler(0)]],
                                          constant float& outlineWidth [[buffer(0)]],
                                          constant float4& outlineColor [[buffer(1)]]) {
    // Sample the SDF texture
    float distance = sdfTexture.sample(sdfSampler, in.texCoord).r;
    
    // Calculate outline and fill
    float smoothWidth = fwidth(distance) * 0.5;
    float outlineAlpha = smoothstep(0.5 - outlineWidth - smoothWidth, 0.5 - outlineWidth + smoothWidth, distance);
    float fillAlpha = smoothstep(0.5 - smoothWidth, 0.5 + smoothWidth, distance);
    
    // Combine outline and fill
    float4 finalColor = mix(outlineColor, in.color, fillAlpha);
    finalColor.a *= outlineAlpha;
    
    return finalColor;
}

// Fragment shader for SDF text with drop shadow
fragment float4 sdf_text_shadow_fragment(VertexOut in [[stage_in]],
                                         texture2d<float> sdfTexture [[texture(0)]],
                                         sampler sdfSampler [[sampler(0)]],
                                         constant float2& shadowOffset [[buffer(0)]],
                                         constant float4& shadowColor [[buffer(1)]]) {
    // Sample main text
    float distance = sdfTexture.sample(sdfSampler, in.texCoord).r;
    float smoothWidth = fwidth(distance) * 0.5;
    float alpha = smoothstep(0.5 - smoothWidth, 0.5 + smoothWidth, distance);
    
    // Sample shadow (offset texture coordinates)
    float2 shadowCoord = in.texCoord + shadowOffset;
    float shadowDistance = sdfTexture.sample(sdfSampler, shadowCoord).r;
    float shadowAlpha = smoothstep(0.5 - smoothWidth, 0.5 + smoothWidth, shadowDistance);
    
    // Combine shadow and text
    float4 textColor = in.color;
    textColor.a *= alpha;
    
    float4 finalShadowColor = shadowColor;
    finalShadowColor.a *= shadowAlpha * (1.0 - alpha); // Only show shadow where text isn't
    
    // Blend shadow behind text
    float4 finalColor = mix(finalShadowColor, textColor, textColor.a);
    finalColor.a = max(shadowAlpha * shadowColor.a, textColor.a);
    
    return finalColor;
}