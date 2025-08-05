//
//  MetalMatrixHelpers.swift
//  FloppyTurd
//
//  Created by Gnosis Engine
//  Copyright © 2024 Floppy Turd Studios. All rights reserved.
//

import Foundation
import simd

/// Matrix helper functions for Metal rendering operations
struct MetalMatrixHelpers {
    
    // MARK: - Matrix Creation
    
    /// Creates a translation matrix
    /// - Parameters:
    ///   - x: X translation
    ///   - y: Y translation
    ///   - z: Z translation (default: 0.0)
    /// - Returns: 4x4 translation matrix
    static func translationMatrix(x: Float, y: Float, z: Float = 0.0) -> simd_float4x4 {
        return simd_float4x4(
            simd_float4(1.0, 0.0, 0.0, 0.0),
            simd_float4(0.0, 1.0, 0.0, 0.0),
            simd_float4(0.0, 0.0, 1.0, 0.0),
            simd_float4(x, y, z, 1.0)
        )
    }
    
    /// Creates a scale matrix
    /// - Parameters:
    ///   - x: X scale
    ///   - y: Y scale
    ///   - z: Z scale (default: 1.0)
    /// - Returns: 4x4 scale matrix
    static func scaleMatrix(x: Float, y: Float, z: Float = 1.0) -> simd_float4x4 {
        return simd_float4x4(
            simd_float4(x, 0.0, 0.0, 0.0),
            simd_float4(0.0, y, 0.0, 0.0),
            simd_float4(0.0, 0.0, z, 0.0),
            simd_float4(0.0, 0.0, 0.0, 1.0)
        )
    }
    
    /// Creates a rotation matrix around the Z axis
    /// - Parameter angleDegrees: Rotation angle in degrees
    /// - Returns: 4x4 rotation matrix
    static func rotationMatrixZ(angleDegrees: Float) -> simd_float4x4 {
        let radians = angleDegrees * Float.pi / 180.0
        let cosRot = cos(radians)
        let sinRot = sin(radians)
        
        return simd_float4x4(
            simd_float4(cosRot, -sinRot, 0.0, 0.0),
            simd_float4(sinRot, cosRot, 0.0, 0.0),
            simd_float4(0.0, 0.0, 1.0, 0.0),
            simd_float4(0.0, 0.0, 0.0, 1.0)
        )
    }
    
    /// Creates a rotation matrix around the X axis
    /// - Parameter angleDegrees: Rotation angle in degrees
    /// - Returns: 4x4 rotation matrix
    static func rotationMatrixX(angleDegrees: Float) -> simd_float4x4 {
        let radians = angleDegrees * Float.pi / 180.0
        let cosRot = cos(radians)
        let sinRot = sin(radians)
        
        return simd_float4x4(
            simd_float4(1.0, 0.0, 0.0, 0.0),
            simd_float4(0.0, cosRot, -sinRot, 0.0),
            simd_float4(0.0, sinRot, cosRot, 0.0),
            simd_float4(0.0, 0.0, 0.0, 1.0)
        )
    }
    
    /// Creates a rotation matrix around the Y axis
    /// - Parameter angleDegrees: Rotation angle in degrees
    /// - Returns: 4x4 rotation matrix
    static func rotationMatrixY(angleDegrees: Float) -> simd_float4x4 {
        let radians = angleDegrees * Float.pi / 180.0
        let cosRot = cos(radians)
        let sinRot = sin(radians)
        
        return simd_float4x4(
            simd_float4(cosRot, 0.0, sinRot, 0.0),
            simd_float4(0.0, 1.0, 0.0, 0.0),
            simd_float4(-sinRot, 0.0, cosRot, 0.0),
            simd_float4(0.0, 0.0, 0.0, 1.0)
        )
    }
    
    // MARK: - Orthographic Projection
    
