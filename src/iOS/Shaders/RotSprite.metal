//
//  RotSprite.metal
//  PooperTrooper
//
//  RotSprite algorithm implementation for high-quality pixel art rotation
//  Created by Gnosis Engine
//  Copyright © 2024 Pooper Trooper Studios. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;

// MARK: - Constants and Structures

/// RotSprite algorithm constants
constant int ROTSPRITE_SCALE_FACTOR = 8;  // 8x upscaling factor
constant float COLOR_SIMILARITY_THRESHOLD = 0.1f;  // Threshold for color similarity in Scale2x

/// RotSprite processing parameters
struct RotSpriteParams {
    float rotationAngle;        // Rotation angle in radians
    uint2 originalSize;         // Original sprite dimensions
    uint2 upscaledSize;         // Upscaled sprite dimensions (originalSize * ROTSPRITE_SCALE_FACTOR)
    float2 rotationCenter;      // Rotation center in normalized coordinates (0.0-1.0) for OUTPUT
    float2 inputCenter;         // Rotation center in normalized coordinates (0.0-1.0) for INPUT (upscaled)
    float colorThreshold;       // Color similarity threshold for Scale2x
};

// MARK: - Utility Functions

/// Calculate color similarity between two pixels
inline float colorSimilarity(float4 color1, float4 color2) {
    float3 diff = color1.rgb - color2.rgb;
    return length(diff);
}

/// Check if two colors are similar within threshold
inline bool colorsAreSimilar(float4 color1, float4 color2, float threshold) {
    return colorSimilarity(color1, color2) < threshold;
}

/// Safely sample texture with bounds checking
inline float4 safeSample(texture2d<float, access::read> texture, uint2 coord, uint2 maxSize) {
    if (coord.x >= maxSize.x || coord.y >= maxSize.y) {
        return float4(0.0, 0.0, 0.0, 0.0);  // Transparent black for out-of-bounds
    }
    return texture.read(coord);
}

/// 2D rotation matrix
inline float2x2 rotationMatrix(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return float2x2(float2(c, -s), float2(s, c));
}

/// High-quality bilinear sampling for smooth texture interpolation
inline float4 sampleBilinear(texture2d<float, access::read> texture, float2 coord, uint2 textureSize) {
    // Get the four surrounding pixels for bilinear interpolation
    float2 pixelCoord = coord - 0.5;
    uint2 baseCoord = uint2(floor(pixelCoord));
    float2 frac = pixelCoord - float2(baseCoord);
    
    // Sample the four corners
    float4 tl = safeSample(texture, baseCoord + uint2(0, 0), textureSize);  // Top-left
    float4 tr = safeSample(texture, baseCoord + uint2(1, 0), textureSize);  // Top-right
    float4 bl = safeSample(texture, baseCoord + uint2(0, 1), textureSize);  // Bottom-left
    float4 br = safeSample(texture, baseCoord + uint2(1, 1), textureSize);  // Bottom-right
    
    // Bilinear interpolation
    float4 top = mix(tl, tr, frac.x);      // Interpolate top edge
    float4 bottom = mix(bl, br, frac.x);   // Interpolate bottom edge
    return mix(top, bottom, frac.y);       // Interpolate between top and bottom
}

// MARK: - Scale2x Algorithm Implementation

