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



// Fragment shader for SDF text rendering
fragment float4 sdf_text_fragment(VertexOut in [[stage_in]],
                                  texture2d<float> sdfTexture [[texture(0)]],
                                  sampler sdfSampler [[sampler(0)]]) {
    // Sample SDF and invert to match our atlas convention (inside dark)
    float distance = 1.0 - sdfTexture.sample(sdfSampler, in.texCoord).r;

    // Standard Valve-style AA: use screen-space derivative for smoothing width
    // Slightly widen AA window for stability
    float w = max(fwidth(distance), 1e-6) * 1.25;
    float alpha = smoothstep(0.5 - w, 0.5 + w, distance);
    
    // DEBUG: Show what we're actually sampling and calculating
    // Uncomment one of these to debug:
    
    // Show raw distance values as grayscale
    // return float4(distance, distance, distance, 1.0);
    
    // Show alpha values as grayscale (what should be opaque vs transparent)
    // return float4(alpha, alpha, alpha, 1.0);
    
    // Show signed distance (red=positive/inside, blue=negative/outside)
    // return float4(max(signedDistance, 0.0), 0.0, max(-signedDistance, 0.0), 1.0);
    
    // Compose final color with correct alpha (non-premultiplied input)
    float4 textColor = in.color;
    textColor.a *= alpha;
    return textColor;
}

// Fragment shader for SDF text with outline
fragment float4 sdf_text_outline_fragment(VertexOut in [[stage_in]],
                                          texture2d<float> sdfTexture [[texture(0)]],
                                          sampler sdfSampler [[sampler(0)]],
                                          constant float& outlineWidth [[buffer(0)]],
                                          constant float4& outlineColor [[buffer(1)]],
                                          constant float& edgeCenter [[buffer(2)]],
                                          constant float& aaScale [[buffer(3)]],
                                          constant float& strokeOnlyFlag [[buffer(4)]]) {
    // Sample SDF, invert to match our atlas (inside dark)
    float dist01 = 1.0 - sdfTexture.sample(sdfSampler, in.texCoord).r;
    // Allow tuning of the SDF edge center (default 0.5)
    float d = dist01 - edgeCenter;

    // Derivative-based smoothing in screen space with tunable scale
    // Wider AA window for outline stability
    float w = max(fwidth(d), 1e-6) * (aaScale * 2.0);
    // Optional: micro-jitter suppression to combat pixel snapping; comment out if too soft
    d = clamp(d, -1.0, 1.0);

    // Convert outline thickness (in px) to SDF domain
    float t = max(outlineWidth, 0.0) * w;

    // Anti-aliased fill coverage
    float fillA = smoothstep(-w, +w, d);

    // Stroke ring strictly outside the fill. Use a wider inner gate to
    // suppress stray stroke samples bleeding into the fill near the edge.
    float strokeOuter = smoothstep(-t - 1.0*w, -t + 1.0*w, d);
    float innerGate   = smoothstep(-1.5 * w, 1.5 * w, d);
    float strokeA = clamp(strokeOuter - innerGate, 0.0, 1.0);

    float4 strokeCol = float4(outlineColor.rgb, outlineColor.a * strokeA);
    float4 fillCol   = float4(in.color.rgb,     in.color.a     * fillA);

    // Two-pass support: when strokeOnlyFlag > 0.5, output only the stroke
    if (strokeOnlyFlag > 0.5) {
        return strokeCol;
    }

    // Single-pass fallback: fill over stroke (standard 'over' operator)
    float outA = clamp(fillCol.a + strokeCol.a * (1.0 - fillCol.a), 0.0, 1.0);
    float3 outRGB = (fillCol.rgb * fillCol.a) + (strokeCol.rgb * strokeCol.a * (1.0 - fillCol.a));
    return float4(outRGB, outA);
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

// === MSDF text shaders ===

static inline float gn_msdf_median3(float a, float b, float c) {
    return max(min(a, b), min(max(a, b), c));
}

// MSDF fill
fragment float4 msdf_text_fragment(VertexOut in [[stage_in]],
                                   texture2d<float> msdfTexture [[texture(0)]],
                                   sampler msdfSampler [[sampler(0)]]) {
    float3 sampleRGB = msdfTexture.sample(msdfSampler, in.texCoord).rgb;
    float sd = gn_msdf_median3(sampleRGB.r, sampleRGB.g, sampleRGB.b) - 0.5;
    float w = max(fwidth(sd), 1e-6);
    float alpha = smoothstep(0.0 - w, 0.0 + w, sd);
    float4 col = in.color;
    col.a *= alpha;
    return col;
}

// MSDF outline (two-pass compatible)
fragment float4 msdf_text_outline_fragment(VertexOut in [[stage_in]],
                                           texture2d<float> msdfTexture [[texture(0)]],
                                           sampler msdfSampler [[sampler(0)]],
                                           constant float& outlineWidth [[buffer(0)]],
                                           constant float4& outlineColor [[buffer(1)]],
                                           constant float& strokeOnlyFlag [[buffer(2)]]) {
    float3 s = msdfTexture.sample(msdfSampler, in.texCoord).rgb;
    float sd = gn_msdf_median3(s.r, s.g, s.b) - 0.5;
    float w = max(fwidth(sd), 1e-6);
    float t = outlineWidth * w;

    float fillA = smoothstep(0.0 - w, 0.0 + w, sd);
    float strokeOuter = smoothstep(-t - w, -t + w, sd);
    float innerGate   = smoothstep(-w, +w, sd);
    float strokeA = clamp(strokeOuter - innerGate, 0.0, 1.0);

    float4 strokeCol = float4(outlineColor.rgb, outlineColor.a * strokeA);
    if (strokeOnlyFlag > 0.5) { return strokeCol; }
    float4 fillCol = float4(in.color.rgb, in.color.a * fillA);
    float outA = clamp(fillCol.a + strokeCol.a * (1.0 - fillCol.a), 0.0, 1.0);
    float3 outRGB = (fillCol.rgb * fillCol.a) + (strokeCol.rgb * strokeCol.a * (1.0 - fillCol.a));
    return float4(outRGB, outA);
}