//
//  SwiftTypes.swift  
//  Professional Game Engine - Swift Type Definitions
//
//  Created by C++ Swift Interop Migration
//  Swift equivalents for common game types - PROPER SWIFT APPROACH
//

import Foundation
import simd
#if canImport(UIKit)
import UIKit
#endif

// MARK: - Core Game Types

/// Swift equivalent of raylib Color struct
public struct RaylibColor: Sendable {
    public let r: UInt8
    public let g: UInt8
    public let b: UInt8
    public let a: UInt8
    
    public init(r: UInt8, g: UInt8, b: UInt8, a: UInt8 = 255) {
        self.r = r
        self.g = g
        self.b = b
        self.a = a
    }
    
    // Common colors
    public static let white = RaylibColor(r: 255, g: 255, b: 255, a: 255)
    public static let black = RaylibColor(r: 0, g: 0, b: 0, a: 255)
    public static let red = RaylibColor(r: 255, g: 0, b: 0, a: 255)
    public static let green = RaylibColor(r: 0, g: 255, b: 0, a: 255)
    public static let blue = RaylibColor(r: 0, g: 0, b: 255, a: 255)
    public static let clear = RaylibColor(r: 0, g: 0, b: 0, a: 0)
}

/// Swift equivalent of raylib Rectangle struct























@_expose(Cxx)
public struct Rectangle: Sendable {
    public let x: Float
    public let y: Float
    public let width: Float
    public let height: Float
    
    public init(x: Float, y: Float, width: Float, height: Float) {
        self.x = x
        self.y = y
        self.width = width
        self.height = height
    }
    
    /// Check if a point is inside this rectangle
    public func contains(point: Vector2) -> Bool {
        return point.x >= x && point.x <= (x + width) &&
               point.y >= y && point.y <= (y + height)
    }
}

/// Swift equivalent of raylib Vector2 struct
@_expose(Cxx)
public struct Vector2: Sendable {
    public let x: Float
    public let y: Float
    
    public init(x: Float, y: Float) {
        self.x = x
        self.y = y
    }
    
    public static let zero = Vector2(x: 0, y: 0)
}



// MARK: - Texture Types

/// Swift equivalent of raylib Texture2D struct
public struct Texture2D: Sendable {
    public let id: UInt32
    public let width: Int32
    public let height: Int32
    public let mipmaps: Int32
    public let format: Int32
    
    public init(id: UInt32, width: Int32, height: Int32, mipmaps: Int32 = 1, format: Int32 = 0) {
        self.id = id
        self.width = width
        self.height = height
        self.mipmaps = mipmaps
        self.format = format
    }
    
    public static let empty = Texture2D(id: 0, width: 0, height: 0)
}

/// Swift equivalent of raylib Image struct
///
/// NOTE: Marked as @unchecked Sendable because it contains an UnsafeMutableRawPointer,
/// which is not Sendable. Use with caution and do not share across concurrency domains
/// unless you are certain of thread safety.
public struct Image: @unchecked Sendable {
    public let data: UnsafeMutableRawPointer?
    public let width: Int32
    public let height: Int32
    public let mipmaps: Int32
    public let format: Int32
    
    public init(data: UnsafeMutableRawPointer?, width: Int32, height: Int32, mipmaps: Int32 = 1, format: Int32 = 0) {
        self.data = data
        self.width = width
        self.height = height
        self.mipmaps = mipmaps
        self.format = format
    }
}

/// Swift equivalent of raylib RenderTexture2D struct
public struct RenderTexture2D: Sendable {
    public let id: UInt32
    public let texture: Texture2D
    public let depth: Texture2D
    
    public init(id: UInt32, texture: Texture2D, depth: Texture2D) {
        self.id = id
        self.texture = texture
        self.depth = depth
    }
}

/// Swift equivalent of raylib Matrix struct
public struct Matrix: Sendable {
    public let m0, m1, m2, m3: Float
    public let m4, m5, m6, m7: Float
    public let m8, m9, m10, m11: Float
    public let m12, m13, m14, m15: Float
    
    public init(m0: Float, m1: Float, m2: Float, m3: Float,
                m4: Float, m5: Float, m6: Float, m7: Float,
                m8: Float, m9: Float, m10: Float, m11: Float,
                m12: Float, m13: Float, m14: Float, m15: Float) {
        self.m0 = m0; self.m1 = m1; self.m2 = m2; self.m3 = m3
        self.m4 = m4; self.m5 = m5; self.m6 = m6; self.m7 = m7
        self.m8 = m8; self.m9 = m9; self.m10 = m10; self.m11 = m11
        self.m12 = m12; self.m13 = m13; self.m14 = m14; self.m15 = m15
    }
    
    public static let identity = Matrix(
        m0: 1, m1: 0, m2: 0, m3: 0,
        m4: 0, m5: 1, m6: 0, m7: 0,
        m8: 0, m9: 0, m10: 1, m11: 0,
        m12: 0, m13: 0, m14: 0, m15: 1
    )
}

/// Swift equivalent of raylib Font struct  
/// Font struct for C++/Swift bridging. Holds an internal id for Swift-side font resource lookup.
/// Raylib-compatible Font struct for agnostic compatibility
public struct Font: Sendable {
    public let baseSize: Int32           // Base size (default chars height)
    public let glyphCount: Int32         // Number of glyph characters
    public let glyphPadding: Int32       // Padding around the glyph characters
    public let texture: Texture2D        // Texture atlas containing the glyphs
    public let recs: [Rectangle]         // Rectangles in texture for the glyphs
    public let glyphs: [GlyphInfo]       // Glyphs info data

    public init(baseSize: Int32, glyphCount: Int32, glyphPadding: Int32, texture: Texture2D, recs: [Rectangle], glyphs: [GlyphInfo]) {
        self.baseSize = baseSize
        self.glyphCount = glyphCount
        self.glyphPadding = glyphPadding
        self.texture = texture
        self.recs = recs
        self.glyphs = glyphs
    }
}

/// Raylib-compatible GlyphInfo struct for agnostic compatibility
public struct GlyphInfo: Sendable {
    public let value: Int32          // Character value (Unicode)
    public let offsetX: Int32        // Character offset X when drawing
    public let offsetY: Int32        // Character offset Y when drawing
    public let advanceX: Int32       // Character advance position X
    public let image: Image          // Character image data

    public init(value: Int32, offsetX: Int32, offsetY: Int32, advanceX: Int32, image: Image) {
        self.value = value
        self.offsetX = offsetX
        self.offsetY = offsetY
        self.advanceX = advanceX
        self.image = image
    }
}