/// Modified Scale2x algorithm for RotSprite upscaling
/// This is the core of the RotSprite algorithm - intelligent upscaling that preserves pixel art quality
kernel void rotsprite_scale2x_upscale(
    texture2d<float, access::read> inputTexture [[texture(0)]],
    texture2d<float, access::write> outputTexture [[texture(1)]],
    constant RotSpriteParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    // Get input texture dimensions
    uint2 inputSize = uint2(inputTexture.get_width(), inputTexture.get_height());
    
    // Calculate which input pixel this thread group is processing
    uint2 inputCoord = gid / ROTSPRITE_SCALE_FACTOR;
    uint2 subPixel = gid % ROTSPRITE_SCALE_FACTOR;
    
    // Bounds check
    if (inputCoord.x >= inputSize.x || inputCoord.y >= inputSize.y) {
        return;
    }
    
    // Sample the 3x3 neighborhood around the current pixel for Scale2x
    //   A | B | C
    //   --+---+--
    //   D | E | F  <- E is our center pixel
    //   --+---+--
    //   G | H | I
    
    float4 A = safeSample(inputTexture, inputCoord + uint2(-1, -1), inputSize);
    float4 B = safeSample(inputTexture, inputCoord + uint2( 0, -1), inputSize);
    float4 C = safeSample(inputTexture, inputCoord + uint2( 1, -1), inputSize);
    float4 D = safeSample(inputTexture, inputCoord + uint2(-1,  0), inputSize);
    float4 E = inputTexture.read(inputCoord);  // Center pixel
    float4 F = safeSample(inputTexture, inputCoord + uint2( 1,  0), inputSize);
    float4 G = safeSample(inputTexture, inputCoord + uint2(-1,  1), inputSize);
    float4 H = safeSample(inputTexture, inputCoord + uint2( 0,  1), inputSize);
    float4 I = safeSample(inputTexture, inputCoord + uint2( 1,  1), inputSize);
    
    // Enhanced Scale2x algorithm with smooth interpolation for better RotSprite quality
    // Uses color similarity instead of exact equality, plus weighted blending
    
    bool verticalDifferent = !colorsAreSimilar(B, H, params.colorThreshold);
    bool horizontalDifferent = !colorsAreSimilar(D, F, params.colorThreshold);
    
    float4 outputColor = E;  // Default to center pixel
    
    // Calculate normalized sub-pixel position (0.0 to 1.0)
    float2 subPos = float2(subPixel) / float(ROTSPRITE_SCALE_FACTOR - 1);
    
    if (verticalDifferent && horizontalDifferent) {
        // Enhanced Scale2x with smooth transitions for better rotation quality
        
        // Calculate influence weights based on position within the 8x8 block
        float leftInfluence = 1.0 - subPos.x;
        float rightInfluence = subPos.x;
        float topInfluence = 1.0 - subPos.y;
        float bottomInfluence = subPos.y;
        
        // Apply Scale2x corner detection with weighted blending
        if (subPixel.x < ROTSPRITE_SCALE_FACTOR/2 && subPixel.y < ROTSPRITE_SCALE_FACTOR/2) {
            // Top-left quadrant - check for diagonal preservation
            if (colorsAreSimilar(D, B, params.colorThreshold) && 
                !colorsAreSimilar(D, H, params.colorThreshold) && 
                !colorsAreSimilar(B, F, params.colorThreshold)) {
                // Classic Scale2x case - preserve the diagonal
                outputColor = mix(E, D, leftInfluence * topInfluence * 0.6);
            } else {
                // Smooth interpolation towards most similar neighbor
                float4 blended = E;
                if (colorsAreSimilar(D, E, params.colorThreshold)) {
                    blended = mix(blended, D, leftInfluence * 0.3);
                }
                if (colorsAreSimilar(B, E, params.colorThreshold)) {
                    blended = mix(blended, B, topInfluence * 0.3);
                }
                outputColor = blended;
            }
        } else if (subPixel.x >= ROTSPRITE_SCALE_FACTOR/2 && subPixel.y < ROTSPRITE_SCALE_FACTOR/2) {
            // Top-right quadrant
            if (colorsAreSimilar(B, F, params.colorThreshold) && 
                !colorsAreSimilar(B, D, params.colorThreshold) && 
                !colorsAreSimilar(F, H, params.colorThreshold)) {
                outputColor = mix(E, B, rightInfluence * topInfluence * 0.6);
            } else {
                float4 blended = E;
                if (colorsAreSimilar(B, E, params.colorThreshold)) {
                    blended = mix(blended, B, topInfluence * 0.3);
                }
                if (colorsAreSimilar(F, E, params.colorThreshold)) {
                    blended = mix(blended, F, rightInfluence * 0.3);
                }
                outputColor = blended;
            }
        } else if (subPixel.x < ROTSPRITE_SCALE_FACTOR/2 && subPixel.y >= ROTSPRITE_SCALE_FACTOR/2) {
            // Bottom-left quadrant
            if (colorsAreSimilar(D, H, params.colorThreshold) && 
                !colorsAreSimilar(D, B, params.colorThreshold) && 
                !colorsAreSimilar(H, F, params.colorThreshold)) {
                outputColor = mix(E, D, leftInfluence * bottomInfluence * 0.6);
            } else {
                float4 blended = E;
                if (colorsAreSimilar(D, E, params.colorThreshold)) {
                    blended = mix(blended, D, leftInfluence * 0.3);
                }
                if (colorsAreSimilar(H, E, params.colorThreshold)) {
                    blended = mix(blended, H, bottomInfluence * 0.3);
                }
                outputColor = blended;
            }
        } else {
            // Bottom-right quadrant
            if (colorsAreSimilar(F, H, params.colorThreshold) && 
                !colorsAreSimilar(F, B, params.colorThreshold) && 
                !colorsAreSimilar(H, D, params.colorThreshold)) {
                outputColor = mix(E, F, rightInfluence * bottomInfluence * 0.6);
            } else {
                float4 blended = E;
                if (colorsAreSimilar(F, E, params.colorThreshold)) {
                    blended = mix(blended, F, rightInfluence * 0.3);
                }
                if (colorsAreSimilar(H, E, params.colorThreshold)) {
                    blended = mix(blended, H, bottomInfluence * 0.3);
                }
                outputColor = blended;
            }
        }
    } else {
        // No clear edges - apply subtle smoothing for rotation quality
        // This helps reduce aliasing artifacts when the upscaled image is rotated
        
        // Calculate a subtle blend with neighboring pixels
        float4 neighborAverage = (D + F + B + H) * 0.25;
        
        // Only blend slightly to maintain pixel art character
        if (colorSimilarity(E, neighborAverage) < params.colorThreshold * 2.0) {
            outputColor = mix(E, neighborAverage, 0.05);  // Very subtle smoothing
        }
        
        // Add position-based micro-adjustments for smoother rotation
        float microBlend = (subPos.x * 0.02) + (subPos.y * 0.02);
        if (microBlend > 0.01) {
            float4 positionBlend = mix(mix(D, F, subPos.x), mix(B, H, subPos.x), subPos.y);
            outputColor = mix(outputColor, positionBlend, microBlend);
        }
    }
    // If not different enough, keep the center pixel (E)
    
    // Write to output texture
    outputTexture.write(outputColor, gid);
}