    /// Creates an orthographic projection matrix
    /// - Parameters:
    ///   - left: Left edge of viewport
    ///   - right: Right edge of viewport
    ///   - bottom: Bottom edge of viewport
    ///   - top: Top edge of viewport
    ///   - near: Near clipping plane
    ///   - far: Far clipping plane
    /// - Returns: 4x4 orthographic projection matrix
    static func orthographicMatrix(left: Float, right: Float, bottom: Float, top: Float, near: Float, far: Float) -> simd_float4x4 {
        return simd_float4x4(
            simd_float4(2.0 / (right - left), 0, 0, 0),
            simd_float4(0, 2.0 / (top - bottom), 0, 0),
            simd_float4(0, 0, -2.0 / (far - near), 0),
            simd_float4(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1)
        )
    }
    
    // MARK: - Sprite Transformations
    
    /// Creates a transformation matrix for a sprite positioned from top-left (standard rendering)
    /// - Parameters:
    ///   - position: Sprite position (x, y) - top-left corner
    ///   - scale: Sprite scale (x, y)
    ///   - rotation: Rotation angle in degrees (rotates around top-left)
    /// - Returns: 4x4 transformation matrix
    static func spriteTransformMatrix(position: (x: Float, y: Float), scale: (x: Float, y: Float), rotation: Float) -> simd_float4x4 {
        // Standard top-left positioning without centering
        let rotation = rotationMatrixZ(angleDegrees: rotation)
        let scale = scaleMatrix(x: scale.x, y: scale.y)
        let translation = translationMatrix(x: position.x, y: position.y)
        
        // Combine transformations: translation * scale * rotation
        return translation * scale * rotation
    }
    
    /// Creates a complete transformation matrix for a sprite with rotation (centered for rotation)
    /// - Parameters:
    ///   - position: Sprite position (x, y)
    ///   - scale: Sprite scale (x, y)
    ///   - rotation: Rotation angle in degrees
    /// - Returns: 4x4 transformation matrix
    static func spriteTransformMatrixCentered(position: (x: Float, y: Float), scale: (x: Float, y: Float), rotation: Float) -> simd_float4x4 {
        // For proper rotation around center, we need to:
        // 1. Translate to center the sprite at origin
        // 2. Apply rotation
        // 3. Apply scale
        // 4. Translate to final position
        
        let centerTranslation = translationMatrix(x: -0.5, y: -0.5)  // Center the unit quad
        let rotation = rotationMatrixZ(angleDegrees: rotation)
        let scale = scaleMatrix(x: scale.x, y: scale.y)
        let finalTranslation = translationMatrix(x: position.x, y: position.y)
        
        // Combine transformations: finalTranslation * scale * rotation * centerTranslation
        return finalTranslation * scale * rotation * centerTranslation
    }
    
    /// Creates a complete transformation matrix for a 2D texture (no rotation)
    /// - Parameters:
    ///   - position: Texture position (x, y)
    ///   - size: Texture size (width, height)
    /// - Returns: 4x4 transformation matrix
    static func textureTransformMatrix(position: (x: Float, y: Float), size: (width: Float, height: Float)) -> simd_float4x4 {
        let translation = translationMatrix(x: position.x, y: position.y)
        let scale = scaleMatrix(x: size.width, y: size.height)
        
        // Combine transformations: translation * scale
        return translation * scale
    }
    
    // MARK: - Matrix Utilities
    
    /// Creates an identity matrix
    /// - Returns: 4x4 identity matrix
    static func identityMatrix() -> simd_float4x4 {
        return simd_float4x4(
            simd_float4(1.0, 0.0, 0.0, 0.0),
            simd_float4(0.0, 1.0, 0.0, 0.0),
            simd_float4(0.0, 0.0, 1.0, 0.0),
            simd_float4(0.0, 0.0, 0.0, 1.0)
        )
    }
    
    /// Creates a viewport projection matrix for 2D rendering
    /// - Parameters:
    ///   - width: Viewport width
    ///   - height: Viewport height
    /// - Returns: 4x4 projection matrix
    static func viewportProjectionMatrix(width: Float, height: Float) -> simd_float4x4 {
        return orthographicMatrix(
            left: 0.0,
            right: width,
            bottom: height,   // Correct: bottom is height (bottom of screen)
            top: 0.0,         // Correct: top is 0.0 (top of screen)
            near: -1.0,
            far: 1.0
        )
    }
} 