// MARK: - Rotation and Downscaling

/// Rotate the upscaled texture and simultaneously downscale back to original size
/// This performs the actual rotation on the high-resolution intermediate image
kernel void rotsprite_rotate_and_downscale(
    texture2d<float, access::read> upscaledTexture [[texture(0)]],
    texture2d<float, access::write> outputTexture [[texture(1)]],
    constant RotSpriteParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint2 outputSize = uint2(outputTexture.get_width(), outputTexture.get_height());
    uint2 upscaledSize = uint2(upscaledTexture.get_width(), upscaledTexture.get_height());
    
    // Bounds check
    if (gid.x >= outputSize.x || gid.y >= outputSize.y) {
        return;
    }
    
    // Current pixel in output texture (center of pixel)
    float2 outputPixel = float2(gid) + 0.5;
    
    float2 outputSizeF = float2(outputSize);
    float2 upscaledSizeF = float2(upscaledSize);
    
    // Pivot in output (downscaled bb space)
    float2 pivotInOutput = params.rotationCenter * outputSizeF;
    
    // Pivot in upscaled (original upscaled space)
    float2 pivotInUpscaled = params.inputCenter * upscaledSizeF;
    
    // 1. Translate to origin (subtract pivot)
    float2 translatedPixel = outputPixel - pivotInOutput;
    
    // 2. Scale up the translated distance to map to upscaled resolution (this performs the downscaling)
    float2 scaledTranslated = translatedPixel * float(ROTSPRITE_SCALE_FACTOR);
    
    // 3. Apply INVERSE rotation (reverse mapping)
    float2x2 invRotMatrix = rotationMatrix(-params.rotationAngle);
    float2 rotatedPixel = invRotMatrix * scaledTranslated;
    
    // 4. Translate back to source space
    float2 sourceCoord = rotatedPixel + pivotInUpscaled;
    
    // Sample with bounds checking and nearest-neighbor for pixel-perfect results
    float4 outputColor = float4(0.0);  // Transparent background for out-of-bounds
    
    if (sourceCoord.x >= 0.0 && sourceCoord.x < upscaledSizeF.x &&
        sourceCoord.y >= 0.0 && sourceCoord.y < upscaledSizeF.y) {
        // Use nearest-neighbor sampling for pixel-perfect results (RotSprite best practice)
        uint2 nearestCoord = uint2(sourceCoord);
        outputColor = safeSample(upscaledTexture, nearestCoord, upscaledSize);
    }
    
    outputTexture.write(outputColor, gid);
}

// MARK: - Detail Restoration (Phase 5)

/// Restore single-pixel details that may have been lost during rotation
/// This is an optional post-processing step for maximum quality
kernel void rotsprite_restore_details(
    texture2d<float, access::read> originalTexture [[texture(0)]],
    texture2d<float, access::read> rotatedTexture [[texture(1)]],
    texture2d<float, access::write> outputTexture [[texture(2)]],
    constant RotSpriteParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint2 textureSize = uint2(originalTexture.get_width(), originalTexture.get_height());
    
    // Bounds check
    if (gid.x >= textureSize.x || gid.y >= textureSize.y) {
        return;
    }
    
    float4 originalPixel = originalTexture.read(gid);
    float4 rotatedPixel = rotatedTexture.read(gid);
    
    // Enhanced detail restoration with smart pixel analysis
    float colorDifference = colorSimilarity(originalPixel, rotatedPixel);
    
    if (colorDifference > params.colorThreshold) {
        // Analyze the neighborhood to determine if restoration is beneficial
        float4 neighbors[4];
        neighbors[0] = safeSample(rotatedTexture, gid + uint2(-1,  0), textureSize);  // Left
        neighbors[1] = safeSample(rotatedTexture, gid + uint2( 1,  0), textureSize);  // Right  
        neighbors[2] = safeSample(rotatedTexture, gid + uint2( 0, -1), textureSize);  // Up
        neighbors[3] = safeSample(rotatedTexture, gid + uint2( 0,  1), textureSize);  // Down
        
        int identicalNeighbors = 0;
        for (int i = 0; i < 4; i++) {
            if (colorsAreSimilar(rotatedPixel, neighbors[i], params.colorThreshold)) {
                identicalNeighbors++;
            }
        }
        
        // Advanced restoration logic based on neighborhood analysis
        if (identicalNeighbors >= 3) {
            // Strong evidence that this area was over-smoothed - restore original detail
            float blendFactor = min(colorDifference / params.colorThreshold, 1.0);
            float4 restoredPixel = mix(rotatedPixel, originalPixel, blendFactor * 0.8);
            outputTexture.write(restoredPixel, gid);
            return;
        } else if (identicalNeighbors >= 2 && colorDifference > params.colorThreshold * 2.0) {
            // Moderate evidence - apply partial restoration for subtle improvement
            float4 restoredPixel = mix(rotatedPixel, originalPixel, 0.3);
            outputTexture.write(restoredPixel, gid);
            return;
        }
        
        // Check for isolated pixels that might benefit from restoration
        bool isIsolated = true;
        for (int i = 0; i < 4; i++) {
            if (colorsAreSimilar(originalPixel, neighbors[i], params.colorThreshold * 1.5)) {
                isIsolated = false;
                break;
            }
        }
        
        if (isIsolated && colorDifference > params.colorThreshold * 1.5) {
            // This appears to be an isolated detail - restore it partially
            float4 restoredPixel = mix(rotatedPixel, originalPixel, 0.5);
            outputTexture.write(restoredPixel, gid);
            return;
        }
    }
    
    // No restoration needed - keep the rotated pixel
    outputTexture.write(rotatedPixel, gid);
}
